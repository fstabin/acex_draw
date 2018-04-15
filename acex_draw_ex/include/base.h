#pragma once

#include "..\..\acex_draw\include\acex_draw.h"
#include <cstdint>
#include "acs\include\def.h"
#include "acs_ioImage\include\acs_ioImage.h"

namespace acex {
	namespace draw {
		namespace ex {
			ACS_EXCEPTION_DEF(ACEXDrawExException, "acex::drawExException");
			ACS_EXCEPTION_DEF1(ACEXDrawExError, "acex::drawExError", ACEXDrawExException);

			ACS_EXCEPTION_DEF1(ACEXDrawExInitFailedException, "acex::drawInitFailed", ACEXDrawExException);
			ACS_EXCEPTION_DEF1(ACEXDrawExCreateResourceFailedException, "acex::drawExCreateResourceFailed", ACEXDrawExException);

			ACS_EXCEPTION_DEF1(ACEXDrawExResourceNotInitedException, "acex::ExResourceNotInited", ACEXDrawExException);

			ACS_EXCEPTION_DEF1(ACEXDrawExInvalidValueException, "acex::drawExInvalidValue", ACEXDrawExException);
			ACS_EXCEPTION_DEF1(ACEXDrawExValueTooLargeException, "acex::drawExValueTooLarge", ACEXDrawExInvalidValueException);
			ACS_EXCEPTION_DEF1(ACEXDrawExValueTooSmallException, "acex::drawExValueTooSmall", ACEXDrawExInvalidValueException);

			class Drawer {
				const acs::SIACS<acex::draw::IDraw> idraw;
				acex::draw::IDrawer* context;
			public:
				Drawer(acex::draw::IDraw* draw) :idraw(draw) {
					if (!idraw->BeginDraw(&context))throw(ACEXDrawExInvalidValueException());
				}
				~Drawer() {
					idraw->EndDraw();
				}

				ACS_NO_COPY(Drawer);

				inline acex::draw::IDrawer* Ptr()const noexcept {
					return context;
				}
				inline operator acex::draw::IDrawer*()const noexcept {
					return context;
				}
				inline acex::draw::IDrawer* operator-> ()const noexcept {
					return context;
				}
			};

			class Updater {
				const acs::SIACS<acex::draw::IDraw> idraw;
				acex::draw::IUpdater* updater;
			public:
				Updater(acex::draw::IDraw* draw) :idraw(draw) {
					idraw->BeginUpdate(&updater);
				}
				~Updater() {
					idraw->EndUpdate();
				}

				ACS_NO_COPY(Updater);

				inline acex::draw::IUpdater* Ptr()const noexcept {
					return updater;
				}
				inline operator acex::draw::IUpdater*()const noexcept {
					return updater;
				}
				inline acex::draw::IUpdater* operator-> ()const noexcept {
					return updater;
				}
			};

			template<typename T>
			class Mapped {
				acs::SIACS<acex::draw::IResource> ires;
				acex::draw::IUpdater* iupd;
				T* ptr;
			public:
				Mapped(acex::draw::IUpdater* upd, acex::draw::IResource* res) {
					ires = res;
					iupd = upd;
					iupd->Map(ires, reinterpret_cast<void**>(&ptr));
				}
				~Mapped() {
					iupd->Unmap(ires);
				}

				ACS_NO_COPY(Mapped);

				inline T* data()const noexcept {
					return ptr;
				}
				inline operator T*()const noexcept {
					return ptr;
				}
				//	inline T& operator[] (size_t i)const noexcept {
				//		return ptr[i];
				//	}
			};

