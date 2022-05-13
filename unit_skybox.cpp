#include <d3d11_1.h>
#include "DDSTextureLoader.h"

#include "unit_skybox.h"
#include "primitive.h"
#include "primitive_samples.h"
#include "graphics.h"


bool UnitSkybox::init(std::shared_ptr<Graphics> graphics)
{
    bool success = PrimitiveSample::createCube(skyboxPrim);
    if (!success)
        return false;


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
