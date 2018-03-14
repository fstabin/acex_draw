struct VSInput
{
	matrix	mat		: WORLD;
	float4 col 	: COLOR;
	float4 texoffs 	: TEXOFFS;
};

VSInput main(VSInput In)
{
	return In;
}
