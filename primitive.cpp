#include "primitive.h"


void Primitive::cleanup()
{
    if (vertexBuffer) vertexBuffer->Release();
    if (indexBuffer) indexBuffer->Release();
}

void Primitive::render(
    std::unique_ptr<Shader> const& shader, ID3D11SamplerState* samplerState, ID3D11ShaderResourceView* tex)
{
    shader->apply();

    auto ctx = graphics->getContext();
    ctx->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    ctx->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R32_UINT, 0);

    // Set primitive topology
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    if (tex && samplerState)
    {
        // Set the sampler state in the pixel shader.
        ctx->PSSetSamplers(0, 1, &samplerState);
        ctx->PSSetShaderResources(0, 1, &tex);
    }
    ctx->DrawIndexed(iCount, 0, 0);
}
