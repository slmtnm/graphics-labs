Texture2D tex : register(t0);
SamplerState samLinear : register(s0);

cbuffer TonemapConstantBuffer : register(b0)
{
    int isBrightnessWindow;
    float meanBrightness;
}


struct VS_INPUT
{
    float3 Pos : POSITION;
    float4 Color : COLOR0;
    float2 Tex : TEXCOORD0;
};


struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR0;
    float2 Tex : TEXCOORD0;
};


/* Tone mapping */
static const float A = 0.1;  // Shoulder Strength
static const float B = 0.50; // Linear Strength
static const float C = 0.1;  // Linear Angle
static const float D = 0.20; // Toe Strength
static const float E = 0.02; // Toe Numerator
static const float F = 0.30; // Toe Denominator
                             // Note: E/F = Toe Angle
static const float W = 11.2; // Linear White Point Value


float EvalExposure()
{
    float keyValue =
        1.03f - 1.5f / (2.0f + log(meanBrightness + 1) / log(10));

    float exposure = keyValue / meanBrightness;

    return exposure;
}


float3 Uncharted2Tonemap(float3 x)
{
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

float3 TonemapFilmic(float3 color)
{
    float exposure = EvalExposure();
    float3 curr = Uncharted2Tonemap(exposure * color);
    float3 whiteScale = 1.0f / Uncharted2Tonemap(W);
    return curr * whiteScale;
}


//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    output.Pos = float4(input.Pos, 1.0f);
    output.Color = input.Color;
    output.Tex = input.Tex;
    return output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(VS_OUTPUT input) : SV_TARGET
{
    float4 color = tex.Sample(samLinear, input.Tex);
    float3 resultColor = TonemapFilmic(float3(color[0], color[1], color[2]));


    return isBrightnessWindow ?
        float4(exp(color[0]) - 1.0f, exp(color[1]) - 1.0f, exp(color[2]) - 1.0f, 1.0f) :
        float4(resultColor, color[3]);
}
