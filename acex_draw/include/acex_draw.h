#pragma once
#ifdef ACEX_DRAW_DLL_EXPORT
#define ACEX_DRAW_API __declspec(dllexport) 
#else
#define ACEX_DRAW_API __declspec(dllimport) 
#endif
#if _WIN32 | _WIN64
#include <windows.h>
#endif
#include "acex_draw_def.h"
#define GET_DESC_DEF(type) virtual void GetDesc(type*)throw() = 0

namespace  acex {
	namespace draw {
		/* ----- view ----- */
		class IView :public acs::IACS {
		public:
		};
		class IRenderResource :public IView {
		public:
		};
		class ITarget :public IView {
		public:
		};
		class IDepthStencil :public IView {
		public:
		};
		/* -----  Resource ----- */
		class IResource : public acs::IACS {
		};

		class IIndex : public IResource {
		public:
			GET_DESC_DEF(INDEXBUFFER_DESC);
		};
		class IVPosition :public IResource {
		public:
			GET_DESC_DEF(VERTEXBUFFER_DESC);
		};
		class IVColor :public IResource {
		public:
			GET_DESC_DEF(VERTEXBUFFER_DESC);
		};
		class IVUv :public IResource {
		public:
			GET_DESC_DEF(VERTEXBUFFER_DESC);
		};
		class IVNormal :public IResource {
		public:
			GET_DESC_DEF(VERTEXBUFFER_DESC);
		};

		class IIWorld :public IResource {
		public:
			GET_DESC_DEF(INSTANCEBUFFER_DESC);
		};
		class IITexState :public IResource {
		public:
			GET_DESC_DEF(INSTANCEBUFFER_DESC);
		};

		class ISprite :public IResource {
		public:
			GET_DESC_DEF(INSTANCEBUFFER_DESC);
		};

		class ICamPro :public IResource {
		public:
			GET_DESC_DEF(CAMPRO_DESC);
		};
		class ILight :public IResource {
		public:
			GET_DESC_DEF(LIGHT_DESC);
		};

		class ITexture : public acs::IACS {
		};

		class ITexture2D :public ITexture {
		public:
			GET_DESC_DEF(TEXTURE2D_DESC);
		};

		/* ----- RenderStates ----- */
		enum PRIMITIVE_TOPOLOGY : uintValue {
			PT_TRIANGLELIST = 1,
			PT_TRIANGLESTRIP = 2,
		};
		enum TEXSAMPLE_MODE :uintValue {
			TS_POINT = 0,
			TS_LINEAR = 1,
			TS_ANISOTROPIC = 2,
		};

		enum RENDER_MODE :uintValue {
			RM_NONE = 0,

			RM_DEFAULT = 1,//RENDER_DATA_DEFAULT

			//2D描画

			RM_SPRITE = 10000,//RENDER_DATA_SPRITE
			RM_SPRITE_COLOR = 10001,//RENDER_DATA_SPRITE_COL

			//3D描画

			RM_WORLD = 100,//RENDER_DATA_WORLD
			RM_WORLD_TEX = 200,//RENDER_DATA_WORLD_TEX
			RM_WORLD_TEX_C = 201,//RENDER_DATA_WORLD_TEX(a=0カット)

			RM_WCT = 250,//RENDER_DATA_WTC(color付テクスチャ)
			RM_WCT_C = 251,//RENDER_DATA_WTC(color付テクスチャ + a=0カット)

			RM_NORMAL_WORLD = 500,//RENDER_DATA_WORLD_TEX
			RM_NORMAL_WORLD_TEX = 600,//RENDER_DATA_WORLD_TEX

			RM_DEPTH = 10,//RENDER_DATA_DEPTH
			RM_SHADOWED = 510,//RENDER_DATA_SHADOW
		};

		struct RENDER_DATA_DEFAULT {
			uintValue InstanceCount;
			uintValue VertexCount;
			uintValue IndexCount;
			PRIMITIVE_TOPOLOGY topology;
			IIndex* index;
			IVPosition* positions;
			IVColor* colors;
			IIWorld* worlds;
		};
		struct RENDER_DATA_WORLD {
			uintValue InstanceCount;
			uintValue VertexCount;
			uintValue IndexCount;
			PRIMITIVE_TOPOLOGY topology;
			IIndex* index;
			IVPosition* positions;
			IVColor* colors;
			IIWorld* worlds;
			ICamPro* campro;
		};
		//WorldColorTextur
		struct RENDER_DATA_WCT {
			uintValue InstanceCount;
			uintValue VertexCount;
			uintValue IndexCount;
			PRIMITIVE_TOPOLOGY topology;
			TEXSAMPLE_MODE sample;
			IIndex* index;
			IVPosition* positions;
			IVColor* colors;
			IVUv* uvs;
			IIWorld* worlds;
			IITexState* texstates;
			ICamPro* campro;
			IRenderResource* texture;
		};
		struct RENDER_DATA_WORLD_TEX {
			uintValue InstanceCount;
			uintValue VertexCount;
			uintValue IndexCount;
			PRIMITIVE_TOPOLOGY topology;
			TEXSAMPLE_MODE sample;
			IIndex* index;
			IVPosition* positions;
			IVUv* uvs;
			IIWorld* worlds;
			IITexState* texstates;
			ICamPro* campro;
			IRenderResource* texture;
		};
		struct RENDER_DATA_NORMAL_WORLD {
			uintValue InstanceCount;
			uintValue VertexCount;
			uintValue IndexCount;
			PRIMITIVE_TOPOLOGY topology;
			IIndex* index;
			IVPosition* positions;
			IVColor* colors;
			IVNormal* normal;
			IIWorld* worlds;
			ICamPro* campro;
			ILight* light;
		};
		struct RENDER_DATA_NORMAL_WORLD_TEX {
			uintValue InstanceCount;
			uintValue VertexCount;
			uintValue IndexCount;
			PRIMITIVE_TOPOLOGY topology;
			TEXSAMPLE_MODE sample;
			IIndex* index;
			IVPosition* positions;
			IVUv* uvs;
			IVNormal* normal;
			IIWorld* worlds;
			IITexState* texstates;
			ICamPro* campro;
			ILight* light;
			IRenderResource* texture;
		};

