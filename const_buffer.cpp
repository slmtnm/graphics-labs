#include <d3d11.h>
#include "const_buffer.h"
#include "graphics.h"


void AppliedConstBuffer::apply(UINT cbufRegister, bool boundVS, bool boundPS) const
{
    if (status)
    {
        auto ctx = Graphics::get()->getContext();
        if (boundVS)
            ctx->VSSetConstantBuffers(cbufRegister, 1, &constBuffer);
        if (boundPS)
            ctx->PSSetConstantBuffers(cbufRegister, 1, &constBuffer);
    }
}
