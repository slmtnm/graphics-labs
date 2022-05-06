#include <d3d11_1.h>
#include "DDSTextureLoader.h"

#include "unit_skybox.h"
#include "primitive.h"
#include "primitive_samples.h"
#include "graphics.h"


bool UnitSkybox::init(std::shared_ptr<Graphics> graphics)
{
    bool success = PrimitiveSample::createSphere(skyboxPrim, 100.0f, true);
    if (!success)
        return false;

    ID3D11Texture2D* skyboxTex = nullptr;

    auto hr = CreateDDSTextureFromFileEx(graphics->getDevice(), L"skymap.dds",
        0, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0,
        D3D11_RESOURCE_MISC_TEXTURECUBE, false,
        (ID3D11Resource**)&skyboxTex, &skyboxSRV);

    if (FAILED(hr))
        return false;

    if (skyboxTex)
        skyboxTex->Release();

    // Define the input layout
    D3D11_INPUT_ELEMENT_DESC simpleLayout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    skyboxShader = ShaderFactory::makeShaders(L"skybox.fx", simpleLayout, 3);
    skyboxShader->addConstBuffers(
        {
            { graphics->getSimpleCbuf()->appliedConstBuffer(), true, false}
        });

    return true;
}

void UnitSkybox::prepareForRender(std::shared_ptr<Graphics>)
{
}

void UnitSkybox::render(std::shared_ptr<Graphics> graphics)
{
    // render skybox
    graphics->startEvent(L"DrawSkybox");
    Graphics::SimpleConstantBuffer simpleCB;
    auto& camera = graphics->getCamera();
    auto pos = camera.getPosition().m128_f32;
    simpleCB.mWorld = XMMatrixTranspose(XMMatrixTranslation(pos[0], pos[1], pos[2]));
    simpleCB.mView = XMMatrixTranspose(camera.view());
    simpleCB.mProjection = XMMatrixTranspose(camera.projection());
    graphics->getSimpleCbuf()->update(simpleCB);
    skyboxPrim->render(skyboxShader, graphics->getSamplerState(), skyboxSRV);
    graphics->endEvent();
}

void UnitSkybox::cleanup(std::shared_ptr<Graphics>)
{
    skyboxPrim->cleanup();
    skyboxShader->cleanup();
    if (skyboxSRV) skyboxSRV->Release();
}
