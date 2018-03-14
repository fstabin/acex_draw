struct PSInput
{
	float4 	position	: SV_POSITION;
	float4 	color		: COLOR;
	float3 	nor		: NORMAL;
	float4 	shadow		: SHADOW;
};

cbuffer CBScene : register(b0)
{
	float3 light;
	float3 Diffuse;
	float3 Ambient;
	float3 Emissive;
};


texture2D dshdow: register(t0);
SamplerState  sampler_shadow : register(s0);

float4 main(PSInput In) : SV_TARGET0
{
	float3 L = normalize(light);
	float3 N = normalize(In.nor);
	float nor = dot(L, N);
	nor = (nor + 1.f) * 0.5f;

	float2 shadowuv;
	shadowuv.x = (1.0f + In.shadow.x / In.shadow.w) * 0.5f;
	shadowuv.y = (1.0f - In.shadow.y / In.shadow.w) * 0.5f;
	float sdepth = dshdow.Sample(sampler_shadow, shadowuv);
	float shadow = (sdepth + 0.0005f > In.shadow.z / In.shadow.w) ? 1.f : 0.f;

	nor = min(shadow, nor);

	float4 col = In.color;
	col.rgb = col.rgb * lerp(Ambient, Diffuse, nor);
	return col;
}