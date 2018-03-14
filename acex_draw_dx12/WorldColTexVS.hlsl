struct VSInput
{
	float4	 position	: POSITION;
	float4	 color		: COLOR;
	float2	 uv		: TEXCOORD;

	matrix world: WORLD;

	float4 texoffs 	: TEXOFFS;
};

struct VSOutput
{
	float4 position	: SV_POSITION;
	float4	 color		: COLOR;
	float2 uv		: TEXCOORD;
};

cbuffer CBScene : register(b0)
{
	float4x4 cam;
	float4x4	pro;
};

VSOutput main(VSInput In)
{
	VSOutput Out;
	Out.position = In.position;
	Out.position = mul(In.position, In.world);
	Out.position = mul(Out.position, cam);
	Out.position = mul(Out.position, pro); 
	Out.color = In.color;
	Out.uv = (In.uv * In.texoffs.zw) + In.texoffs.xy;
	return Out;
}

