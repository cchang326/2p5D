#include "stdafx.h"
#include "parallax.h"

namespace Parallax {

bool compareLayerDepth(LayerImage &p1, LayerImage &p2)
{
    return p1.depth > p2.depth;
}

void createCompatibleDIB(DWORD width, DWORD height, HWND hwnd, DWORD compression, DWORD bpp,
                         HDC *pdc, HBITMAP *pbmp, PixelMap *pMap)
{
    BYTE *pCanvas;
    BITMAPINFO bmi;

    bmi.bmiHeader.biSize = sizeof (bmi.bmiHeader);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = (WORD)bpp;
    bmi.bmiHeader.biCompression = compression;
    bmi.bmiHeader.biSizeImage = 0;
    bmi.bmiHeader.biClrUsed = 0;
    bmi.bmiHeader.biClrImportant = 0;
    HDC hdc = GetDC(hwnd);    
    *pdc = CreateCompatibleDC(hdc);
    *pbmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (void**)&pCanvas, NULL, 0);
    SelectObject(*pdc, *pbmp);

    // save canvas
    memset(pCanvas, 0, width * height * bmi.bmiHeader.biBitCount / 8);
    pMap->pixels = pCanvas;
    pMap->width = width;
    pMap->height = height;
    ASSERT(compression == BI_RGB);
    pMap->format = (bpp == 32) ? ARGB32 : RGB24;
    pMap->stride = width * (bpp >> 3);
    pMap->bpp = bpp;

    DeleteDC(hdc);
}

BOOL CPainter::Init(DWORD width, DWORD height, HWND hwnd)
{
    createCompatibleDIB(width, height, hwnd, BI_RGB, 32, &m_cdc, &m_hbmp, &m_canvas);
    loadLayers();
    return TRUE;
}

void CPainter::loadLayers()
{
    const wchar_t* layerFiles[] = {
        L"images\\layers\\0_sky.png",
        L"images\\layers\\1_grass.png",
        L"images\\layers\\2_trees.png",
        L"images\\layers\\3_trees.png",
        L"images\\layers\\4_trees.png"
    };
    const int layerDepth[] = { 1000, 10, 8, 5, 2 };
    for (int i = 0; i < _countof(layerFiles); i++) {
        Bitmap* bmp = new Bitmap(layerFiles[i], FALSE);
        if (bmp) {
            GdiPlusBmp gdiBmp = { 0 };
            gdiBmp.pBitmap = bmp;
            Rect rect(0, 0, bmp->GetWidth(), bmp->GetHeight());
            bmp->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &gdiBmp.bmpData);
            m_bitmaps.push_back(gdiBmp);

            // convert to layer.
            LayerImage depImg = { 0 };
            depImg.img.width = gdiBmp.bmpData.Width;
            depImg.img.height = gdiBmp.bmpData.Height;
            depImg.img.stride = gdiBmp.bmpData.Stride;
            depImg.img.pixels = (BYTE*)gdiBmp.bmpData.Scan0;
            depImg.img.bpp = 32;
            depImg.img.format = ARGB32;
            depImg.depth = layerDepth[i];
            depImg.img.invert(); // invert vertically.
            m_layers.push_back(depImg);
        }
        else {
            assert(0);
        }
    }
}

void CPainter::updateCameraPos(int shiftX, int shiftY)
{
    m_cameraShift = { shiftX, shiftY };
}

void CPainter::Draw()
{
    m_blender.blendLayers(m_layers, &m_canvas, m_cameraShift);
}

void CBlender::blendSingleLayer(const PixelMap *pLayer, COORD& offset, PixelMap *pDstImg)
{
    if (pLayer->isInverted()) {
        offset.Y = -offset.Y;
    }

    Rect dstRect(0, 0, pDstImg->width, pDstImg->height);
    Rect srcRect(offset.X, offset.Y, pLayer->width, pLayer->height);
    dstRect.Intersect(srcRect);
    int srcStartX = 0, srcStartY = 0;
    if (offset.X < 0) {
        srcStartX = -offset.X;
    }
    if (offset.Y < 0) {
        srcStartY = -offset.Y;
    }
    const BYTE *pSrc = pLayer->getPtr(srcStartX, srcStartY);
    BYTE *pDst = pDstImg->getPtr(dstRect.X, dstRect.Y);
    for (int y = 0; y < dstRect.Height; y++) {
        for (int x = 0; x < dstRect.Width; x++) {
            unsigned int s = *(unsigned int*)(&pSrc[x << 2]);
            if (s >> 24) {
                *(int*)(&pDst[x << 2]) = s;
            }
        }
        pSrc += pLayer->stride;
        pDst += pDstImg->stride;
    }
}

void CBlender::blendLayers(std::vector<LayerImage>& layers, PixelMap *pDstImg, const COORD& cameraShift)
{
    // sort from back to front.
    std::sort(layers.begin(), layers.end(), compareLayerDepth);
    for (const LayerImage& layer : layers) {
        COORD shift = getShift(layer.depth, cameraShift);
        blendSingleLayer(&layer.img, shift, pDstImg);        
    }
}

CTracker::~CTracker()
{
    DeleteDC(m_dc);
    DeleteObject(m_hBmp);
}

HRESULT CTracker::initialize(int width, int height, HWND hWnd)
{
    HRESULT hr = S_OK;
    CLSID uuid;
    createCompatibleDIB(width, height, hWnd, BI_RGB, 24, &m_dc, &m_hBmp, &m_map);
    m_hwnd = hWnd;

    hr = m_capture.init();
    if (FAILED(hr)) {
        goto done;
    }
    m_capture.setSampleCallback(createSampleProcessCallbackFunc());
    m_width = m_capture.getCaptureWidth();
    m_height = m_capture.getCaptureHeight();
    uuid = m_capture.getCaptureFormat();
    if (uuid == MFVideoFormat_RGB24) {
        m_format = RGB24;
        m_bpp = 24;
    } else {
        // todo: add YUV support.
        ASSERT(0);
    }

done:
    return hr;
}

HRESULT CTracker::start()
{
    return m_capture.start();
}

HRESULT CTracker::stop()
{
    return m_capture.stop();
}

void CTracker::detectFace(const BYTE * pSampleBuffer, DWORD dwSampleSize)
{
    //ASSERT(dwSampleSize == m_width * m_height * (m_bpp >> 3));
    
    // paint to canvas.
    memcpy(m_map.pixels, pSampleBuffer, dwSampleSize);
    RECT rect = {0, 0, m_width, m_height};
    InvalidateRect(m_hwnd, &rect, TRUE);
}

const SampleProcessFunc CTracker::createSampleProcessCallbackFunc()
{
    return SampleProcessFunc(
        [this](const BYTE * sample, DWORD size) {
        detectFace(sample, size);
    });
}

}