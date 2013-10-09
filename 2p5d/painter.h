#include "stdafx.h"

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

		m_bValue = 0;
		memset (m_pCanvas, m_bValue, m_dwWidth * m_dwHeight * m_bmi.bmiHeader.biBitCount/8);
		DeleteDC(hdc);

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
	BITMAPINFO m_bmi;
	HDC m_cdc;
	HBITMAP m_hbmp;
	
private:
	BYTE m_bValue;
	BYTE *m_pCanvas;
};