			inline bool CreateITarget(acex::draw::ITarget** out, acex::draw::IDraw* Draw, UINT32 x, UINT32 y, void*data = nullptr) {
				acex::draw::RESOURCE_DESC rdesc;
				acex::draw::TEXTURE2D_DESC cdesc;
				acs::SIACS<acex::draw::ITexture2D> tex2d;
				cdesc.Width = x;
				cdesc.Height = y;
				cdesc.useflag = acex::draw::TEXUSE_TARGET;
				rdesc.type = acex::draw::RESOURCE_TYPE_TEXTURE2D;
				rdesc.AccessFlag = acex::draw::RESOURCE_ACCESS_NONE;
				rdesc.desc = &cdesc;
				if (!Draw->CreateResource(rdesc, data, reinterpret_cast<void**>(&tex2d)))return false;
				if (!Draw->CreateView(tex2d, acex::draw::VIEW_TARGET, reinterpret_cast<void**>(out)))return false;
				return true;
			}
			inline bool CreateITargetResource(acex::draw::ITarget** otarget, acex::draw::IRenderResource** rsorresource, acex::draw::IDraw* Draw, UINT32 x, UINT32 y, void*data = nullptr) {
				acex::draw::RESOURCE_DESC rdesc;
				acex::draw::TEXTURE2D_DESC cdesc;
				acs::SIACS<acex::draw::ITexture2D> tex2d;
				cdesc.Width = x;
				cdesc.Height = y;
				cdesc.useflag = acex::draw::TEXUSE_TARGET | acex::draw::TEXUSE_RENDER_RESOURCE;
				rdesc.type = acex::draw::RESOURCE_TYPE_TEXTURE2D;
				rdesc.AccessFlag = acex::draw::RESOURCE_ACCESS_NONE;
				rdesc.desc = &cdesc;
				if (!Draw->CreateResource(rdesc, data, reinterpret_cast<void**>(&tex2d)))return false;
				if (!Draw->CreateView(tex2d, acex::draw::VIEW_TARGET, reinterpret_cast<void**>(otarget)))return false;
				if (!Draw->CreateView(tex2d, acex::draw::VIEW_RENDER_RESOURCE, reinterpret_cast<void**>(rsorresource)))return false;
				return true;
			}
			inline bool CreateIDepthStencil(acex::draw::IDepthStencil** out, acex::draw::IDraw* Draw, UINT32 x, UINT32 y, void*data = nullptr) {
				acex::draw::RESOURCE_DESC rdesc;
				acex::draw::TEXTURE2D_DESC cdesc;
				acs::SIACS<acex::draw::ITexture2D> tex2d;
				cdesc.Width = x;
				cdesc.Height = y;
				cdesc.useflag = acex::draw::TEXUSE_DEPTHSTENCIL;
				rdesc.type = acex::draw::RESOURCE_TYPE_TEXTURE2D;
				rdesc.AccessFlag = acex::draw::RESOURCE_ACCESS_NONE;
				rdesc.desc = &cdesc;
				if (!Draw->CreateResource(rdesc, data, reinterpret_cast<void**>(&tex2d)))return false;
				if (!Draw->CreateView(tex2d, acex::draw::VIEW_DEPTHSTENCIL, reinterpret_cast<void**>(out)))return false;
				return true;
			}
			inline bool CreateIDepthBuffer(acex::draw::IDepthStencil** out, acex::draw::IRenderResource** rsorresource, acex::draw::IDraw* Draw, UINT32 x, UINT32 y, void*data = nullptr) {
				acex::draw::RESOURCE_DESC rdesc;
				acex::draw::TEXTURE2D_DESC cdesc;
				acs::SIACS<acex::draw::ITexture2D> tex2d;
				cdesc.Width = x;
				cdesc.Height = y;
				cdesc.useflag = acex::draw::TEXUSE_DEPTHSTENCIL | acex::draw::TEXUSE_RENDER_RESOURCE;
				rdesc.type = acex::draw::RESOURCE_TYPE_TEXTURE2D;
				rdesc.AccessFlag = acex::draw::RESOURCE_ACCESS_NONE;
				rdesc.desc = &cdesc;
				if (!Draw->CreateResource(rdesc, data, reinterpret_cast<void**>(&tex2d)))return false;
				if (!Draw->CreateView(tex2d, acex::draw::VIEW_DEPTHSTENCIL, reinterpret_cast<void**>(out)))return false;
				if (!Draw->CreateView(tex2d, acex::draw::VIEW_RENDER_RESOURCE, reinterpret_cast<void**>(rsorresource)))return false;
				return true;
			}
			inline bool CreateITexture2DP(acex::draw::IDraw* Draw, acs::image::IioImage* loader, const wchar_t* pass, acex::draw::ITexture2D** out) {
				acs::SIACS<acs::image::IImageData> img;
				if (!loader->CleateImageFromFilename(pass, &img))return false;
				acs::image::IMAGE_STATE state;
				if (!img->GetState(&state))return false;
				acex::draw::RESOURCE_DESC rdesc;
				acex::draw::TEXTURE2D_DESC cdesc;
				cdesc.Height = state.Height;
				cdesc.Width = state.Width;
				cdesc.useflag = acex::draw::TEXUSE_RENDER_RESOURCE;
				rdesc.type = acex::draw::RESOURCE_TYPE_TEXTURE2D;
				rdesc.AccessFlag = acex::draw::RESOURCE_ACCESS_NONE;
				rdesc.desc = &cdesc;
				if (!Draw->CreateResource(rdesc, img->GetData(), reinterpret_cast<void**>(out)))return false;
				return true;
			}
			inline bool CreateIRenderResource(acex::draw::IDraw* Draw, acex::draw::ITexture2D* tex2d, acex::draw::IRenderResource** out) {
				if (!Draw->CreateView(tex2d, acex::draw::VIEW_RENDER_RESOURCE, reinterpret_cast<void**>(out)))return false;
				return true;
			}
			inline bool CreateIRenderResourceM(acex::draw::IDraw* Draw, UINT32 x, UINT32 y, const acex::draw::COLOR*data, acex::draw::IRenderResource** out) {
				acex::draw::RESOURCE_DESC rdesc;
				acex::draw::TEXTURE2D_DESC cdesc;
				acs::SIACS<acex::draw::ITexture2D> tex2d;
				cdesc.Width = x;
				cdesc.Height = y;
				cdesc.useflag = acex::draw::TEXUSE_RENDER_RESOURCE;
				rdesc.type = acex::draw::RESOURCE_TYPE_TEXTURE2D;
				rdesc.AccessFlag = acex::draw::RESOURCE_ACCESS_NONE;
				rdesc.desc = &cdesc;
				if (!Draw->CreateResource(rdesc, data, reinterpret_cast<void**>(&tex2d)))return false;
				if (!CreateIRenderResource(Draw, tex2d, out))return false;
				return true;
			}
			inline bool CreateIRenderResourceP(acex::draw::IDraw* Draw, acs::image::IioImage* loader, const wchar_t* pass, acex::draw::IRenderResource** out) {
				acs::SIACS<acs::image::IImageData> img;
				acs::SIACS<acex::draw::ITexture2D> tex2d;
				if (!CreateITexture2DP(Draw, loader, pass, &tex2d))return false;
				if (!CreateIRenderResource(Draw, tex2d, out))return false;
				return true;
			}
						
