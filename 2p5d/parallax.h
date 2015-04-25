#include "stdafx.h"

#define ASSERT assert

namespace Parallax  {

enum ColorFormat {
    UNKNOWN = 0,
    GRAY8,
    I420,
    IYUV  = I420,
    ARGB32,
    RGB24,
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
        return pixels + y * stride + x * (bpp >> 3);
    }

    void invert()
    {
        pixels = getPtr(0, height - 1);
        stride = -stride;
    }

    bool isInverted() const
    {
        return stride < 0;
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

typedef std::function<void(int x, int y)> SetCameraOffsetFunc;

class CBlender {
public:
    CBlender():
        m_strengthX(1), m_strengthY(1)
    {}

    float getPixelShift1D(int depth, int camShift)
    {
        static const int cameraViewDistance = 1; // distance from camera to view plane.
        return ((float)depth / (depth + cameraViewDistance)) * camShift;
    }

    COORD getShift(int depth, const COORD& cameraShift)
    {
        COORD shift = {0};
        shift.X = (int)getPixelShift1D(depth, cameraShift.X) * m_strengthX;
        shift.Y = (int)getPixelShift1D(depth, cameraShift.Y) * m_strengthY;
        return shift;
    }

    void blendSingleLayer(const PixelMap *pLayer, COORD& offset, PixelMap *pDst);
    void blendLayers(std::vector<LayerImage>& layers, PixelMap *pDstImg, const COORD& cameraShift);

private:
    uint32_t m_strengthX;
    uint32_t m_strengthY;
};

class CPainter {
public:
	CPainter():
		m_cdc(NULL),
		m_hbmp(NULL)
	{
        memset(&m_canvas, 0, sizeof(m_canvas));
        memset(&m_cameraShift, 0, sizeof(m_cameraShift));
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

    const SetCameraOffsetFunc createCamPosUpdateCallbackFunc();

    BOOL Init(DWORD width, DWORD height, HWND hwnd);

    void updateCameraPos(int shiftX, int shiftY);

    void Draw();

private:
    void loadLayers();
	
public:
	HDC m_cdc;
    	
private:
    HBITMAP m_hbmp;
	PixelMap m_canvas;
    std::vector<GdiPlusBmp> m_bitmaps;
    std::vector<LayerImage> m_layers;
    ULONG_PTR m_gdiplusToken;
    COORD m_cameraShift;

    CBlender m_blender;
};

class CTracker {
public:
    CTracker() :
        m_hBmp(NULL), m_dc(NULL), m_camOffsetCallback(nullptr),
        m_width(0), m_height(0), m_format(UNKNOWN),
        m_bpp(0), m_hwnd(0), m_pFaceDetector(NULL)
    {}
    ~CTracker();

    void SetCameraOffsetCallBackFunc(SetCameraOffsetFunc func)
    {
        m_camOffsetCallback = func;
    }

    HRESULT initialize(int width, int height, HWND hWnd);
    HRESULT start();
    HRESULT stop();

private:
    const SampleProcessFunc createSampleProcessCallbackFunc();
    void detectFace(const BYTE * pSampleBuffer, DWORD dwSampleSize);

public:
    HDC m_dc;
private:
    // parallax cam offset
    SetCameraOffsetFunc m_camOffsetCallback;
    mutable std::mutex m_stateDataMutex;
    
    // webcam preview
    HBITMAP m_hBmp;
    PixelMap m_map;
    HWND m_hwnd;
    
    // capture.
    CMFCamCapture m_capture;
    DWORD m_width;
    DWORD m_height;
    ColorFormat m_format;
    DWORD m_bpp;

    // face detector
    CFaceDetector* m_pFaceDetector;
};

}