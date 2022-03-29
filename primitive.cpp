#include "primitive.h"


void Primitive::cleanup()
{
    if (vertexBuffer) vertexBuffer->Release();
    if (indexBuffer) indexBuffer->Release();
}

void Primitive::render(
    Shader const& shader, 
    std::vector<std::shared_ptr<AppliedConstBuffer>> const& constBuffers,
    std::vector<UINT> cbufRegister,
    ID3D11SamplerState* samplerState, ID3D11ShaderResourceView* tex)
{
    shader.apply();

    for (size_t idx = 0; idx < constBuffers.size(); idx++)
        // let all buffers be binded to both VS and PS for now
        constBuffers[idx]->apply(cbufRegister[idx], true, true);

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
