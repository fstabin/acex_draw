struct PSInput
{
	float4 	position	: SV_POSITION;
	float4 	color		: COLOR;
};

float4 main(PSInput In) : SV_TARGET0
{
	return In.color;
}
