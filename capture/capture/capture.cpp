#include "capture.h"

CMFCamCapture::CMFCamCapture() :
    m_videoDeviceId(2)
{
}

CMFCamCapture::~CMFCamCapture()
{
    stop();
    if (m_spSource) {
        m_spSource->Shutdown();
    }
    if (m_spSession) {
        m_spSession->Shutdown();
    }
    MFShutdown();
    CoUninitialize();
}

HRESULT CMFCamCapture::init()
{
    SampleGrabberCB *pCallback = NULL;
    IMFMediaType *pType = NULL;
    HRESULT hr;

    CHECK_HR(hr = CoInitializeEx(NULL, COINIT_MULTITHREADED));
    CHECK_HR(hr = MFStartup(MF_VERSION));

    // Configure the media type that the Sample Grabber will receive.
    // Setting the major and subtype is usually enough for the topology loader
    // to resolve the topology.

    CHECK_HR(hr = MFCreateMediaType(&pType));
    CHECK_HR(hr = pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
    CHECK_HR(hr = pType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_I420));

    // Create the sample grabber sink.
    CHECK_HR(hr = SampleGrabberCB::CreateInstance(&pCallback));
    CHECK_HR(hr = MFCreateSampleGrabberSinkActivate(pType, pCallback, &m_spSinkActivate));

    // To run as fast as possible, set this attribute (requires Windows 7):
    CHECK_HR(hr = m_spSinkActivate->SetUINT32(MF_SAMPLEGRABBERSINK_IGNORE_CLOCK, TRUE));

    // Create the Media Session.
    CHECK_HR(hr = MFCreateMediaSession(NULL, &m_spSession));

    CHECK_HR(hr = enumVideoDevices());

    CHECK_HR(hr = setupVideoDevice());

    CHECK_HR(hr = setupMfSession());
    
done:    
    SafeRelease(&pCallback);
    SafeRelease(&pType);
    return hr;
}

HRESULT CMFCamCapture::setupMfSession()
{
    HRESULT hr = S_OK;
    IMFTopology *pTopology = NULL;
    
    // Create the topology.
    CHECK_HR(hr = CreateTopology(m_spSource, m_spSinkActivate, &pTopology));
    CHECK_HR(hr = m_spSession->SetTopology(0, pTopology));

done:
    SafeRelease(&pTopology);
    return hr;
}

HRESULT CMFCamCapture::setupVideoDevice()
{
    HRESULT hr = S_OK;

    if ((size_t)m_videoDeviceId >= m_deviceActivateObjects.size()) {
        hr = E_FAIL;
        goto done;
    }

    // activate
    m_deviceActivateObjects[m_videoDeviceId]->ActivateObject(IID_PPV_ARGS(&m_spSource));
    if (FAILED(hr)) {
        goto done;
    }

    CHECK_HR(hr = chooseCaptureFormats());

done:
    return hr;
}

HRESULT CMFCamCapture::enumVideoDevices()
{
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

    // cache the devices.
    m_deviceActivateObjects.clear();
    for (DWORD i = 0; i < count; i++) {
        m_deviceActivateObjects.push_back(CComPtr<IMFActivate>(ppDevices[i]));
    }
    
done:
    SafeRelease(&pAttributes);
    for (DWORD i = 0; i < count; i++) {
        SafeRelease(&ppDevices[i]);
    }
    CoTaskMemFree(ppDevices);
    return hr;
}

HRESULT CMFCamCapture::chooseCaptureFormats()
{
    IMFPresentationDescriptor *pPD = NULL;
    IMFStreamDescriptor *pSD = NULL;
    IMFMediaTypeHandler *pHandler = NULL;
    IMFMediaType *pType = NULL;
    DWORD i;

    HRESULT hr = m_spSource->CreatePresentationDescriptor(&pPD);
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

        bool found = selectMediaType(m_spSource, pType);
        if (found) {
            LogMediaType(pType);
            OutputDebugString(L"\n");
        }
        SafeRelease(&pType);
        if (found) {            
            hr = SetDeviceFormat(m_spSource, i);
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

bool CMFCamCapture::selectMediaType(IMFMediaSource *pSource, IMFMediaType *pType)
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
            if (*var.puuid == MFVideoFormat_I420 ||
                *var.puuid == MFVideoFormat_IYUV /*||
                *var.puuid == MFVideoFormat_RGB24*/
                ) {
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

void CMFCamCapture::runSession()
{
    IMFMediaEvent *pEvent = NULL;
    PROPVARIANT var;
    PropVariantInit(&var);
    
    HRESULT hr = S_OK;
    CHECK_HR(hr = m_spSession->Start(&GUID_NULL, &var));

    while (1) {

        std::unique_lock<std::mutex> stateDataLock(m_stateDataMutex, std::defer_lock_t());
        stateDataLock.lock();
        if (std::this_thread::get_id() != m_captureThread.get_id()) {
            break;
        }
        stateDataLock.unlock();

        HRESULT hrStatus = S_OK;
        MediaEventType met;

        CHECK_HR(hr = m_spSession->GetEvent(0, &pEvent));
        CHECK_HR(hr = pEvent->GetStatus(&hrStatus));
        CHECK_HR(hr = pEvent->GetType(&met));

        if (FAILED(hrStatus)) {
            // printf("Session error: 0x%x (event id: %d)\n", hrStatus, met);
            hr = hrStatus;
            goto done;
        }
        if (met == MESessionEnded) {
            break;
        }
        SafeRelease(&pEvent);
    }

done:
    PropVariantClear(&var);
    SafeRelease(&pEvent);
}

HRESULT CMFCamCapture::start()
{
    std::unique_lock<std::mutex> stateDataLock(m_stateDataMutex);

    if (m_captureThread.get_id() != std::thread::id()) {
        // thread already running.
        return E_FAIL;
    }
    std::thread newThread([this]() { runSession(); });
    m_captureThread.swap(newThread);    
    return S_OK;
}

HRESULT CMFCamCapture::stop()
{
    std::unique_lock<std::mutex> stateDataLock(m_stateDataMutex, std::defer_lock_t());
    stateDataLock.lock();
    // Check if the callback thread is running.
    if (m_captureThread.get_id() == std::thread::id()) {
        return E_FAIL;
    }

    // Discard current callback thread. 
    // Attach it to a temporary local "thread" variable, signal it to stop and then if it is 
    // still exetuting wait unitl it finishes.
    std::thread stoppedThread;
    m_captureThread.swap(stoppedThread);
    
    stateDataLock.unlock();

    m_spSession->Stop();

    if (stoppedThread.joinable()) {
        stoppedThread.join();
    }
    return S_OK;
}