			inline bool CreateIIndex(
				acex::draw::IIndex** _returnVal,
				acex::draw::IDraw* Draw,
				acex::draw::INDEX_FOMAT fomat,
				uint32_t ArraySize,
				uint32_t AccessFlag = acex::draw::RESOURCE_ACCESS_NONE,
				const void* InitData = nullptr
			)
			{
				acex::draw::INDEXBUFFER_DESC cdesc = { fomat,ArraySize };
				acex::draw::RESOURCE_DESC rdesc = { acex::draw::RESOURCE_TYPE_INDEX,AccessFlag ,&cdesc };
				return (Draw->CreateResource(rdesc, InitData, (void**)_returnVal));
			}
			inline bool CreateIVPosition(
				acex::draw::IVPosition** _returnVal,
				acex::draw::IDraw* Draw,
				uint32_t ArraySize,
				uint32_t AccessFlag = acex::draw::RESOURCE_ACCESS_NONE,
				const acex::draw::VERTEX_POSITION* InitData = nullptr
			) {
				acex::draw::VERTEXBUFFER_DESC cdesc = { ArraySize };
				acex::draw::RESOURCE_DESC rdesc = { acex::draw::RESOURCE_TYPE_VERTEXPOS,AccessFlag ,&cdesc };
				return (Draw->CreateResource(rdesc, InitData, (void**)_returnVal));
			}
			inline bool CreateIVColor(
				acex::draw::IVColor** _returnVal,
				acex::draw::IDraw* Draw,
				uint32_t ArraySize,
				uint32_t AccessFlag = acex::draw::RESOURCE_ACCESS_NONE,
				const	acex::draw::VERTEX_COLOR* InitData = nullptr
			) {
				acex::draw::VERTEXBUFFER_DESC cdesc = { ArraySize };
				acex::draw::RESOURCE_DESC rdesc = { acex::draw::RESOURCE_TYPE_VERTEXCOLOR,AccessFlag ,&cdesc };
				return (Draw->CreateResource(rdesc, InitData, (void**)_returnVal));
			}
			inline bool CreateIVUv(
				acex::draw::IVUv** _returnVal,
				acex::draw::IDraw* Draw,
				uint32_t ArraySize,
				uint32_t AccessFlag = acex::draw::RESOURCE_ACCESS_NONE,
				const		acex::draw::VERTEX_UV* InitData = nullptr
			) {
				acex::draw::VERTEXBUFFER_DESC cdesc = { ArraySize };
				acex::draw::RESOURCE_DESC rdesc = { acex::draw::RESOURCE_TYPE_VERTEXUV,AccessFlag ,&cdesc };
				return (Draw->CreateResource(rdesc, InitData, (void**)_returnVal));
			}
			inline bool CreateIVNormal(
				acex::draw::IVNormal** _returnVal,
				acex::draw::IDraw* Draw,
				uint32_t ArraySize,
				uint32_t AccessFlag = acex::draw::RESOURCE_ACCESS_NONE,
				const		acex::draw::VERTEX_NORMAL* InitData = nullptr
			) {
				acex::draw::VERTEXBUFFER_DESC cdesc = { ArraySize };
				acex::draw::RESOURCE_DESC rdesc = { acex::draw::RESOURCE_TYPE_VERTEXNORMAL,AccessFlag ,&cdesc };
				return (Draw->CreateResource(rdesc, InitData, (void**)_returnVal));
			}
			inline bool CreateIIWorld(
				acex::draw::IIWorld** _returnVal,
				acex::draw::IDraw* Draw,
				uint32_t ArraySize,
				uint32_t AccessFlag = acex::draw::RESOURCE_ACCESS_NONE,
				const	acex::draw::INS_WORLD* InitData = nullptr
			) {
				acex::draw::INSTANCEBUFFER_DESC cdesc = { ArraySize };
				acex::draw::RESOURCE_DESC rdesc = { acex::draw::RESOURCE_TYPE_INSWORLD,AccessFlag ,&cdesc };
				return (Draw->CreateResource(rdesc, InitData, (void**)_returnVal));
			}
			inline bool CreateIITexState(
				acex::draw::IITexState** _returnVal,
				acex::draw::IDraw* Draw,
				uint32_t ArraySize,
				uint32_t AccessFlag = acex::draw::RESOURCE_ACCESS_NONE,
				const acex::draw::INS_TEXSTATE* InitData = nullptr
			) {
				acex::draw::INSTANCEBUFFER_DESC cdesc = { ArraySize };
				acex::draw::RESOURCE_DESC rdesc = { acex::draw::RESOURCE_TYPE_INSTEXSTATE,AccessFlag ,&cdesc };
				return (Draw->CreateResource(rdesc, InitData, (void**)_returnVal));
			}
			inline bool CreateICamPro(
				acex::draw::ICamPro** _returnVal,
				acex::draw::IDraw* Draw,
				uint32_t AccessFlag = acex::draw::RESOURCE_ACCESS_NONE,
				const	acex::draw::C_CAMPRO* InitData = nullptr
			) {
				acex::draw::CAMPRO_DESC cdesc = {};
				const	acex::draw::RESOURCE_DESC rdesc = { acex::draw::RESOURCE_TYPE_CAMPRO,AccessFlag ,&cdesc };
				return (Draw->CreateResource(rdesc, InitData, (void**)_returnVal));
			}
			inline bool CreateILight(
				acex::draw::ILight** _returnVal,
				acex::draw::IDraw* Draw,
				uint32_t AccessFlag = acex::draw::RESOURCE_ACCESS_NONE,
				const	acex::draw::C_LIGHT* InitData = nullptr
			) {
				acex::draw::LIGHT_DESC cdesc = {};
				const	acex::draw::RESOURCE_DESC rdesc = { acex::draw::RESOURCE_TYPE_LIGHT,AccessFlag ,&cdesc };
				return (Draw->CreateResource(rdesc, InitData, (void**)_returnVal));
			}

