struct PSInput
{
	float4 position	: SV_POSITION;
	float4	 color	: COLOR;
	float2 uv		: TEXCOORD;
};
texture2D tex : register(t0);
SamplerState  sampler_0 : register(s0);
float4 main(PSInput In) : SV_TARGET0
{
	float4 col = tex.Sample(sampler_0,In.uv);
	return col * In.color;
}