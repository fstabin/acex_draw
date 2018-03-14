struct VSInput
{
	float4	 position	: POSITION;
	float2	 uv		: TEXCOORD;

	matrix world: WORLD;

	float4 texoffs 	: TEXOFFS;
};

struct VSOutput
{
	float4 position	: SV_POSITION;
	float2 uv		: TEXCOORD;
};

cbuffer CBScene : register(b0)
{
	float4x4 cam;
	float4x4 pro;
};

VSOutput main(VSInput In)
{
	VSOutput Out;
	float4 woffs = In.world._m30_m31_m32_m33;
	woffs = mul(woffs, cam);
	Out.position = mul(In.position,
		matrix(In.world._m00_m01_m02_m03,
			In.world._m10_m11_m12_m13,
			In.world._m20_m21_m22_m23,
			woffs));
	Out.position = mul(Out.position, pro);
	Out.uv = (In.uv * In.texoffs.zw) + In.texoffs.xy;
	return Out;
}