			inline void RenderDefault(
				acex::draw::IDrawer* Draw,
				uint32_t InstanceCount,
				uint32_t VertexCount,
				uint32_t IndexCount,
				acex::draw::PRIMITIVE_TOPOLOGY topology,
				acex::draw::IIndex* index,
				acex::draw::IVPosition* positions,
				acex::draw::IVColor* colors,
				acex::draw::IIWorld* worlds
			) {
				acex::draw::RENDER_DATA_DEFAULT data = { InstanceCount ,VertexCount ,IndexCount ,topology,index,positions,colors,worlds };
				Draw->Draw(acex::draw::RM_DEFAULT, &data);
			}
			inline void RenderTexture(
				acex::draw::IDrawer* Draw,
				uint32_t InstanceCount,
				uint32_t VertexCount,
				uint32_t IndexCount,
				acex::draw::PRIMITIVE_TOPOLOGY topology,
				acex::draw::TEXSAMPLE_MODE sample,
				acex::draw::IIndex* index,
				acex::draw::IVPosition* positions,
				acex::draw::IVUv* uvs,
				acex::draw::IIWorld* worlds,
				acex::draw::IRenderResource* texture
			) {
				acex::draw::RENDER_DATA_TEXTURE data = { InstanceCount ,VertexCount ,IndexCount ,topology,sample,index,positions,uvs,worlds,texture };
				Draw->Draw(acex::draw::RM_TEXTURE, &data);
			}
			inline void RenderWorld(
				acex::draw::IDrawer* Draw,
				uint32_t InstanceCount,
				uint32_t VertexCount,
				uint32_t IndexCount,
				acex::draw::PRIMITIVE_TOPOLOGY topology,
				acex::draw::IIndex* index,
				acex::draw::IVPosition* positions,
				acex::draw::IVColor* colors,
				acex::draw::IIWorld* worlds,
				acex::draw::ICamPro* campro
			) {
				acex::draw::RENDER_DATA_WORLD data = { InstanceCount ,VertexCount ,IndexCount ,topology,index,positions,colors,worlds,campro };
				Draw->Draw(acex::draw::RM_WORLD, &data);
			}
			inline void RenderWorldTex(
				acex::draw::IDrawer* Draw,
				uint32_t InstanceCount,
				uint32_t VertexCount,
				uint32_t IndexCount,
				acex::draw::PRIMITIVE_TOPOLOGY topology,
				acex::draw::TEXSAMPLE_MODE sample,
				acex::draw::IIndex* index,
				acex::draw::IVPosition* positions,
				acex::draw::IVUv* uvs,
				acex::draw::IIWorld* worlds,
				acex::draw::IITexState* texstates,
				acex::draw::ICamPro* campro,
				acex::draw::IRenderResource* texture
			) {
				acex::draw::RENDER_DATA_WORLD_TEX data = { InstanceCount ,VertexCount ,IndexCount ,topology,sample,index,positions,uvs,worlds,texstates,campro,texture };
				Draw->Draw(acex::draw::RM_WORLD_TEX, &data);
			}
			inline void RenderWorldTex_C(
				acex::draw::IDrawer* Draw,
				uint32_t InstanceCount,
				uint32_t VertexCount,
				uint32_t IndexCount,
				acex::draw::PRIMITIVE_TOPOLOGY topology,
				acex::draw::TEXSAMPLE_MODE sample,
				acex::draw::IIndex* index,
				acex::draw::IVPosition* positions,
				acex::draw::IVUv* uvs,
				acex::draw::IIWorld* worlds,
				acex::draw::IITexState* texstates,
				acex::draw::ICamPro* campro,
				acex::draw::IRenderResource* texture
			) {
				acex::draw::RENDER_DATA_WORLD_TEX data = { InstanceCount ,VertexCount ,IndexCount ,topology,sample,index,positions,uvs,worlds,texstates,campro,texture };
				Draw->Draw(acex::draw::RM_WORLD_TEX_C, &data);
			}

