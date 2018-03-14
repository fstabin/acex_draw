struct VSOutput
{
	matrix mat		: WORLD;
	float4 texoffs 	: TEXOFFS;
};

struct GSOutput
{
	float4 position	: SV_POSITION;
	float2 uv		: TEXCOORD;
};

cbuffer CBScene : register(b0)
{
	float4x4 cam;
	float4x4 pro;
};

[maxvertexcount(4)]
void main(point VSOutput In[1], inout TriangleStream<GSOutput> TriStream)
{
	float3 vpos[4] = { { -0.5,0.5,0 },{ 0.5,0.5,0 },{ -0.5,-0.5,0 },{ 0.5,-0.5,0 } };
	float2 vuv[4] = { { 0,0 } ,{ 1,0 },{ 0,1 },{ 1,1 } };
	uint index[6] = { 0,1,2,1,3,2 };

	matrix mat = mul(In[0].mat, cam);
	mat = mul(mat, pro);
	GSOutput Out;
	for (uint v = 0; v < 4; ++v)
	{
		uint ind = v;
		Out.position = mul(float4(vpos[ind],1), mat);
		Out.uv = vuv[ind];
		Out.uv = (Out.uv * In[0].texoffs.zw) + In[0].texoffs.xy;
		TriStream.Append(Out);
	}
	TriStream.RestartStrip();
}