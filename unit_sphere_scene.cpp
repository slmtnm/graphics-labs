#include <windows.h>
#include "unit_sphere_scene.h"
#include "primitive.h"
#include "primitive_samples.h"

bool UnitSphereScene::init(std::shared_ptr<Graphics>)
{
	return PrimitiveSample::createSphere(spherePrim, radius);
}

void UnitSphereScene::prepareForRender(std::shared_ptr<Graphics>)
{
}

void UnitSphereScene::render(std::shared_ptr<Graphics> graphics)
{
    Graphics::MaterialConstantBuffer mtlCB;
    ZeroMemory(&mtlCB, sizeof(Graphics::MaterialConstantBuffer));
    mtlCB.roughness = 0.5f;
    mtlCB.metalness = 0.1f;
    mtlCB.F0 = XMFLOAT3(0.95f, 0.64f, 0.54f);

    graphics->startEvent(L"DrawSphereGrid");

    const int GridSize = 8;

    auto mtlCbuf = graphics->getMaterialCbuf();

    for (int y = -GridSize / 2; y < GridSize / 2; y++)
    {
        mtlCB.metalness = 0.01f + (y + GridSize / 2) * (1 - 0.01f) / (GridSize - 1);
        for (int x = -GridSize / 2; x < GridSize / 2; x++)
        {
            mtlCB.roughness = 0.01f + (x + GridSize / 2) * (1 - 0.01f) / (GridSize - 1);
            mtlCB.World = XMMatrixTranspose(XMMatrixTranslation(3 * x * radius, 3 * y * radius, 30.0f));

            mtlCbuf->update(mtlCB);

            spherePrim->render(graphics->getPBRShader(), graphics->getSamplerState(), graphics->getIrradianceCubeSRV());
        }
    }
    graphics->endEvent();
}

void UnitSphereScene::cleanup(std::shared_ptr<Graphics>)
{
	spherePrim->cleanup();
}