			inline void RenderWCT(
				acex::draw::IDrawer* Draw,
				uint32_t InstanceCount,
				uint32_t VertexCount,
				uint32_t IndexCount,
				acex::draw::PRIMITIVE_TOPOLOGY topology,
				acex::draw::TEXSAMPLE_MODE sample,
				acex::draw::IIndex* index,
				acex::draw::IVPosition* positions,
				acex::draw::IVColor* colors,
				acex::draw::IVUv* uvs,
				acex::draw::IIWorld* worlds,
				acex::draw::IITexState* texstates,
				acex::draw::ICamPro* campro,
				acex::draw::IRenderResource* texture
			) {
				acex::draw::RENDER_DATA_WCT data = { InstanceCount ,VertexCount ,IndexCount ,topology,sample,index,positions,colors,uvs,worlds,texstates,campro,texture };
				Draw->Draw(acex::draw::RM_WCT, &data);
			}
			inline void RenderWCT_C(
				acex::draw::IDrawer* Draw,
				uint32_t InstanceCount,
				uint32_t VertexCount,
				uint32_t IndexCount,
				acex::draw::PRIMITIVE_TOPOLOGY topology,
				acex::draw::TEXSAMPLE_MODE sample,
				acex::draw::IIndex* index,
				acex::draw::IVPosition* positions,
				acex::draw::IVColor* colors,
				acex::draw::IVUv* uvs,
				acex::draw::IIWorld* worlds,
				acex::draw::IITexState* texstates,
				acex::draw::ICamPro* campro,
				acex::draw::IRenderResource* texture
			) {
				acex::draw::RENDER_DATA_WCT data = { InstanceCount ,VertexCount ,IndexCount ,topology,sample,index,positions,colors,uvs,worlds,texstates,campro,texture };
				Draw->Draw(acex::draw::RM_WCT_C, &data);
			}


