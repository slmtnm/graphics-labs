#include <windows.h>
#include <windowsx.h>
#include <d3d11_1.h>
#include <tchar.h>
#include "imgui_impl_win32.h"
#include "window.h"
#include "graphics.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


std::shared_ptr<Window> Window::inst(new Window);
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

    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_CREATE:
        if (!inst->onCreate(hWnd, graphics))
            exit(0);
        RECT rc;
        GetWindowRect(hWnd, &rc);
        inst->cursorX = (rc.left + rc.right) / 2;
        inst->cursorY = (rc.top + rc.bottom) / 2;
        SetCursorPos(inst->cursorX, inst->cursorY);
        ShowCursor(TRUE);
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
        case 0x30: // 0
            graphics->resetLightIntensity(0);
            graphics->resetLightIntensity(1);
            graphics->resetLightIntensity(2);
            break;
        case 0x31: // 1
            graphics->increaseLightIntensity(0);
            break;
        case 0x32: // 2
            graphics->decreaseLightIntensity(0);
            break;
        case 0x33: // 3
            graphics->increaseLightIntensity(1);
            break;
        case 0x34: // 4
            graphics->decreaseLightIntensity(1);
            break;
        case 0x35: // 5
            graphics->increaseLightIntensity(2);
            break;
        case 0x36: // 6
            graphics->decreaseLightIntensity(2);
            break;
        case VK_OEM_PLUS: // +
            graphics->increaseLightIntensity(0);
            graphics->increaseLightIntensity(1);
            graphics->increaseLightIntensity(2);
            break;
        case VK_OEM_MINUS: // -
            graphics->decreaseLightIntensity(0);
            graphics->decreaseLightIntensity(1);
            graphics->decreaseLightIntensity(2);
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
    case WM_RBUTTONDOWN:
        inst->cursorX = GET_X_LPARAM(lParam);
        inst->cursorY = GET_Y_LPARAM(lParam);
        break;
    case WM_MOUSEMOVE:
    {
        if (inst == nullptr)
            break;

        if (!(wParam & MK_RBUTTON))
            break;

        RECT rc;
        GetClientRect(hWnd, &rc);

        int newCursorX = GET_X_LPARAM(lParam);
        int newCursorY = GET_Y_LPARAM(lParam);

        int dx, dy;

        if (inst->first) {
            dx = dy = 0;
            inst->first = false;
        }
        else {
            dx = newCursorX - inst->cursorX;
            dy = newCursorY - inst->cursorY;

            graphics->rotate(dx, dy);
        }

        inst->cursorX = newCursorX;
        inst->cursorY = newCursorY;

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
