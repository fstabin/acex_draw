struct PSInput
{
	float4 	position	: SV_POSITION;
	float4 	color		: COLOR;
	float3 	nor		: NORMAL;
};

cbuffer CBScene : register(b0)
{
	float3 light;
	float3 Diffuse;
	float3 Ambient;

	float3 Emissive;
};

float4 main(PSInput In) : SV_TARGET0
{
	float4 col = In.color;

	float3 L = normalize(light);
	float3 N = normalize(In.nor);
	float nor = dot(L, N);
	nor = (nor + 1) / 2;
	col.rgb = col.rgb * lerp(Ambient, Diffuse, nor);
	return col;
}