			inline void RenderNormalWorld(
				acex::draw::IDrawer* Draw,
				uint32_t InstanceCount,
				uint32_t VertexCount,
				uint32_t IndexCount,
				acex::draw::PRIMITIVE_TOPOLOGY topology,
				acex::draw::IIndex* index,
				acex::draw::IVPosition* positions,
				acex::draw::IVColor* colors,
				acex::draw::IVNormal* nor,
				acex::draw::IIWorld* worlds,
				acex::draw::ICamPro* campro,
				acex::draw::ILight* light
			) {
				acex::draw::RENDER_DATA_NORMAL_WORLD data = { InstanceCount ,VertexCount ,IndexCount ,topology,index,positions,colors,nor,worlds,campro,light };
				Draw->Draw(acex::draw::RM_NORMAL_WORLD, &data);
			}

			inline void RenderNormalWorldTex(
				acex::draw::IDrawer* Draw,
				uint32_t InstanceCount,
				uint32_t VertexCount,
				uint32_t IndexCount,
				acex::draw::PRIMITIVE_TOPOLOGY topology,
				acex::draw::TEXSAMPLE_MODE sample,
				acex::draw::IIndex* index,
				acex::draw::IVPosition* positions,
				acex::draw::IVUv* uvs,
				acex::draw::IVNormal* nor,
				acex::draw::IIWorld* worlds,
				acex::draw::IITexState* texstates,
				acex::draw::ICamPro* campro,
				acex::draw::ILight* light,
				acex::draw::IRenderResource* texture
			) {
				acex::draw::RENDER_DATA_NORMAL_WORLD_TEX data = { InstanceCount ,VertexCount ,IndexCount ,topology,sample,index,positions,uvs,nor,worlds,texstates,campro,light,texture };
				Draw->Draw(acex::draw::RM_NORMAL_WORLD_TEX, &data);
			}

