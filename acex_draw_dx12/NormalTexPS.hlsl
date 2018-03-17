struct PSInput
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
	float3 	nor : NORMAL;
};
texture2D tex : register(t0);
SamplerState  sampler_0 : register(s0);

cbuffer CBScene : register(b1)
{
	float3 light;
	float3 Diffuse;
	float3 Ambient;
	float3 Emissive;
};

float4 main(PSInput In) : SV_TARGET0
{
	float4 col = tex.Sample(sampler_0,In.uv);

	float3 L = normalize(light);
	float3 N = normalize(In.nor);
	float nor = dot(L, N);
	nor = (nor + 1) / 2;
	col.rgb = col.rgb * lerp(Ambient, Diffuse, nor);
	return col;
}