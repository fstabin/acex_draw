struct VSInput
{
	float4	 position	: POSITION;
	float4	 color		: COLOR;
	matrix	mat		: WORLD;
};

struct VSOutput
{
	float4 position	: SV_POSITION;
	float4 color		: COLOR;
};

cbuffer CBScene : register(b0)
{
	float4x4 cam;
	float4x4	pro;
};

VSOutput main(VSInput In)
{
	VSOutput Out;

	Out.position = mul(In.position, In.mat);
	Out.position = mul(Out.position, cam);
	Out.position = mul(Out.position, pro);
	Out.color = In.color;

	return Out;
}

//	EOF
