#include "capture.h"

HRESULT RunSampleGrabber(PCWSTR pszFileName)
{
    HRESULT hr = S_OK;
    CMFCamCapture capture;
    CHECK_HR(hr = capture.init());
    CHECK_HR(hr = capture.start());
    Sleep(5000);
    capture.stop();
done:
    return S_OK;
}

int wmain(int argc, wchar_t* argv[])
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (SUCCEEDED(hr)) {
        hr = MFStartup(MF_VERSION);
        if (SUCCEEDED(hr)) {
            hr = RunSampleGrabber(argv[1]);
            MFShutdown();
        }
        CoUninitialize();
    }
    return 0;
}

// SampleGrabberCB implementation

// Create a new instance of the object.
HRESULT SampleGrabberCB::CreateInstance(SampleGrabberCB **ppCB)
{
    *ppCB = new (std::nothrow) SampleGrabberCB();

    if (ppCB == NULL) {
        return E_OUTOFMEMORY;
    }
    return S_OK;
}

STDMETHODIMP SampleGrabberCB::QueryInterface(REFIID riid, void** ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(SampleGrabberCB, IMFSampleGrabberSinkCallback),
        QITABENT(SampleGrabberCB, IMFClockStateSink),
        { 0 }
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) SampleGrabberCB::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) SampleGrabberCB::Release()
{
    ULONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0) {
        delete this;
    }
    return cRef;

}

// IMFClockStateSink methods.

// In these example, the IMFClockStateSink methods do not perform any actions. 
// You can use these methods to track the state of the sample grabber sink.

STDMETHODIMP SampleGrabberCB::OnClockStart(MFTIME hnsSystemTime, LONGLONG llClockStartOffset)
{
    return S_OK;
}

STDMETHODIMP SampleGrabberCB::OnClockStop(MFTIME hnsSystemTime)
{
    return S_OK;
}

STDMETHODIMP SampleGrabberCB::OnClockPause(MFTIME hnsSystemTime)
{
    return S_OK;
}

STDMETHODIMP SampleGrabberCB::OnClockRestart(MFTIME hnsSystemTime)
{
    return S_OK;
}

STDMETHODIMP SampleGrabberCB::OnClockSetRate(MFTIME hnsSystemTime, float flRate)
{
    return S_OK;
}

// IMFSampleGrabberSink methods.

STDMETHODIMP SampleGrabberCB::OnSetPresentationClock(IMFPresentationClock* pClock)
{
    return S_OK;
}

STDMETHODIMP SampleGrabberCB::OnProcessSample(REFGUID guidMajorMediaType, DWORD dwSampleFlags,
    LONGLONG llSampleTime, LONGLONG llSampleDuration, const BYTE * pSampleBuffer,
    DWORD dwSampleSize)
{
    // Display information about the sample.
    DBGMSG(L"Sample: start = %I64d, duration = %I64d, bytes = %d\n", llSampleTime, llSampleDuration, dwSampleSize);
    if (m_procCallback) {
        m_procCallback(pSampleBuffer, dwSampleSize);
    }
    return S_OK;
}

STDMETHODIMP SampleGrabberCB::OnShutdown()
{
    return S_OK;
}
