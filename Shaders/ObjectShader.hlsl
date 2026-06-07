cbuffer ConstantBuffer : register(b0)
{
    matrix world;
    matrix view;
    matrix projection;
    float4 baseColor;
    float4 ambientColor;
    float4 lightDir;
    float4 lightColor;
    float4 cameraPos;
    float roughness;
    float metallic;
    float2 padding;
};

Texture2D albedoTexture : register(t0);
Texture2D normalTexture : register(t1);
SamplerState texSampler : register(s0);

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texcoord : TEXCOORD;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 worldNormal : NORMAL;
    float2 texcoord : TEXCOORD;
    float3 worldPos : WORLDPOS;
};

VSOutput VSMain(VSInput input)
{
    VSOutput output;
    float4 worldPos = mul(float4(input.position, 1.0f), world);
    output.position = mul(mul(worldPos, view), projection);
    output.worldNormal = normalize(mul(input.normal, (float3x3) world));
    output.texcoord = input.texcoord;
    output.worldPos = worldPos.xyz;
    return output;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
    float4 texColor = albedoTexture.Sample(texSampler, input.texcoord);
    float4 albedo = texColor * baseColor;
    if (dot(texColor.rgb, 1.0f) < 0.001f)
        albedo = baseColor;

    float3 N = normalize(input.worldNormal);
    float3 L = normalize(-lightDir.xyz);
    float NdotL = max(0.0f, dot(N, L));

    float3 ambient = ambientColor.xyz * albedo.xyz;
    float3 diffuse = lightColor.xyz * albedo.xyz * NdotL;

    float3 V = normalize(cameraPos.xyz - input.worldPos);
    float3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0f), roughness * 128.0f + 1.0f);
    float3 specular = lightColor.xyz * spec * (1.0f - metallic);

    return float4(ambient + diffuse + specular, albedo.w);
}
