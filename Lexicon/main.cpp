#include <windows.h>
#include <windowsx.h>
#include <wchar.h>
#include <iostream>
#include <gdiplus.h>
#include <atlimage.h>
#include <MemoryBuffer.h>
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Imaging.h>
using namespace Gdiplus;
using namespace winrt;
using namespace winrt::Windows::Graphics::Imaging;
using namespace winrt::Windows::Foundation;

HWND overlayHwnd = nullptr;
bool drag = false;
POINT startPoint = {};
POINT endPoint = {};
POINT startRegion = {};
POINT endRegion = {};
bool showRegion = true;

void bitmapToPng(HBITMAP hBitmap, const wchar_t* path) {
	CImage image;
	image.Attach(hBitmap);
	image.Save(path, ImageFormatPNG);
}

SoftwareBitmap hbitmapToSoftwareBitmap(HBITMAP hBitmap) {
	BITMAP bmp;
	GetObject(hBitmap, sizeof(BITMAP), &bmp);
	int width = bmp.bmWidth;
	int height = bmp.bmHeight;
	BITMAPINFOHEADER bi;
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = width;
	bi.biHeight = -height;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	std::vector<BYTE> pixels(width * height * 4);
	HDC hdc = GetDC(nullptr);
	GetDIBits(hdc, hBitmap, 0, height, pixels.data(), (BITMAPINFO*)&bi, DIB_RGB_COLORS);
	ReleaseDC(nullptr, hdc);
	SoftwareBitmap softwareBitmap(BitmapPixelFormat::Bgra8, width, height, BitmapAlphaMode::Premultiplied);
	BitmapBuffer buffer = softwareBitmap.LockBuffer(BitmapBufferAccessMode::Write);
	IMemoryBufferReference reference = buffer.CreateReference();
	auto spByteAccess = reference.as<::Windows::Foundation::IMemoryBufferByteAccess>();
	BYTE* data = nullptr;
	UINT32 capacity = 0;
	spByteAccess->GetBuffer(&data, &capacity);
	memcpy(data, pixels.data(), pixels.size());
	return softwareBitmap;
}

HBITMAP CaptureScreenRegion() {
	int width = endRegion.x - startRegion.x;
	int height = endRegion.y - startRegion.y;
	HDC hdcScreen = GetDC(nullptr);
	HDC hdcMem = CreateCompatibleDC(hdcScreen);
	HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
	SelectObject(hdcMem, hBitmap);
	BitBlt(hdcMem, 0, 0, width, height, hdcScreen, startRegion.x, startRegion.y, SRCCOPY);
	DeleteDC(hdcMem);
	ReleaseDC(nullptr, hdcScreen);
	return hBitmap;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
			case 1:
				ShowWindow(overlayHwnd, SW_SHOW);
				SetForegroundWindow(overlayHwnd);
                break;
			case 2:
				MessageBox(hwnd, L"Translation Paused", L"Info", MB_OK);
				break;
			case 3:
				MessageBox(hwnd, L"Settings", L"Info", MB_OK);
				break;
		}
		return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
LRESULT CALLBACK OverlayWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_LBUTTONDOWN:
		drag = true;
		startPoint.x = GET_X_LPARAM(lParam);
		startPoint.y = GET_Y_LPARAM(lParam);
		endPoint.x = GET_X_LPARAM(lParam);
		endPoint.y = GET_Y_LPARAM(lParam);
		return 0;
	case WM_MOUSEMOVE:
		if (drag) {
			HDC hdc = GetDC(hwnd);
			HPEN pen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
			HGDIOBJ oldPen = SelectObject(hdc, pen);
			HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
			int oldROP2 = SetROP2(hdc, R2_NOTXORPEN);
			Rectangle(hdc, startPoint.x, startPoint.y, endPoint.x, endPoint.y);
			endPoint.x = GET_X_LPARAM(lParam);
			endPoint.y = GET_Y_LPARAM(lParam);
			Rectangle(hdc, startPoint.x, startPoint.y, endPoint.x, endPoint.y);
			SetROP2(hdc, oldROP2);
			SelectObject(hdc, oldPen);
			SelectObject(hdc, oldBrush);
			DeleteObject(pen);
			ReleaseDC(hwnd, hdc);
		}
		return 0;
	case WM_LBUTTONUP:
		if (drag) {
			HDC hdc = GetDC(hwnd);
			HPEN pen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
			HGDIOBJ oldPen = SelectObject(hdc, pen);
			HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
			int oldROP2 = SetROP2(hdc, R2_NOTXORPEN);
			Rectangle(hdc, startPoint.x, startPoint.y, endPoint.x, endPoint.y);
			drag = false;
			ShowWindow(hwnd, SW_HIDE);
			startRegion.x = min(startPoint.x, endPoint.x);
			startRegion.y = min(startPoint.y, endPoint.y);
			endRegion.x = max(startPoint.x, endPoint.x);
			endRegion.y = max(startPoint.y, endPoint.y);
			startPoint = {};
			endPoint = {};
			wchar_t buf[128];
			swprintf_s(buf, L"Region Selected (%ld,%ld) to (%ld,%ld)", startRegion.x, startRegion.y, endRegion.x, endRegion.y);
			MessageBox(hwnd, buf, L"Info", MB_OK);
			bitmapToPng(CaptureScreenRegion(), L"region_capture.png");
			SoftwareBitmap softwareBitmap = hbitmapToSoftwareBitmap(CaptureScreenRegion());
		}
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
	init_apartment();
	ULONG_PTR gdiplusToken;
	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
    const wchar_t kClassName[] = L"LexiconWindow";
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = kClassName;
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED,
        kClassName,
        L"Lexicon",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );
	HWND hwndRegionButton = CreateWindow(
		L"BUTTON",
		L"Select OCR Region",
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		10, 10, 100, 50,
		hwnd,
		(HMENU)1,
		hInstance,
		nullptr
	);
    HWND hwndPauseButton = CreateWindow(
        L"BUTTON",
        L"Pause",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        110, 10, 100, 50,
        hwnd,
        (HMENU)2,
        hInstance,
        nullptr
    );
    HWND hwndSettingButton = CreateWindow(
        L"BUTTON",
        L"Settings",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        210, 10, 100, 50,
        hwnd,
        (HMENU)3,
        hInstance,
        nullptr
    );
	const wchar_t kOverlayClassName[] = L"LexiconOverlayWindow";
	WNDCLASSEX overlayWc = {};
	overlayWc.cbSize = sizeof(WNDCLASSEX);
	overlayWc.lpfnWndProc = OverlayWindowProc;
	overlayWc.hInstance = hInstance;
	overlayWc.lpszClassName = kOverlayClassName;
    overlayWc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
	RegisterClassEx(&overlayWc);
    overlayHwnd = CreateWindowEx(
		WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
		kOverlayClassName,
		L"Overlay",
		WS_POPUP,
		0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );
    SetLayeredWindowAttributes(hwnd, RGB(0, 192, 255), 200, LWA_ALPHA);
	SetLayeredWindowAttributes(overlayHwnd, 0, 90, LWA_ALPHA);
    ShowWindow(hwnd, SW_SHOW);
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
	GdiplusShutdown(gdiplusToken);
    return 0;
}

