struct VSInput
{
	float4	pos	: POSITION;
};

struct VSOutput
{
	float4	pos	: SV_POSITION;
};

VSOutput main(VSInput In)
{
	VSOutput Out;

	Out.pos = In.pos;

	return Out;
}