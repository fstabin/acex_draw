struct VSInput
{
	float4	position	: POSITION;
	float4	color		: COLOR;
	float4x4	vertexOffset_ : WORLD;
};

struct VSOutput
{
	float4	position	: SV_POSITION;
	float4	color		: COLOR;
};

VSOutput main(VSInput In)
{
	VSOutput Out;

	Out.position = mul(In.position, In.vertexOffset_);
	Out.color = In.color;

	return Out;
}

//	EOF
