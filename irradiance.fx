SamplerState ObjSamplerState;
TextureCube Cubemap;

cbuffer SimpleConstantBuffer : register(b0)
{
    matrix World;
    matrix View;
    matrix Projection;
};

struct VS_INPUT
{
    float3 Pos : POSITION;
    float3 Norm: NORMAL;
    float4 Color : COLOR0;
};


struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float3 Norm: NORMAL;
    float3 TexCoord : POSITION;
};


VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;

    output.Pos = mul(float4(input.Pos, 1.0f), World);
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);

    output.TexCoord = input.Pos;

    return output;
}


float4 PS(VS_OUTPUT input) : SV_Target
{
    const int N1 = 200, N2 = 50;
    const float PI = 3.14159265359f;

    float3 irradiance = float3(0.0, 0.0, 0.0);
    for (int i = 0; i < N1; i++)
    {
        for (int j = 0; j < N2; j++)
        {
            float phi = i * (2 * PI / N1);
            float theta = j * (PI / 2 / N2);

            float3 normal = normalize(input.TexCoord);
            float3 dir = abs(normal.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
            float3 tangent = normalize(cross(dir, normal));
            float3 binormal = cross(normal, tangent);

            // Перевод сферических координат в декартовы (в касательном пространстве)
            float3 tangentSample = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            // ... и из касательного пространства в мировое
            float3 sampleVec = tangentSample.x * tangent + tangentSample.y * binormal + tangentSample.z * normal;
            irradiance += Cubemap.Sample(ObjSamplerState, sampleVec).rgb * cos(theta) * sin(theta);
        }
    }
    irradiance = PI * irradiance / (N1 * N2);

    return float4(irradiance, 1.0f);
}
