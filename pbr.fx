SamplerState ObjSamplerState;
TextureCube DiffuseIrradianceMap;

//--------------------------------------------------------------------------------------
// Constant Buffer Variables
//--------------------------------------------------------------------------------------
cbuffer PBRConstantBuffer : register(b0)
{
    matrix View;
    matrix Projection;
    // lights
    float4 LightColor[4];
    float4 LightPos[4];
    float4 LightIntensity;
    // camera
    float3 CameraPos;
    // draw mask
    int DrawMask;
}

/*
 * drawMask:
 * 0 -- Full PBR
 * 1 -- D
 * 2 -- F
 * 3 -- G
 */

cbuffer MaterialConstantBuffer : register(b1)
{
    matrix World;
    float3 F0;
    float roughness;
    float metalness;
}


static const float PI = 3.14159f;

//--------------------------------------------------------------------------------------
// Vertex Shader's input and output vertex format
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
    float3 Pos : POSITION;
    float3 Norm : NORMAL;
    float4 Color : COLOR;
};

struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float3 Norm : NORMAL;
    float3 WorldPos: POSITION1;
    float4 Color : COLOR;
};

float3 NN(float3 vec)
{
    float normVec = sqrt(dot(vec, vec));
    return vec / normVec;
}

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    output.Pos = mul(float4(input.Pos, 1.0f), World);
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    output.Color = input.Color;
    output.Norm = normalize(mul(input.Norm, transpose((float3x3)(World))));
    output.WorldPos = mul(float4(input.Pos, 1.0f), World).xyz;

    return output;
}


float pow2(float x)
{
    return x * x;
}

float pow5(float x)
{
    return x * pow2(pow2(x));
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float D(float3 n, float3 h)
{
    float Dval = pow2(roughness) / (PI * pow2(pow2(dot(n, h)) * (pow2(roughness) - 1) + 1));
    return Dval;
}

float Gv(float3 n, float3 vec)
{
    float k = pow2(roughness + 1) / 8;
    float nv = max(0, dot(n, vec));
    float Gval = nv / (nv * (1 - k) + k);
    return Gval;
}

float G(float3 n, float3 v, float3 l)
{
    return Gv(n, v) * Gv(n, l);
}

float3 F(float3 n, float3 v)
{
    float3 F0met = float3(0.04f, 0.04f, 0.04f) * (1 - metalness) + F0 * metalness;
    float3 Fval = F0met + (max(1 - roughness, F0met) - F0met) * pow5(1 - dot(n, v));
    return Fval;
}

float3 ambient(float3 albedo, float3 n, float3 v)
{
    float3 Fval = F(n, v);
    float3 kS = Fval;
    float3 kD = float3(1.0, 1.0, 1.0) - kS;
    kD *= 1.0 - metalness;
    float3 irradiance = DiffuseIrradianceMap.Sample(ObjSamplerState, n).rgb;
    float3 diffuse = irradiance * albedo;
    float3 ambient = kD * diffuse;
    return ambient;
}

float3 fr(float3 albedo, float3 n, float3 v, float3 l)
{
    float3 h = normalize((v + l) * 0.5f);

    float3 Fval = F(n, v);
    float3 Dval = D(n, h);
    float3 Gval = G(n, v, l);

    float3 frval =
        (1 - Fval) * albedo / PI * (1 - metalness) +
        Dval * Fval * Gval / (4 * dot(l, n) * dot(v, n));

    if (DrawMask == 1)
        return Dval;
    if (DrawMask == 2)
        return Fval;
    if (DrawMask == 3)
        return Gval;

    return frval;
}


float4 PS(VS_OUTPUT input) : SV_Target
{
    float3 resultColor = float3(0.0f, 0.0f, 0.0f);
    // direction from point to camera
    float3 v = normalize(CameraPos - input.WorldPos);
    // normal
    float3 n = normalize(input.Norm);

    // point lights
    for (uint i = 0; i < 0; i++) { //(DrawMask == 0 ? 3 : 1)
        // direction from point to light
        float3 l = normalize(LightPos[i].xyz - input.WorldPos);
        // light color
        float4 lightColor = LightColor[i] * LightIntensity[i];
        // result color
        float3 color = fr(input.Color.rgb, n, v, l);
        if (DrawMask == 0)
            color *= lightColor.rgb * max(0, dot(l, n));

        resultColor += color;
    }

    // ambient from irradiance map
    resultColor += ambient(input.Color.rgb, n, v);

    return float4(resultColor, input.Color.a);
}
