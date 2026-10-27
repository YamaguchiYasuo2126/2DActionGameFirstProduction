cbuffer RushAimParameters : register(b0)
{
    float2 centerUv;
    float strength;
    float elapsedSeconds;
    float2 screenSize;
    float2 padding;
};

struct ScreenVertex
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

ScreenVertex VSMain(uint vertexId : SV_VertexID)
{
    ScreenVertex output;
    // 頂点バッファを使わず、画面を覆う三角形を生成する。
    output.uv = float2((vertexId << 1) & 2, vertexId & 2);
    output.position = float4(output.uv * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    return output;
}

float4 PSMain(ScreenVertex input) : SV_TARGET
{
    float2 offset = input.uv - centerUv;
    offset.x *= screenSize.x / screenSize.y;
    float distanceFromPlayer = length(offset);

    // 中央は見やすく残し、周辺だけ冷たい色のビネットで包む。
    float vignette = smoothstep(0.16, 0.95, distanceFromPlayer);
    float angle = atan2(offset.y, offset.x);
    float rays = pow(saturate(cos(angle * 20.0 + elapsedSeconds * 0.8)), 48.0);
    rays *= smoothstep(0.20, 0.36, distanceFromPlayer);
    rays *= 1.0 - smoothstep(0.90, 1.10, distanceFromPlayer);

    // 実時間で広がる薄い輪。ゲーム内時間が20%でも演出は止まらない。
    float ringRadius = 0.18 + frac(elapsedSeconds * 0.65) * 0.70;
    float ring = 1.0 - smoothstep(0.0, 0.028, abs(distanceFromPlayer - ringRadius));
    ring *= 1.0 - smoothstep(0.83, 1.0, distanceFromPlayer);

    float accent = saturate(rays * 0.8 + ring * 0.7);
    float3 color = lerp(float3(0.025, 0.075, 0.18), float3(0.22, 0.82, 1.0), accent);
    float alpha = strength * saturate(0.07 + 0.36 * vignette + 0.21 * rays + 0.25 * ring);
    return float4(color, alpha);
}
