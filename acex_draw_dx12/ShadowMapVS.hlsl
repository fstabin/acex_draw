struct VSInput
{
	float4 position		: POSITION;
	float4 color			:COLOR;
	float3 nor		: NORMAL;

	matrix world: WORLD;
};

struct VSOutput
{
	float4 	position	: SV_POSITION;
	float4 	color		: COLOR;
	float3 	nor		: NORMAL;
	float4 	shadow		: SHADOW;
};

cbuffer CBScene : register(b0)
{
	float4x4 cam;
	float4x4	pro;
};

cbuffer Light : register(b1)
{
	float4x4 lcam;
	float4x4	lpro;
};

VSOutput main(VSInput In)
{
	VSOutput Out;

	Out.position = mul(In.position, In.world);
	Out.position = mul(Out.position, cam);
	Out.position = mul(Out.position, pro);
	Out.color = In.color;

	Out.nor = mul(In.nor, (float3x3)In.world);

	Out.shadow = mul(In.position, In.world);
	Out.shadow = mul(Out.shadow, lcam);
	Out.shadow = mul(Out.shadow, lpro);

	return Out;
}
