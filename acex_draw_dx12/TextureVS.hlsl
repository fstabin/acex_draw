struct VSInput
{
	float4	 position	: POSITION;
float2	 uv		: TEXCOORD;
matrix world: WORLD;
};

struct VSOutput
{
	float4 position	: SV_POSITION;
	float2 uv		: TEXCOORD;
};

VSOutput main(VSInput In)
{
	VSOutput Out;
	Out.position = In.position;
	Out.position = mul(Out.position, In.world);
	Out.uv = In.uv;
	return Out;
}