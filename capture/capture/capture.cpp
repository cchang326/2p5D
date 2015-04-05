#include "capture.h"

HRESULT ChooseCaptureFormats(IMFMediaSource *pSource);
HRESULT SetDeviceFormat(IMFMediaSource *pSource, DWORD dwFormatIndex);
HRESULT CreateVideoDeviceSource(IMFMediaSource **ppSource);

bool SelectMediaType(IMFMediaSource *pSource, IMFMediaType *pType);

HRESULT CreateCaptureSource(IMFMediaSource **ppSource)
{
    HRESULT hr = S_OK;
    CHECK_HR(hr = CreateVideoDeviceSource(ppSource)); 
    CHECK_HR(hr = ChooseCaptureFormats(*ppSource));    

done:
    return hr;
}

HRESULT CreateVideoDeviceSource(IMFMediaSource **ppSource)
{
    *ppSource = NULL;

    IMFMediaSource *pSource = NULL;
    IMFAttributes *pAttributes = NULL;
    IMFActivate **ppDevices = NULL;

    // Create an attribute store to specify the enumeration parameters.
    HRESULT hr = MFCreateAttributes(&pAttributes, 1);
    if (FAILED(hr)) {
        goto done;
    }

    // Source type: video capture devices
    hr = pAttributes->SetGUID(
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
        );
    if (FAILED(hr)) {
        goto done;
    }

    // Enumerate devices.
    UINT32 count;
    hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
    if (FAILED(hr)) {
        goto done;
    }

    if (count == 0) {
        hr = E_FAIL;
        goto done;
    }

    // Create the media source object.
    hr = ppDevices[0]->ActivateObject(IID_PPV_ARGS(&pSource));
    if (FAILED(hr)) {
        goto done;
    }

    *ppSource = pSource;
    (*ppSource)->AddRef();

done:
    SafeRelease(&pAttributes);

    for (DWORD i = 0; i < count; i++) {
        SafeRelease(&ppDevices[i]);
    }
    CoTaskMemFree(ppDevices);
    SafeRelease(&pSource);
    return hr;
}

HRESULT ChooseCaptureFormats(IMFMediaSource *pSource)
{
    IMFPresentationDescriptor *pPD = NULL;
    IMFStreamDescriptor *pSD = NULL;
    IMFMediaTypeHandler *pHandler = NULL;
    IMFMediaType *pType = NULL;
    DWORD i;

    HRESULT hr = pSource->CreatePresentationDescriptor(&pPD);
    if (FAILED(hr)) {
        goto done;
    }

    BOOL fSelected;
    hr = pPD->GetStreamDescriptorByIndex(0, &fSelected, &pSD);
    if (FAILED(hr)) {
        goto done;
    }

    hr = pSD->GetMediaTypeHandler(&pHandler);
    if (FAILED(hr)) {
        goto done;
    }

    DWORD cTypes = 0;
    hr = pHandler->GetMediaTypeCount(&cTypes);
    if (FAILED(hr)) {
        goto done;
    }

    for (i = 0; i < cTypes; i++) {
        hr = pHandler->GetMediaTypeByIndex(i, &pType);
        if (FAILED(hr)) {
            goto done;
        }

        /*LogMediaType(pType);
        OutputDebugString(L"\n");*/
        bool found = SelectMediaType(pSource, pType);
        SafeRelease(&pType);
        if (found) {
            hr = SetDeviceFormat(pSource, i);
            break;
        }
    }

    if (i >= cTypes) {
        hr = E_FAIL;
    }

done:
    SafeRelease(&pPD);
    SafeRelease(&pSD);
    SafeRelease(&pHandler);
    SafeRelease(&pType);
    return hr;
}

bool SelectMediaType(IMFMediaSource *pSource, IMFMediaType *pType)
{
    UINT32 count = 0;
    UINT32 found = 0;
    UINT32 i;

    HRESULT hr = pType->GetCount(&count);
    if (FAILED(hr)) {
        return false;
    }

    if (count == 0) {
        return false;
    }

    for (i = 0; i < count; i++) {
        GUID guid = { 0 };
        PROPVARIANT var;
        PropVariantInit(&var);
        
        hr = pType->GetItemByIndex(i, &guid, &var);
        if (FAILED(hr)) {
            goto done;
        }
        if (guid == MF_MT_FRAME_SIZE) {
            UINT32 uHigh = 0, uLow = 0;
            Unpack2UINT32AsUINT64(var.uhVal.QuadPart, &uHigh, &uLow);
            if (uHigh == 640 && (uLow == 480 || uLow == 360)) {
                found |= 1;
            }
        }
        else if (guid == MF_MT_MAJOR_TYPE) {
            if (*var.puuid == MFMediaType_Video) {
                found |= 2;
            }
        }
        else if (guid == MF_MT_SUBTYPE) {
            if (*var.puuid == MFVideoFormat_RGB24 || *var.puuid == MFVideoFormat_I420 ||
                *var.puuid == MFVideoFormat_IYUV) {
                found |= 4;
            }
        }
    done:
        PropVariantClear(&var);

        if (found == 7) {
            return true;
        }
    }
    
    return false;
}

HRESULT SetDeviceFormat(IMFMediaSource *pSource, DWORD dwFormatIndex)
{
    IMFPresentationDescriptor *pPD = NULL;
    IMFStreamDescriptor *pSD = NULL;
    IMFMediaTypeHandler *pHandler = NULL;
    IMFMediaType *pType = NULL;

    HRESULT hr = pSource->CreatePresentationDescriptor(&pPD);
    if (FAILED(hr)) {
        goto done;
    }

    BOOL fSelected;
    hr = pPD->GetStreamDescriptorByIndex(0, &fSelected, &pSD);
    if (FAILED(hr)) {
        goto done;
    }

    hr = pSD->GetMediaTypeHandler(&pHandler);
    if (FAILED(hr)) {
        goto done;
    }

    hr = pHandler->GetMediaTypeByIndex(dwFormatIndex, &pType);
    if (FAILED(hr)) {
        goto done;
    }

    hr = pHandler->SetCurrentMediaType(pType);

done:
    SafeRelease(&pPD);
    SafeRelease(&pSD);
    SafeRelease(&pHandler);
    SafeRelease(&pType);
    return hr;
}
