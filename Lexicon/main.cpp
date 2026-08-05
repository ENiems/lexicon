#include <windows.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
			case 1:
				MessageBox(hwnd, L"Select Region", L"Info", MB_OK);
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

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
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
    SetLayeredWindowAttributes(hwnd, RGB(0, 192, 255), 200, LWA_ALPHA);
    ShowWindow(hwnd, SW_SHOW);
    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

