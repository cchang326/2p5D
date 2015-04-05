#pragma once

#include <new>
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <Mferror.h>
#include <Shlwapi.h>
#include <stdio.h>

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

HRESULT LogMediaType(IMFMediaType *pType);
HRESULT CreateCaptureSource(IMFMediaSource **ppSource);