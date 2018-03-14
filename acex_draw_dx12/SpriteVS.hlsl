struct VSInput
{
	matrix	mat		: WORLD;
	float4 texoffs 	: TEXOFFS;
};

VSInput main(VSInput In)
{
	return In;
}