			inline void RenderSprite(
				acex::draw::IDrawer* Draw,
				uint32_t InstanceCount,
				acex::draw::TEXSAMPLE_MODE sample,
				acex::draw::IIWorld* worlds,
				acex::draw::IITexState* texstates,
				acex::draw::ICamPro* campro,
				acex::draw::IRenderResource* texture,
				uint32_t InstanceOffs = 0
			) {
				acex::draw::RENDER_DATA_SPRITE data = { InstanceCount,InstanceOffs ,sample,worlds,texstates,campro,texture };
				Draw->Draw(acex::draw::RM_SPRITE, &data);
			}

			inline void RenderSpriteColor(
				acex::draw::IDrawer* Draw,
				uint32_t InstanceCount,
				acex::draw::TEXSAMPLE_MODE sample,
				acex::draw::IIWorld* worlds,
				acex::draw::IVColor* colors,
				acex::draw::IITexState* texstates,
				acex::draw::ICamPro* campro,
				acex::draw::IRenderResource* texture,
				uint32_t InstanceOffs = 0
			) {
				acex::draw::RENDER_DATA_SPRITE_COLOR data = { InstanceCount,InstanceOffs ,sample,worlds,colors,texstates,campro,texture };
				Draw->Draw(acex::draw::RM_SPRITE_COLOR, &data);
			}


			inline void RenderShadowed(
				acex::draw::IDrawer* Draw,
				uint32_t InstanceCount,
				uint32_t IndexCount,
				acex::draw::PRIMITIVE_TOPOLOGY topology,
				acex::draw::IIndex* index,
				acex::draw::IVPosition* positions,
				acex::draw::IVColor* colors,
				acex::draw::IVNormal* normal,
				acex::draw::IIWorld* worlds,
				acex::draw::ICamPro* campro,
				acex::draw::ILight* light,
				acex::draw::ICamPro* Llightcp,
				acex::draw::IRenderResource* shadow,
				uint32_t InstanceOffs = 0,
				uint32_t IndexOffs = 0,
				uint32_t VertexOffs = 0
			) {
				acex::draw::RENDER_DATA_SHADOW data = { InstanceCount,InstanceOffs ,IndexCount,IndexOffs,VertexOffs,
					topology,index,positions,colors,normal,worlds,campro,light,Llightcp,shadow };
				Draw->Draw(acex::draw::RM_SHADOWED, &data);
			}
			inline void RenderDepth(
				acex::draw::IDrawer* Draw,
				uint32_t InstanceCount,
				uint32_t IndexCount,
				acex::draw::PRIMITIVE_TOPOLOGY topology,
				acex::draw::IIndex* index,
				acex::draw::IVPosition* positions,
				acex::draw::IIWorld* worlds,
				acex::draw::ICamPro* campro,
				uint32_t InstanceOffs = 0,
				uint32_t IndexOffs = 0,
				uint32_t VertexOffs = 0
			) {
				acex::draw::RENDER_DATA_DEPTH data = { InstanceCount,InstanceOffs,IndexCount, IndexOffs,VertexOffs,topology,index,positions,worlds,campro };
				Draw->Draw(acex::draw::RM_DEPTH, &data);
			}
		}
	}
}