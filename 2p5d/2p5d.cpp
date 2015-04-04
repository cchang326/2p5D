// b1 b1 b1 b1 b1 b1 b1
// 2p5d.cpp : Defines the entry point for the application.
//
// master master master

#include "stdafx.h"
#include "2p5d.h"
#include "parallax.h"
#include "windowsx.h"

#define MAX_LOADSTRING 100
#define TIMER_ID 0

#define TRACKER_WIDTH  300
#define TRACKER_HEIGHT 300

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
TCHAR szTrackerWinClass[MAX_LOADSTRING];        // TRACKER WINDOW CLASS NAME
HWND hwndRender = NULL;
HWND hwndTracker = NULL;

// Forward declarations of functions included in this code module:
ATOM				RegisterRenderWinClass(HINSTANCE hInstance);
ATOM				RegisterTrackerWinClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
BOOL				InitTrackerInstance(HINSTANCE, int);
LRESULT CALLBACK	RenderWinProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	TrackerWinProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

Parallax::CPainter g_painter;
static const int g_width = 1280;
static const int g_height = 720;
static const int g_fps = 30;

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_MY2P5D, szWindowClass, MAX_LOADSTRING);
    LoadString(hInstance, IDC_TRACKER, szTrackerWinClass, MAX_LOADSTRING);
	RegisterRenderWinClass(hInstance);
    RegisterTrackerWinClass(hInstance);
		
	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow) || !InitTrackerInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MY2P5D));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: RegisterRenderWinClass()
//
//  PURPOSE: Registers the window class.
//
ATOM RegisterRenderWinClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= RenderWinProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MY2P5D));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_MY2P5D);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//  FUNCTION: RegisterTrackerWinClass()
//
//  PURPOSE: Registers the window class.
//
ATOM RegisterTrackerWinClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = TrackerWinProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MY2P5D));
    wcex.hCursor = LoadCursor(NULL, IDC_CROSS);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szTrackerWinClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   RECT rect = {0, 0, g_width, g_height};
   DWORD stype = (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU);
   AdjustWindowRect(&rect, stype, TRUE);
   int winWidth = rect.right - rect.left;
   int winHeight = rect.bottom - rect.top;

   hWnd = CreateWindow(szWindowClass, szTitle, stype,
                       CW_USEDEFAULT, 0, winWidth, winHeight,
                       NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   g_painter.Init(g_width, g_height, hWnd);
   SetTimer(hWnd, TIMER_ID, 1000 / g_fps, 0);
   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   hwndRender = hWnd;

   return TRUE;
}

BOOL InitTrackerInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd;

    hInst = hInstance; // Store instance handle in our global variable

    RECT rect = { 0, 0, TRACKER_WIDTH, TRACKER_HEIGHT };
    DWORD stype = (WS_CAPTION);
    AdjustWindowRect(&rect, stype, TRUE);
    int winWidth = rect.right - rect.left;
    int winHeight = rect.bottom - rect.top;

    hWnd = CreateWindow(szTrackerWinClass, _T("Tracker"), stype,
        CW_USEDEFAULT, 0, winWidth, winHeight,
        NULL, NULL, hInstance, NULL);

    if (!hWnd) {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    hwndTracker = hWnd;

    return TRUE;
}

//
//  FUNCTION: RenderWinProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK RenderWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		{
            BitBlt(hdc, 0, 0, g_width, g_height, g_painter.m_cdc, 0, 0, SRCCOPY);
		}   

		EndPaint(hWnd, &ps);
		break;

    case WM_ERASEBKGND:
        // This repaints the whole client background, which may cause flickering.
        // Ignore the message since we're painting the whole client area anyway.
        break;
        
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
    case WM_TIMER:
        {
            g_painter.Draw();
            RECT rect = { 0, 0, g_width, g_height };
            InvalidateRect(hWnd, &rect, TRUE);
        }
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// todo: attach tracker window to render window.
// use SetWindowPos and GetWindowRect.
void AttachTrackerToRender()
{
    RECT rectRender = { 0 };
}

LRESULT CALLBACK TrackerWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message) {
    case WM_CREATE:

        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        
        EndPaint(hWnd, &ps);
        break;

    case WM_LBUTTONDOWN:
    case WM_MOUSEMOVE:
        if (wParam == MK_LBUTTON) 
        {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            g_painter.updateCameraPos(x - TRACKER_WIDTH / 2, y - TRACKER_HEIGHT / 2);        
        }
        break;

    case WM_LBUTTONUP:
        g_painter.updateCameraPos(0, 0);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
