struct VSInput
{
	float4	 position		: POSITION;

	matrix world: WORLD;
};

struct VSOutput
{
	float4 	position	: SV_Position;
};

cbuffer CBScene : register(b0)
{
	float4x4 cam;
	float4x4	pro;
};

VSOutput main(VSInput In)
{
	VSOutput Out;
	Out.position = mul(In.position, In.world);
	Out.position = mul(Out.position, cam);
	Out.position = mul(Out.position, pro);
	return Out;
}
