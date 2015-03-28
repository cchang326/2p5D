#pragma once

#include "stdafx.h"

namespace Parallax  {

enum ColorFormat {
    GRAY8,
    ARGB32,
};

struct PixelMap {
    std::vector<BYTE> pixels;
    int width;
    int height;
    int stride;
    int bpp;
    ColorFormat format;
    
    void allocate()
    {
        pixels.assign(width * height * bpp / 8, 0);
    }
};

struct DepthImage {
    int depth; // todo: extend to depth map.
    PixelMap *pAlphaMask;
    PixelMap *pImg;    
};

bool compareDepth(DepthImage *p1, DepthImage *p2);

class CParralaxor {
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
    }

    void blendLayer(const PixelMap *pImg, const PixelMap *pAlphaMask, const COORD& offset, PixelMap *pDst)
    {
    }

    void renderLayers(std::vector<DepthImage*>& layers, PixelMap *pDstImg, const COORD& cameraShift)
    {
        // sort from back to front.
        std::sort(layers.begin(), layers.end(), compareDepth);
        for (const DepthImage* pDepthImg : layers) {
            COORD shift = getShift(pDepthImg->depth, cameraShift);
            blendLayer(pDepthImg->pImg, pDepthImg->pAlphaMask, shift, pDstImg);
        }
    }
};

class CPainter {
public:
	CPainter():
		m_pCanvas(NULL),
		m_cdc(NULL),
		m_hbmp(NULL)
	{;}

	~CPainter(){
		DeleteDC(m_cdc);
		DeleteObject(m_hbmp);
	}

    void loadLayers()
    {
        // todo: load layers here.
        Bitmap bmp(L"images\\layered\\background.jpg", FALSE);
    }

	BOOL Init(DWORD width, DWORD height, HWND hwnd)
	{
		m_dwHeight = height;
		m_dwWidth = width;
		
		m_bmi.bmiHeader.biSize = sizeof (m_bmi.bmiHeader);
		m_bmi.bmiHeader.biWidth = m_dwWidth;
		m_bmi.bmiHeader.biHeight = m_dwHeight;
		m_bmi.bmiHeader.biPlanes = 1;
		m_bmi.bmiHeader.biBitCount = 32;
		m_bmi.bmiHeader.biCompression = BI_RGB;
		m_bmi.bmiHeader.biSizeImage = 0;
		m_bmi.bmiHeader.biClrUsed = 0;
		m_bmi.bmiHeader.biClrImportant = 0;
		HDC hdc = GetDC(hwnd);
		m_cdc = CreateCompatibleDC (hdc); 
		m_hbmp = CreateDIBSection(hdc, &m_bmi, DIB_RGB_COLORS, (void**)&m_pCanvas, NULL, 0);
        SelectObject(m_cdc, m_hbmp);

		m_bValue = 0;
		memset (m_pCanvas, m_bValue, m_dwWidth * m_dwHeight * m_bmi.bmiHeader.biBitCount/8);
		DeleteDC(hdc);

        loadLayers();
                
		return TRUE;
	}    

	void Draw ()
	{
		m_bValue += 2;
		memset(m_pCanvas, m_bValue, m_dwHeight * m_dwWidth * m_bmi.bmiHeader.biBitCount/8);		
	}
	
public:
	DWORD m_dwWidth;
	DWORD m_dwHeight;
	HDC m_cdc;
    	
private:
    BITMAPINFO m_bmi;
    HBITMAP m_hbmp;
	BYTE m_bValue;
	BYTE *m_pCanvas;
    std::vector<DepthImage*> m_layers;
};

};