#pragma once

#include <new>
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <Mferror.h>
#include <Shlwapi.h>
#include <stdio.h>
#include <atlbase.h>
#include <atlcom.h>
#include <vector>
#include <mutex>
#include <thread>

#pragma comment(lib, "mfplat")
#pragma comment(lib, "mf")
#pragma comment(lib, "mfuuid")
#pragma comment(lib, "Shlwapi")

#define CHECK_HR(x) if (FAILED(x)) { goto done; }

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT) {
        (*ppT)->Release();
        *ppT = NULL;
    }
};

typedef std::function<void(const BYTE * pSampleBuffer, DWORD dwSampleSize)> SampleProcessFunc;

HRESULT LogMediaType(IMFMediaType *pType);
void DBGMSG(PCWSTR format, ...);
HRESULT CreateTopology(IMFMediaSource *pSource, IMFActivate *pSinkActivate, IMFTopology **ppTopo);
HRESULT AddOutputNode(IMFTopology *pTopology, IMFActivate *pActivate, DWORD dwId, IMFTopologyNode **ppNode);
HRESULT AddSourceNode(IMFTopology *pTopology, IMFMediaSource *pSource, IMFPresentationDescriptor *pPD, IMFStreamDescriptor *pSD, IMFTopologyNode **ppNode);
HRESULT SetDeviceFormat(IMFMediaSource *pSource, DWORD dwFormatIndex);

// The class that implements the callback interface.
class SampleGrabberCB : public IMFSampleGrabberSinkCallback {
    long m_cRef;

    SampleGrabberCB() : m_cRef(1), m_procCallback(nullptr) {}

public:
    static HRESULT CreateInstance(SampleGrabberCB **ppCB);
    void setSampleProcessCallback(SampleProcessFunc callback)
    {
        m_procCallback = callback;
    }
    
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IMFClockStateSink methods
    STDMETHODIMP OnClockStart(MFTIME hnsSystemTime, LONGLONG llClockStartOffset);
    STDMETHODIMP OnClockStop(MFTIME hnsSystemTime);
    STDMETHODIMP OnClockPause(MFTIME hnsSystemTime);
    STDMETHODIMP OnClockRestart(MFTIME hnsSystemTime);
    STDMETHODIMP OnClockSetRate(MFTIME hnsSystemTime, float flRate);

    // IMFSampleGrabberSinkCallback methods
    STDMETHODIMP OnSetPresentationClock(IMFPresentationClock* pClock);
    STDMETHODIMP OnProcessSample(REFGUID guidMajorMediaType, DWORD dwSampleFlags,
                                 LONGLONG llSampleTime, LONGLONG llSampleDuration, const BYTE * pSampleBuffer,
                                 DWORD dwSampleSize);
    STDMETHODIMP OnShutdown();

private:
    SampleProcessFunc m_procCallback;
};

class CMFCamCapture {
public:
    CMFCamCapture();
    ~CMFCamCapture();
    
    HRESULT init();
    HRESULT start();
    HRESULT stop();

    void setSampleCallback(SampleProcessFunc callback);
    DWORD getCaptureWidth() const { return m_captureWidth; }
    DWORD getCaptureHeight() const { return m_captureHeight; }
    CLSID getCaptureFormat() const { return m_captureFormat; }

private:
    HRESULT enumVideoDevices();
    HRESULT setupVideoDevice();
    HRESULT setupMfSession();
    void runSession();

    HRESULT chooseCaptureFormats();
    bool selectMediaType(IMFMediaSource *pSource, IMFMediaType *pType);
    
private:
    UINT m_videoDeviceId;
    UINT m_captureWidth;
    UINT m_captureHeight;
    CLSID m_captureFormat;

    mutable std::mutex m_stateDataMutex;
    std::thread m_captureThread;

    CComPtr<SampleGrabberCB> m_spSampleGrabber;
    CComPtr<IMFMediaSession> m_spSession;  
    CComPtr<IMFMediaSource> m_spSource;
    CComPtr<IMFActivate> m_spSinkActivate;
    std::vector<CComPtr<IMFActivate>> m_deviceActivateObjects;
};