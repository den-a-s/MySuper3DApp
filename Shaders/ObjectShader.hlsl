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
    float4 specularColor;
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

    float3 sampledNormal = normalTexture.Sample(texSampler, input.texcoord).xyz;
    sampledNormal = sampledNormal * 2.0f - 1.0f;
    if (abs(sampledNormal.x) > 0.01f || abs(sampledNormal.y) > 0.01f) {
        float3 dp1 = ddx(input.worldPos);
        float3 dp2 = ddy(input.worldPos);
        float2 duv1 = ddx(input.texcoord);
        float2 duv2 = ddy(input.texcoord);
        float3 T = dp1 * duv2.y - dp2 * duv1.y;
        if (length(T) > 0.001f) {
            T = normalize(T);
            float3 B = cross(N, T);
            sampledNormal = normalize(sampledNormal);
            N = normalize(mul(sampledNormal, float3x3(T, B, N)));
        }
    }

    float3 L = normalize(-lightDir.xyz);
    float NdotL = max(0.0f, dot(N, L));

    float3 ambient = ambientColor.xyz * albedo.xyz;
    float3 diffuse = lightColor.xyz * albedo.xyz * NdotL;

    float3 V = normalize(cameraPos.xyz - input.worldPos);
    float3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0f), roughness * 128.0f + 1.0f);
    float3 specular = specularColor.xyz * lightColor.xyz * spec * (1.0f - metallic);

    return float4(ambient + diffuse + specular, albedo.w);
}
