#include <windows.h>
#include "window.h"
#include "graphics.h"
#include "unit_skybox.h"

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow) {
    auto graphics = Graphics::get();
    graphics->addUnit(std::make_shared<UnitSkybox>());
    auto wnd = Window::window(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
    return 0;
}
