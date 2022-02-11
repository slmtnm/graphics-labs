#include <windows.h>
#include <d3d11_1.h>
#include <tchar.h>
#include "window.h"
#include "graphics.h"

std::shared_ptr<Window> Window::inst;

bool Window::onCreate(HWND hWnd, std::shared_ptr<Graphics>& graphics) {
    // init directx
    graphics = Graphics::init(hWnd);
    if (!graphics) {
        MessageBox(NULL,
            _T("Could not initialize DirectX"),
            _T("graphics-labs"),
            NULL);
        return false;
    }
    return true;
}

LRESULT CALLBACK Window::WndProc(
    _In_ HWND hWnd,
    _In_ UINT msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam) {
    PAINTSTRUCT ps;
    HDC hdc;
    static std::shared_ptr<Graphics> graphics;

    switch (msg) {
    case WM_CREATE:
        if (!inst->onCreate(hWnd, graphics))
            return 1;
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        graphics->render();
        EndPaint(hWnd, &ps);
        break;
    case WM_CLOSE:
        graphics->cleanup();
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

int Window::init(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow) {
    const TCHAR szWindowClass[] = _T("graphics-labs");

    WNDCLASSEX wc{ 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = Window::WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = szWindowClass;
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

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

    MSG msg{ 0 };
    while (GetMessage(&msg, NULL, 0, 0))
    {
        /* Translate virtual-key msg into character msg */
        TranslateMessage(&msg);
        /* Send message to WindowProcedure */
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}

std::shared_ptr<Window> Window::window(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    inst->init(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    return inst;
}