		struct RENDER_DATA_SHADOW {
			uintValue InstanceCount;
			uintValue InstanceOffs;
			uintValue IndexCount;
			uintValue IndexOffs;
			uintValue VertexOffs;
			acex::draw::PRIMITIVE_TOPOLOGY topology;
			IIndex* index;
			IVPosition* positions;
			IVColor* colors;
			IVNormal* normal;
			IIWorld* worlds;
			ICamPro* campro;
			ILight* light;
			ICamPro* Llightcp;
			IRenderResource* shadow;
		};
		struct RENDER_DATA_SPRITE {
			uintValue InstanceCount;
			uintValue InstanceOffs;
			TEXSAMPLE_MODE sample;
			IIWorld* worlds;
			IITexState* texstates;
			ICamPro* campro;
			IRenderResource* texture;
		};
		struct RENDER_DATA_SPRITE_COLOR {
			uintValue InstanceCount;
			uintValue InstanceOffs;
			TEXSAMPLE_MODE sample;
			IIWorld* worlds;
			IVColor* colors;
			IITexState* texstates;
			ICamPro* campro;
			IRenderResource* texture;
		};

		struct RENDER_DATA_DEPTH {
			uintValue InstanceCount;
			uintValue InstanceOffs;
			uintValue IndexCount;
			uintValue IndexOffs;
			uintValue VertexOffs;
			PRIMITIVE_TOPOLOGY topology;
			IIndex* index;
			IVPosition* positions;
			IIWorld* worlds;
			ICamPro* campro;
		};

		class IDrawer {
		public:
			virtual void ACS_TCALL SetRenderArea(const AREA& area)throw() = 0;
			virtual AREA ACS_TCALL GetRenderArea()throw() = 0;

			virtual void ACS_TCALL SetTargets(uintValue TargetCnt, ITarget*const* pTarget, IDepthStencil* pDepth = nullptr)throw() = 0;
			virtual void ACS_TCALL ClearTarget(uintValue index, float color[4])throw() = 0;
			virtual void ACS_TCALL ClearDepthStencill()throw() = 0;

			virtual void ACS_TCALL CopyTexture(ITexture* Dst, ITexture* Src)throw() = 0;

			virtual void ACS_TCALL Draw(RENDER_MODE, const void* RenderData)throw() = 0;
		};
		class IUpdater {
		public:
			virtual bool ACS_TCALL Map(IResource*, void**)throw() = 0;
			virtual void ACS_TCALL Unmap(IResource*)throw() = 0;

			virtual bool ACS_TCALL UpdateTexture2D(ITexture2D* dest, const TEXTURE2D_SOURCE_INFO*, const COLOR* src)throw() = 0;
			virtual bool ACS_TCALL ReadTexture2D(ITexture2D* src, const TEXTURE2D_DEST_INFO*, COLOR* dest)throw() = 0;
		};

#if _WIN32 | _WIN64
		struct INIT_DESC {
			HWND hWnd;
			SIZE Size;
			bool useWarpDevice;
		};
#endif

		class IDraw :public acs::IACS {
		public:

			virtual bool GetScreenTarget(ITarget**)throw() = 0;
			virtual bool ResizeScreenTarget(const SIZE& size)throw() = 0;

			virtual bool CreateResource(
				const RESOURCE_DESC& desc,
				const void* pData,
				void** ppOut)throw() = 0;
			virtual bool CreateView(ITexture2D*, VIEW_TYPE, void**)throw() = 0;

			virtual bool BeginUpdate(IUpdater**)throw() = 0;
			virtual bool EndUpdate()throw() = 0;
			virtual bool BeginDraw(IDrawer**)throw() = 0;
			virtual bool EndDraw()throw() = 0;

			virtual bool Present(int)throw() = 0;
			virtual void WaitDrawDone()throw() = 0;

			virtual bool isEnable()throw() = 0;
		};
		enum LIB_MODE : uintValue {
			LIB_MODE_DX12 = 0,
			LIB_MODE_DX9 = 1,
		};

		ACEX_DRAW_API bool ACS_SCALL CreateDraw(INIT_DESC*, IDraw**)throw();
	}
}

