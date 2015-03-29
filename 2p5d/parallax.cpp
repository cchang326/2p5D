#include "stdafx.h"
#include "parallax.h"

namespace Parallax {

bool compareLayerDepth(LayerImage &p1, LayerImage &p2)
{
    return p1.depth > p2.depth;
}

BOOL CPainter::Init(DWORD width, DWORD height, HWND hwnd)
{
    BYTE *pCanvas;
    
    m_bmi.bmiHeader.biSize = sizeof (m_bmi.bmiHeader);
    m_bmi.bmiHeader.biWidth = width;
    m_bmi.bmiHeader.biHeight = height;
    m_bmi.bmiHeader.biPlanes = 1;
    m_bmi.bmiHeader.biBitCount = 32;
    m_bmi.bmiHeader.biCompression = BI_RGB;
    m_bmi.bmiHeader.biSizeImage = 0;
    m_bmi.bmiHeader.biClrUsed = 0;
    m_bmi.bmiHeader.biClrImportant = 0;
    HDC hdc = GetDC(hwnd);
    m_cdc = CreateCompatibleDC(hdc);
    m_hbmp = CreateDIBSection(hdc, &m_bmi, DIB_RGB_COLORS, (void**)&pCanvas, NULL, 0);
    SelectObject(m_cdc, m_hbmp);

    // save canvas.
    memset(pCanvas, 0, width * height * m_bmi.bmiHeader.biBitCount / 8);
    m_canvas.pixels = pCanvas;
    m_canvas.width = width;
    m_canvas.height = height;
    m_canvas.format = ARGB32;
    m_canvas.stride = width * 4;
    m_canvas.bpp = 32;

    loadLayers();

    DeleteDC(hdc);

    return TRUE;
}

void CPainter::loadLayers()
{
    const wchar_t* layerFiles[] = {
        L"C:\\test\\gitRepos\\2p5D\\2p5d\\images\\layered\\0_sky.png",
        L"C:\\test\\gitRepos\\2p5D\\2p5d\\images\\layered\\1_grass.png",
        L"C:\\test\\gitRepos\\2p5D\\2p5d\\images\\layered\\2_trees.png",
        L"C:\\test\\gitRepos\\2p5D\\2p5d\\images\\layered\\3_trees.png",
        L"C:\\test\\gitRepos\\2p5D\\2p5d\\images\\layered\\4_trees.png"
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

void CPainter::Draw()
{
    COORD camShift = { 0, 0 };
    m_blender.blendLayers(m_layers, &m_canvas, camShift);
}

void CBlender::blendSingleLayer(const PixelMap *pLayer, COORD& offset, PixelMap *pDstImg)
{
    Rect dstRect(0, 0, pDstImg->width, pDstImg->height);
    Rect srcRect(offset.X, offset.Y, pLayer->width, pLayer->height);
    dstRect.Intersect(srcRect);
    int srcStartX = 0, srcStartY = 0;
    if (offset.X < 0) {
        srcStartX = -offset.X;
        offset.X = 0;
    }
    if (offset.Y < 0) {
        srcStartY = -offset.Y;
        offset.Y = 0;
    }
    const BYTE *pSrc = pLayer->getPtr(srcStartX, srcStartY);
    BYTE *pDst = pDstImg->getPtr(dstRect.X, dstRect.Y);
    for (int y = dstRect.Y; y < dstRect.GetBottom(); y++) {
        for (int x = dstRect.X; x < dstRect.GetRight(); x++) {
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

}