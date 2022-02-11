#pragma once

#include <memory>

class Graphics;

class Window
{
public:
    static std::shared_ptr<Window> window(_In_ HINSTANCE hInstance,
        _In_opt_ HINSTANCE hPrevInstance,
        _In_ LPSTR lpCmdLine,
        _In_ int nCmdShow);
private:
    Window() {}

    int init(_In_ HINSTANCE hInstance,
        _In_opt_ HINSTANCE hPrevInstance,
        _In_ LPSTR lpCmdLine,
        _In_ int nCmdShow);

    bool onCreate(HWND hWnd, std::shared_ptr<Graphics>& graphics);

    static LRESULT CALLBACK WndProc(
        _In_ HWND hWnd,
        _In_ UINT msg,
        _In_ WPARAM wParam,
        _In_ LPARAM lParam);

    Window(Window const&) = delete;
    Window& operator=(Window const&) = delete;

    static std::shared_ptr<Window> inst;
};

