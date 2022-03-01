#include <windows.h>
#include <windowsx.h>
#include <d3d11_1.h>
#include <tchar.h>
#include "window.h"
#include "graphics.h"

std::shared_ptr<Window> Window::inst;
std::shared_ptr<Graphics> Window::graphics;

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
    switch (msg) {
    case WM_CREATE:
        if (!inst->onCreate(hWnd, graphics))
            exit(0);
        RECT rc;
        GetWindowRect(hWnd, &rc);
        SetCursorPos((rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2);
        ShowCursor(FALSE);
        break;
    case WM_PAINT:
        graphics->render();
        break;
    case WM_KEYDOWN:
    {
        auto vkCode = LOWORD(wParam);
        switch (vkCode) {
        case 0x41: // A
            graphics->setMoveLeft(true);
            break;
        case 0x44: // D
            graphics->setMoveRight(true);
            break;
        case 0x53: // S
            graphics->setMoveBackward(true);
            break;
        case 0x57: // W
            graphics->setMoveForward(true);
            break;
        case VK_SPACE:
            graphics->setMoveUp(true);
            break;
        case 0x43: // C
            graphics->setMoveDown(true);
            break;
        case 0x51: // Q
            PostMessage(hWnd, WM_CLOSE, 0, 0);
            break;
        }
        break;
    }
    case WM_KEYUP:
    {
        auto vkCode = LOWORD(wParam);
        switch (vkCode) {
        case 0x41: // A
            graphics->setMoveLeft(false);
            break;
        case 0x44: // D
            graphics->setMoveRight(false);
            break;
        case 0x53: // S
            graphics->setMoveBackward(false);
            break;
        case 0x57: // W
            graphics->setMoveForward(false);
            break;
        case VK_SPACE:
            graphics->setMoveUp(false);
            break;
        case 0x43: // C
            graphics->setMoveDown(false);
            break;
        }
        break;
    }
    case WM_MOUSEMOVE:
    {
        RECT rc;
        GetClientRect(hWnd, &rc);

        POINT centerClient = { (rc.right + rc.left) / 2, (rc.top + rc.bottom) / 2 };
		int cursorX = GET_X_LPARAM(lParam); 
		int cursorY = GET_Y_LPARAM(lParam);

        POINT centerScreen = centerClient;
        ClientToScreen(hWnd, &centerScreen);
        SetCursorPos(centerScreen.x, centerScreen.y);

        int dx = centerClient.x -  cursorX;
        int dy = centerClient.y - cursorY ;
        if (dx != 0 || dy != 0) {
            graphics->rotate(dx, dy);
        }
        break;
    }
    case WM_SIZE:
    {
        UINT width = LOWORD(lParam);
        UINT height = HIWORD(lParam);
        if (FAILED(graphics->resizeBackbuffer(width, height)))
            MessageBox(nullptr, L"Failed to resize buffer", L"Critical error", MB_OK);
        break;
    }
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

    UpdateWindow(hWnd);
    ShowWindow(hWnd, nCmdShow);

    MSG msg{ 0 };
    
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

std::shared_ptr<Window> Window::window(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow) {
    inst->init(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    return inst;
}
