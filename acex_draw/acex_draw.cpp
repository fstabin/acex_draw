#include "stdafx.h"
#define ACEX_DRAW_DLL_EXPORT
#include "acex_draw\include\acex_draw.h"
#include <atlbase.h>
#include "acs\include\acs_def.h"

#define ACS_IACS_CHILD \
size_t refCnt = 1;\
virtual void Addref(){\
refCnt++;\
}\
virtual void Release(){\
refCnt--;\
if(refCnt == 0)delete this;\
}

#if _WIN64
const char InitFuncName[] = "?InitACEXDraw::draw@@YA_NPEAUINIT_DESC@acex::draw@@PEAPEAVIDraw@2@@Z";
#elif _WIN32
const char InitFuncName[] = "?InitACEXDraw::draw@@YG_NPAUINIT_DESC@acex::draw@@PAPAVIDraw@2@@Z";
#endif
//__declspec(dllexport) AT_DRAW_RESULT __stdcall Initacex::draw(AT_DRAW_INIT_DESC* desc, acex::draw**);
typedef bool ACS_SCALL DLLInitFunc(acex::draw::INIT_DESC* desc, acex::draw::IDraw**);
ACS_EXCEPTION_DEF(NO_SUPPORT, "NO_SUPPORT");
ACS_EXCEPTION_DEF(FILE_OPEN_ERR, "FILE_OPEN_ERR");

namespace acex {
	namespace draw {
		class CModule {
			HMODULE mod;
		public:
			CModule() :mod(nullptr) {

			}
			CModule(const char* dll_name) :mod(nullptr) {
				attach(dll_name);
			}
			~CModule() {
				free();
			}
			CModule(CModule&) = delete;
			CModule& operator=(CModule&) = delete;

			void attach(const char* dll_name) {
				free();
				mod = LoadLibraryA(dll_name);
				if (mod == NULL)FILE_OPEN_ERR();
			}
			void free() {
				if (mod) {
					FreeLibrary(mod);
					mod = nullptr;
				}
			}
			operator HMODULE() {
				return mod;
			}
		};

		class MLink :public ILink {
			CModule mod;
			DLLInitFunc* Init;
		public:
			ACS_IACS_CHILD
				virtual bool ACS_TCALL Query(void**out, uint32_t id) noexcept {
				return false;
			}

			MLink(LIB_MODE mode) {
				const char* dll_name;
				switch (mode)
				{
				case LIB_MODE_DX12:
					dll_name = "acex::draw2_dx12.dll";
					break;
				case LIB_MODE_DX9:
					dll_name = "acex::draw2_dx9.dll";
					break;
				default:
					throw(NO_SUPPORT());
				}
				mod.attach(dll_name);
				Init = reinterpret_cast<DLLInitFunc*>(GetProcAddress(mod, InitFuncName));
			}
			virtual bool ACS_TCALL CreateDraw(INIT_DESC* desc, IDraw** out) {
				return Init(desc, out);
			}
		};
	}
}

ACEX_DRAW_API bool ACS_SCALL acex::draw::CreateLink(ILink** plink, LIB_MODE mode) {
	try {
		*plink = static_cast<ILink*>(new MLink(mode));
	}
	catch (const std::exception& ex) {
		OutputDebugStringA(ex.what());
		return false;
	}
	return true;
}