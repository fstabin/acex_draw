#include "acex_draw_dx12.h"
#include <cstdlib>
unsigned int DescriptorSize_RTV;
bool __stdcall acex::draw::CreateDraw(acex::draw::INIT_DESC* desc, acex::draw::IDraw** out) {
	try
	{
		*out =new acex::draw::MDraw(desc);
	}
	catch (const std::exception& ex)
	{
		OutputDebugStringA(ex.what());
		return false;
	}
	return true;
}