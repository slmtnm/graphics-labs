cbuffer ExposureConstantBuffer : register(b0)
{
    float meanBrightness;
    float _dummy[15];
}

//--------------------------------------------------------------------------------------

struct VS_INPUT
{
    float4 Pos : POSITION;
    float4 Color : COLOR0;
};


struct VS_OUTPUT
{
    float4 Pos : SV_POSITION;
    float4 Color : COLOR0;
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
        1.03f - 2.0f / (2.0f + log(meanBrightness + 1) / log(10));

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


VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT)0;
    output.Pos = input.Pos;
    output.Color = input.Color;
    return output;
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS(VS_OUTPUT input) : SV_TARGET
{
    float4 color = input.Color;
    float3 resultColor = TonemapFilmic(float3(color[0], color[1], color[2]));

    return float4(resultColor, color[3]);
}
