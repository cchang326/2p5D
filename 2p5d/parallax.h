#pragma once

#include "stdafx.h"

namespace Parallax  {

enum ColorFormat {
    GRAY8,
    ARGB32,
};

struct PixelMap {
    BYTE* pixels;
    int width;
    int height;
    int stride;
    int bpp;
    ColorFormat format;
    
    BYTE* getPtr(int x, int y) const
    {
        return pixels + y * stride + x;
    }

    void invert()
    {
        pixels = getPtr(0, height - 1);
        stride = -stride;
    }
};

struct LayerImage {
    int depth; // todo: extend to depth map.
    //PixelMap *pAlphaMask;
    PixelMap img;    
};

struct GdiPlusBmp {
    Bitmap *pBitmap;
    BitmapData bmpData;
};

class CBlender {
public:
    float getPixelShift1D(int depth, int camShift)
    {
        static const int cameraViewDistance = 1; // distance from camera to view plane.
        return ((float)depth / (depth + cameraViewDistance)) * camShift;
    }

    COORD getShift(int depth, const COORD& cameraShift)
    {
        COORD shift = {0};
        shift.X = (int)getPixelShift1D(depth, cameraShift.X);
        shift.Y = (int)getPixelShift1D(depth, cameraShift.Y);
        return shift;
    }

    void blendSingleLayer(const PixelMap *pLayer, COORD& offset, PixelMap *pDst);
    void blendLayers(std::vector<LayerImage>& layers, PixelMap *pDstImg, const COORD& cameraShift);
};

class CPainter {
public:
	CPainter():
		m_cdc(NULL),
		m_hbmp(NULL)
	{
        memset(&m_canvas, 0, sizeof(m_canvas));
        // Initialize GDI+.
        GdiplusStartupInput gdiplusStartupInput;        
        GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);       
    }

	~CPainter(){
		DeleteDC(m_cdc);
		DeleteObject(m_hbmp);
        for (size_t i = 0; i < m_bitmaps.size(); i++) {
            m_bitmaps[i].pBitmap->UnlockBits(&m_bitmaps[i].bmpData);
            delete m_bitmaps[i].pBitmap;
            m_bitmaps[i].pBitmap = NULL;
        }
        m_bitmaps.clear();
        m_layers.clear();
        GdiplusShutdown(m_gdiplusToken);
	}

    void loadLayers();

    BOOL Init(DWORD width, DWORD height, HWND hwnd);

    void Draw();
	
public:
	HDC m_cdc;
    	
private:
    BITMAPINFO m_bmi;
    HBITMAP m_hbmp;
	PixelMap m_canvas;
    std::vector<GdiPlusBmp> m_bitmaps;
    std::vector<LayerImage> m_layers;
    ULONG_PTR m_gdiplusToken;

    CBlender m_blender;
};

}