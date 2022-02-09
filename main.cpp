#include <windows.h>
#include <tchar.h>
#include <cstdio>
#include <vector>
#include <optional>
#include <d3d11_1.h>
#include <directxcolors.h>

#include "graphics.h"

LRESULT CALLBACK WndProc(
    _In_ HWND hWnd,
    _In_ UINT msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
) {
    PAINTSTRUCT ps;
    HDC hdc;

    switch(msg) {
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
        break;
    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
     default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}
 
int WINAPI WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nCmdShow
) {
    const TCHAR szWindowClass[] = _T("graphics-labs");

	WNDCLASSEX wc{0};
	wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = 0;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = szWindowClass;
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL,
            _T("Call to RegisterClassEx failed!"),
            _T("graphics-labs"),
            NULL);
        return 1;
   }

    const TCHAR szTitle[] = _T("graphics-labs");
    const unsigned int WIDTH = 800;
    const unsigned int HEIGHT = 800;
    HWND hWnd = CreateWindowEx(
        WS_EX_OVERLAPPEDWINDOW,
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WIDTH, HEIGHT,
        NULL,
        NULL,
        hInstance,
        NULL
    );
    
    if (!hWnd) {
        MessageBox(NULL,
            _T("Call to CreateWindow failed!"),
            _T("graphics-labs"),
            NULL);
        return 1;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // init directx
    auto graphics = Graphics::init(hWnd);
    if (!graphics.has_value()) {
        MessageBox(NULL,
            _T("Could not initialize DirectX"),
            _T("graphics-labs"),
            NULL);
        return 1;
    }

    MSG msg{0};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            graphics.value().render();
        }
    }

    return (int)msg.wParam;
}
