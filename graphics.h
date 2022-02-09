#pragma once

#include <optional>
#include <vector>
#include <d3d11_1.h>

class Graphics {
private:
    ID3D11Device *device = nullptr;
    ID3D11Device1 *device1 = nullptr;
	ID3D11DeviceContext *context = nullptr;
	ID3D11DeviceContext1 *context1 = nullptr;
    IDXGISwapChain *swapChain = nullptr;
	IDXGISwapChain1 *swapChain1 = nullptr;
	ID3D11RenderTargetView *renderTargetView = nullptr;

public:
	// factory method
	static std::optional<Graphics> init(HWND hWnd);

	// render the frame
	void render();

	// cleanup all d3d objects
	void cleanup();
};

