#pragma once
#include "stdafx.h"
#define ACEX_DRAW_DLL_EXPORT
#include "acex_draw\include\acex_draw.h"
#include <dxgi1_5.h>
#include <D3d12.h>
#include <atomic>
#pragma comment (lib,"DXGI.lib")
#pragma comment (lib,"D3d12.lib")

#include "acs\include\def.h"

#include <mutex>
#include <thread>
#include <atlutil.h>
#include <memory>
#include<atlbase.h>
#include<new>
#include <atomic>

#include"TextureVS.h"
#include"TexturePS.h"
#include"TexturePS_C.h"
#include"WorldTexVS.h"

#include"EasyVS.h"
#include"EasyPS.h"

#include"WorldVS.h"
//#include"WorldPS.h"

#include "NormalWorldVS.h"
#include "NormalWorldTexVS.h"
#include "NormalPS.h"
#include "NormalTexPS.h"

#include "WorldColTexVS.h"
#include "WorldColTexPS.h"
#include "WorldColTexPS_C.h"

#include "SpriteVS.h"
#include "SpriteColVS.h"
#include "SpriteGS.h"
#include "SpriteColGS.h"

#include "DepthVS.h"
#include "DepthPS.h"

#include "ShadowMapVS.h"
#include "ShadowMapPS.h"

extern unsigned int DescriptorSize_RTV;

float color_zero[4] = { 0, 0, 0, 0 };

#define ACS_IACS_CHILD \
private:\
std::atomic_int64_t refCnt = 1;\
public:\
virtual void ACS_TCALL Addref()final{\
refCnt++;\
}\
virtual void ACS_TCALL Release()final{\
volatile int64_t  val; \
val = --refCnt;\
if(val <= 0)delete this;\
}
#define GET_DESC(type) private: type desc;\
public: virtual void ACS_TCALL GetDesc(type* out) final {*out = desc;};

#define GET_HEAP_DEF(name) const decltype(name)& GetHEAP(){return name;}//IDescripterHeap
#define GET_RES_DEF(name) const decltype(name)& GetRES(){return name;}//IResource

#define MAP_DEF(buf) \
virtual bool Map(void** d)final{	return SUCCEEDED(Buf->Map(0, 0, d));}\
virtual void Unmap()final{Buf->Unmap(0,0);}

ACS_EXCEPTION_DEF(VIEW_CREATE_ERR, "VIEW_CREATE_ERR\n");
ACS_EXCEPTION_DEF(SCREEN_TARGET_CREATE_ERR, "SCREEN_TARGET_CREATE_ERR\n");
ACS_EXCEPTION_DEF(RESOURCE_CREATE_ERR, "RESOURCE_CREATE_ERR\n");

namespace {
	DXGI_FORMAT defaultColorFomat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT defaultDepthMakeFomat = DXGI_FORMAT_R32_TYPELESS;
	DXGI_FORMAT defaultDepthResourceFomat = DXGI_FORMAT_R32_FLOAT;
	DXGI_FORMAT defaultDepthFomat = DXGI_FORMAT_D32_FLOAT;

	//アップロード用バッファ作成(インデックス,頂点,インスタンスバッファ)
	void CreateBuffer(
		ID3D12Device* pDevice,
		uint32_t Size,
		ID3D12Resource** Buf) {
		HRESULT hr;
		D3D12_HEAP_PROPERTIES prop = {};
		prop.Type = D3D12_HEAP_TYPE_UPLOAD;
		prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		prop.CreationNodeMask = 1;
		prop.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC rdesc = {};
		rdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		rdesc.Alignment = 0;
		rdesc.Width = Size;
		rdesc.Height = 1;
		rdesc.DepthOrArraySize = 1;
		rdesc.MipLevels = 1;
		rdesc.Format = DXGI_FORMAT_UNKNOWN;
		rdesc.SampleDesc.Count = 1;
		rdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		rdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		hr = pDevice->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &rdesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(Buf));
		if FAILED(hr)throw(RESOURCE_CREATE_ERR());
	}

}

namespace  acex{
	namespace draw{
		struct MTexture :acs::IACS {
			//out 前回の状態 in 遷移する状態
			virtual D3D12_RESOURCE_STATES LastState(D3D12_RESOURCE_STATES) = 0;
			virtual ID3D12Resource* Resource() = 0;
		};
		struct MView :acs::IACS {
			virtual MTexture* GetTexture() = 0;
		};

		// 前回の状態と遷移する状態が違うときバリアをセット
		static void BarrierSet(ID3D12GraphicsCommandList* comlist, D3D12_RESOURCE_STATES After, MTexture* texture) {
			D3D12_RESOURCE_BARRIER barrier;
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;// バリアはリソースの状態遷移に対して設置
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = texture->Resource();		// リソースは描画ターゲット
			barrier.Transition.StateBefore = texture->LastState(After);
			barrier.Transition.StateAfter = After;		// 遷移後は描画ターゲット
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			if (barrier.Transition.StateAfter != barrier.Transition.StateBefore) {
				comlist->ResourceBarrier(1, &barrier);
			}
		}

		/* ----- view ----- */
		class MRenderResource :public IRenderResource,public MView {
			ACS_IACS_CHILD;
			CComPtr<ID3D12DescriptorHeap> Heap;
			acs::SIACS<MTexture> PTex;
			
		public:
			MRenderResource(ID3D12Device* pDevice, MTexture* parent,ID3D12Resource* Resource,DXGI_FORMAT fomat):PTex(parent){
				
				HRESULT hr;
				D3D12_DESCRIPTOR_HEAP_DESC desc = {};
				desc.NumDescriptors = 1;
				desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
				desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
				hr = pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&Heap));
				if (FAILED(hr))throw(VIEW_CREATE_ERR());
				D3D12_SHADER_RESOURCE_VIEW_DESC srv = {};
				srv = {};
				srv.Format = fomat;
				srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				srv.Texture2D.MipLevels = 1;
				srv.Texture2D.PlaneSlice = 0;
				srv.Texture2D.MostDetailedMip = 0;
				srv.Texture2D.ResourceMinLODClamp = 0;
				pDevice->CreateShaderResourceView(Resource, &srv, Heap->GetCPUDescriptorHandleForHeapStart());
			}
			MTexture* GetTexture() {
				return PTex;
			}
			ID3D12DescriptorHeap* GetHEAP() {
				return Heap;
			};
		};

		struct MTARGET_INIT_DESC {
			ID3D12Resource* resource;
			D3D12_RESOURCE_STATES state;
			DXGI_FORMAT fomat;
			D3D12_VIEWPORT vp;
			D3D12_RECT rect;
		};
		class MTarget :public ITarget, public MView {
		public:
			virtual ~MTarget() {}
			virtual D3D12_CPU_DESCRIPTOR_HANDLE& GetRD() = 0;
			virtual const MTARGET_INIT_DESC* GetInitDesc() = 0;
		};

		class MDefaultTarget :public MTarget {
			CComPtr<ID3D12DescriptorHeap> Heap;
				acs::SIACS<MTexture> PTex;//リソース
				const MTARGET_INIT_DESC init_desc;
				D3D12_CPU_DESCRIPTOR_HANDLE hnd;
		public:
			ACS_IACS_CHILD;

			MDefaultTarget(ID3D12Device* pDevice, MTexture* itex, MTARGET_INIT_DESC& _desc)
				:PTex(itex), init_desc(_desc) {
				HRESULT hr;
				D3D12_DESCRIPTOR_HEAP_DESC desc = {};
				desc.NumDescriptors = 1;
				desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
				desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
				hr = pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&Heap));
				if (FAILED(hr))throw(VIEW_CREATE_ERR());
				D3D12_RENDER_TARGET_VIEW_DESC rtvd = {};
				rtvd.Format = _desc.fomat;
				rtvd.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
				rtvd.Texture2D.MipSlice = 0;
				rtvd.Texture2D.PlaneSlice = 0;
				pDevice->CreateRenderTargetView(_desc.resource, &rtvd, Heap->GetCPUDescriptorHandleForHeapStart());
				hnd = Heap->GetCPUDescriptorHandleForHeapStart();
			}
	
			virtual ~MDefaultTarget() {}
			D3D12_RESOURCE_STATES GetBarrierDefault() {
				return init_desc.state;
			}
			const MTARGET_INIT_DESC* GetInitDesc() throw() {
				return &init_desc;
			}
			D3D12_CPU_DESCRIPTOR_HANDLE& GetRD() {
				return hnd;
			}
			virtual MTexture* GetTexture() final {
				return PTex;
			}
		};
	
		class MScreenTarget:public MTarget ,public MTexture {
			ACS_IACS_CHILD;
			CComPtr<IDXGISwapChain3> s_pSwap;
			CComPtr<ID3D12DescriptorHeap> s_pHeap;
			CComPtr<ID3D12Resource> s_pResource[2];
			uint16_t r_index = 0;

			D3D12_RESOURCE_STATES last_state = D3D12_RESOURCE_STATE_PRESENT;

			MTARGET_INIT_DESC init_desc;
			D3D12_CPU_DESCRIPTOR_HANDLE hnd;
			void InitScreen(ID3D12Device* d3device, const SIZE& size){
				HRESULT hr;
				hr = s_pSwap->GetBuffer(0, IID_PPV_ARGS(&s_pResource[0]));
				if (FAILED(hr))throw(SCREEN_TARGET_CREATE_ERR());
				hr = s_pSwap->GetBuffer(1, IID_PPV_ARGS(&s_pResource[1]));
				if (FAILED(hr))throw(SCREEN_TARGET_CREATE_ERR());
				r_index = s_pSwap->GetCurrentBackBufferIndex();
				{
					init_desc.fomat = DXGI_FORMAT_R8G8B8A8_UNORM;
					init_desc.resource = s_pResource[r_index];
					init_desc.state = D3D12_RESOURCE_STATE_PRESENT;
					init_desc.vp.Width = static_cast<float>(size.Width);
					init_desc.vp.Height = static_cast<float>(size.Height);
					init_desc.vp.TopLeftX = init_desc.vp.TopLeftY = 0.0f;
					init_desc.vp.MinDepth = 0.0f;
					init_desc.vp.MaxDepth = 1.0f;
					init_desc.rect.top = init_desc.rect.left = 0;
					init_desc.rect.right = size.Width;
					init_desc.rect.bottom = size.Height;
				}

				D3D12_DESCRIPTOR_HEAP_DESC desc = {};
				desc.NumDescriptors = 2;
				desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
				desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
				hr = d3device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&s_pHeap));
				if (FAILED(hr))throw(SCREEN_TARGET_CREATE_ERR());
				D3D12_RENDER_TARGET_VIEW_DESC rtvd = {};
				rtvd.Format = init_desc.fomat;
				rtvd.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
				rtvd.Texture2D.MipSlice = 0;
				rtvd.Texture2D.PlaneSlice = 0;
				hnd = s_pHeap->GetCPUDescriptorHandleForHeapStart();
				d3device->CreateRenderTargetView(s_pResource[0], &rtvd, hnd);
				hnd.ptr += DescriptorSize_RTV;
				d3device->CreateRenderTargetView(s_pResource[1], &rtvd, hnd);
				hnd = s_pHeap->GetCPUDescriptorHandleForHeapStart();
				hnd.ptr += DescriptorSize_RTV * r_index;
			
			}
		public:
			MScreenTarget(IDXGIFactory2* DxFact, ID3D12Device* d3device,
				ID3D12CommandQueue* comque,const INIT_DESC* _desc){
				HRESULT hr;
			
				CComPtr<IDXGISwapChain1> pSwap;
				DXGI_SWAP_CHAIN_DESC1 sd = {};
				sd.Width = _desc->Size.Width;
				sd.Height = _desc->Size.Height;
				sd.Format = defaultColorFomat;
				sd.Stereo = false;
				sd.SampleDesc.Count = 1;
				sd.SampleDesc.Quality = 0;
				sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
				sd.BufferCount = 2;
				sd.Scaling = DXGI_SCALING_STRETCH;
				sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
				sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
				sd.Flags = 0;

				DXGI_SWAP_CHAIN_FULLSCREEN_DESC sfd;
				sfd.RefreshRate.Numerator = 1;
				sfd.RefreshRate.Denominator = 60;
				sfd.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
				sfd.Scaling = DXGI_MODE_SCALING_CENTERED;
				sfd.Windowed = true;

				hr = DxFact->CreateSwapChainForHwnd(comque, _desc->hWnd, &sd, &sfd, nullptr, &pSwap);
				if (FAILED(hr))throw(SCREEN_TARGET_CREATE_ERR());

				hr = pSwap->QueryInterface(IID_PPV_ARGS(&s_pSwap));
				if (FAILED(hr))throw(SCREEN_TARGET_CREATE_ERR());
				InitScreen(d3device, _desc->Size);
			}
			virtual ~MScreenTarget() {}

			const MTARGET_INIT_DESC* GetInitDesc() throw() {
				return &init_desc;
			}
			D3D12_CPU_DESCRIPTOR_HANDLE& GetRD() {
				return hnd;
			}
			virtual bool ACS_TCALL ResizeScreenTarget(ID3D12Device* d3device, const SIZE& size) final {
				HRESULT hr;
				s_pResource[0].Release();
				s_pResource[1].Release();
				s_pHeap.Release();

				hr = s_pSwap->ResizeBuffers(2, size.Width, size.Height, defaultColorFomat, NULL);

				InitScreen(d3device, size);
				return true;
			}
			bool Present(int interval) {
				bool r =  SUCCEEDED(s_pSwap->Present(interval, 0));
				r_index = s_pSwap->GetCurrentBackBufferIndex();
				init_desc.resource = s_pResource[r_index];
				hnd.ptr = s_pHeap->GetCPUDescriptorHandleForHeapStart().ptr;
				hnd.ptr += (DescriptorSize_RTV * r_index);
				return r;
			}

			MTexture* GetTexture() {
				return this;
			}
			D3D12_RESOURCE_STATES LastState(D3D12_RESOURCE_STATES ls) {
				D3D12_RESOURCE_STATES out = last_state;
				last_state = ls;
				return out;
			}
			virtual ID3D12Resource* Resource() final {
				return s_pResource[r_index];
			}
		};

		class MDepthStencil :public IDepthStencil,public MView{
			ACS_IACS_CHILD;
			CComPtr<ID3D12DescriptorHeap> Heap;
			acs::SIACS<MTexture> PTex;//リソース
		public:
			MDepthStencil(ID3D12Device* pDevice, MTexture* parent, ID3D12Resource* Resource, DXGI_FORMAT fomat) :PTex(parent) {
				HRESULT hr;
				D3D12_DESCRIPTOR_HEAP_DESC desc = {};
				desc.NumDescriptors = 1;
				desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
				desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
				desc.NodeMask = 0;
				hr = pDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&Heap));
				if (FAILED(hr))throw(VIEW_CREATE_ERR());
				D3D12_DEPTH_STENCIL_VIEW_DESC srvd = {};
				srvd.Format = fomat;
				srvd.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
				srvd.Flags = D3D12_DSV_FLAG_NONE;
				srvd.Texture2D.MipSlice = 0;
				pDevice->CreateDepthStencilView(Resource, &srvd, Heap->GetCPUDescriptorHandleForHeapStart());
			}
			GET_HEAP_DEF(Heap);

			MTexture* GetTexture() {
				return PTex;
			}
		};

		/* -----  Resource ----- */
		class MResource {
		public:
			virtual bool Map(void**) = 0;
			virtual void Unmap() = 0;
		};

		class MIndex : public MResource, public IIndex {
			ACS_IACS_CHILD;
			GET_DESC(INDEXBUFFER_DESC);

			CComPtr<ID3D12Resource>Buf;
			D3D12_INDEX_BUFFER_VIEW data;
		public:
			MIndex(
				ID3D12Device* pDevice,
				const RESOURCE_DESC& pdesc,
				const void* pData) {
				auto _desc = static_cast<const INDEXBUFFER_DESC*>(pdesc.desc);
				desc = *_desc;
				UINT StrideBytes = desc.fmt;
				HRESULT hr;
				{
					D3D12_HEAP_PROPERTIES prop = {};
					prop.Type = D3D12_HEAP_TYPE_UPLOAD;
					prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
					prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
					prop.CreationNodeMask = 1;
					prop.VisibleNodeMask = 1;

					D3D12_RESOURCE_DESC rdesc = {};
					rdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
					rdesc.Alignment = 0;
					rdesc.Width = StrideBytes * desc.ArraySize;
					rdesc.Height = 1;
					rdesc.DepthOrArraySize = 1;
					rdesc.MipLevels = 1;
					rdesc.Format = DXGI_FORMAT_UNKNOWN;
					rdesc.SampleDesc.Count = 1;
					rdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
					rdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
					hr = pDevice->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &rdesc,
						D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
						IID_PPV_ARGS(&Buf));
					if FAILED(hr)throw(RESOURCE_CREATE_ERR());

					if (pData) {
						UINT8* p;
						hr = Buf->Map(0, nullptr, reinterpret_cast<void**>(&p));
						if FAILED(hr)throw(RESOURCE_CREATE_ERR());
						memcpy(p, pData, StrideBytes * desc.ArraySize);
						Buf->Unmap(0, nullptr);
					}
				}
		
				data.BufferLocation = Buf->GetGPUVirtualAddress();
				data.SizeInBytes = StrideBytes * desc.ArraySize;
				data.Format = (desc.fmt == IFMT_U16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
			}

			MAP_DEF(Buf);
			D3D12_INDEX_BUFFER_VIEW& GetIBV() {
				return data;
			};
		};

		class MVertex :public MResource, public IVPosition, public IVColor, public IVUv, public IVNormal, public IIWorld, public IITexState {
			ACS_IACS_CHILD;
			bool vertex = true;
			union {
				VERTEXBUFFER_DESC ver;
				INSTANCEBUFFER_DESC ins;
			}desc;
			public: virtual void ACS_TCALL GetDesc(VERTEXBUFFER_DESC* out) { *out = desc.ver; };
	//		public: virtual void ACS_TCALL GetDesc(INSTANCEBUFFER_DESC* out) { *out = desc.ins; };

			CComPtr<ID3D12Resource>Buf;
			D3D12_VERTEX_BUFFER_VIEW data;
		public:
			MVertex(
				ID3D12Device* pDevice,
				const RESOURCE_DESC& pdesc,
				const void* pSource)
			{
				HRESULT hr;
				UINT BufferSize;
				UINT StrideSizeofByte;
				switch (pdesc.type)
				{
				case RESOURCE_TYPE_VERTEXPOS:
					StrideSizeofByte = sizeof(VERTEX_POSITION);
					goto ON_VERTEX;
				case RESOURCE_TYPE_VERTEXCOLOR:
					StrideSizeofByte = sizeof(VERTEX_COLOR);
					goto ON_VERTEX;
				case RESOURCE_TYPE_VERTEXUV:
					StrideSizeofByte = sizeof(VERTEX_UV);
					goto ON_VERTEX;
				case RESOURCE_TYPE_VERTEXNORMAL:
					StrideSizeofByte = sizeof(VERTEX_NORMAL);
					goto ON_VERTEX;
				case RESOURCE_TYPE_INSWORLD:
					StrideSizeofByte = sizeof(INS_WORLD);
					goto ON_INSTANCE;
				case RESOURCE_TYPE_INSTEXSTATE:
					StrideSizeofByte = sizeof(INS_TEXSTATE);
					goto ON_INSTANCE;
				default:
					throw(RESOURCE_CREATE_ERR());
				}
			ON_VERTEX: 
				{
					auto _desc = static_cast<const VERTEXBUFFER_DESC*>(pdesc.desc);
					desc.ver = *_desc;
					BufferSize = StrideSizeofByte * desc.ver.ArraySize;
					vertex = true;
				}
				goto SET;
			ON_INSTANCE:
				{
					auto _desc = static_cast<const INSTANCEBUFFER_DESC*>(pdesc.desc);
					desc.ins = *_desc;
					BufferSize = StrideSizeofByte * desc.ins.ArraySize;
					vertex = false;
				}
			SET:
				{
					D3D12_HEAP_PROPERTIES prop = {};
					prop.Type = D3D12_HEAP_TYPE_UPLOAD;
					prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
					prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
					prop.CreationNodeMask = 1;
					prop.VisibleNodeMask = 1;

					D3D12_RESOURCE_DESC desc = {};
					desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
					desc.Alignment = 0;
					desc.Width = BufferSize;
					desc.Height = 1;
					desc.DepthOrArraySize = 1;
					desc.MipLevels = 1;
					desc.Format = DXGI_FORMAT_UNKNOWN;
					desc.SampleDesc.Count = 1;
					desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
					desc.Flags = D3D12_RESOURCE_FLAG_NONE;

					hr = pDevice->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &desc,
						D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
						IID_PPV_ARGS(&Buf));
					if FAILED(hr)throw(RESOURCE_CREATE_ERR());
				}
				if (pSource) {
					void * data;
					hr = Buf->Map(0, 0, &data);
					if FAILED(hr)throw(RESOURCE_CREATE_ERR());
					memcpy(data, pSource, BufferSize);
					Buf->Unmap(0, 0);
					if FAILED(hr)throw(RESOURCE_CREATE_ERR());
				}
				data.BufferLocation = Buf->GetGPUVirtualAddress();
				data.SizeInBytes = BufferSize;
				data.StrideInBytes = StrideSizeofByte;
			}
			MAP_DEF(Buf);
			D3D12_VERTEX_BUFFER_VIEW& GetVBV() { return data; }
		};

		class MCamPro :public MResource, public ICamPro {
			ACS_IACS_CHILD;
			GET_DESC(CAMPRO_DESC);
		private:
			CComPtr<ID3D12Resource> Buf;
		public:
			MCamPro(
				ID3D12Device* pDevice,
				const RESOURCE_DESC& pdesc,
				const void* pSource) {
				auto _desc = static_cast<const CAMPRO_DESC*>(pdesc.desc);
				desc = *_desc;
				HRESULT hr;
				{
					// 定数バッファリソース作成
					{
						D3D12_HEAP_PROPERTIES prop;
						prop.Type = D3D12_HEAP_TYPE_UPLOAD;
						prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
						prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
						prop.CreationNodeMask = 1;
						prop.VisibleNodeMask = 1;

						D3D12_RESOURCE_DESC desc;

						desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
						desc.Alignment = 0;
						desc.Width = 256;
						desc.Height = 1;
						desc.DepthOrArraySize = 1;
						desc.MipLevels = 1;
						desc.Format = DXGI_FORMAT_UNKNOWN;
						desc.SampleDesc = { 1, 0 };
						desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
						desc.Flags = D3D12_RESOURCE_FLAG_NONE;

						hr = pDevice->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &desc,
							D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&Buf));
						if FAILED(hr)throw(RESOURCE_CREATE_ERR());
					}
				}
				if (pSource) {
					void * buf;
					hr = Buf->Map(0, 0, &buf);
					if FAILED(hr)throw(RESOURCE_CREATE_ERR());
					memcpy(buf, pSource, sizeof(C_CAMPRO));
					Buf->Unmap(0, 0);
				}
			}
			MAP_DEF(Buf);
			GET_RES_DEF(Buf);
		};
		class MLight :public MResource, public ILight {
			ACS_IACS_CHILD;
			GET_DESC(LIGHT_DESC);
		private:
			CComPtr<ID3D12Resource> Buf;
		public:
			MLight(
				ID3D12Device* pDevice,
				const RESOURCE_DESC& pdesc,
				const void* pSource) {
				auto _desc = static_cast<const LIGHT_DESC*>(pdesc.desc);
				desc = *_desc;
				HRESULT hr;
				{
					// 定数バッファリソースを作成
					{
						D3D12_HEAP_PROPERTIES prop;
						prop.Type = D3D12_HEAP_TYPE_UPLOAD;
						prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
						prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
						prop.CreationNodeMask = 1;
						prop.VisibleNodeMask = 1;

						D3D12_RESOURCE_DESC desc;

						desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
						desc.Alignment = 0;
						desc.Width = 256;
						desc.Height = 1;
						desc.DepthOrArraySize = 1;
						desc.MipLevels = 1;
						desc.Format = DXGI_FORMAT_UNKNOWN;
						desc.SampleDesc = { 1, 0 };
						desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
						desc.Flags = D3D12_RESOURCE_FLAG_NONE;

						hr = pDevice->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &desc,
							D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&Buf));
						if FAILED(hr)throw(RESOURCE_CREATE_ERR());
					}
				}
				if (pSource) {
					void * buf;
					hr = Buf->Map(0, 0, &buf);
					if FAILED(hr)throw(RESOURCE_CREATE_ERR());
					memcpy(buf, pSource, sizeof(C_LIGHT));
					Buf->Unmap(0, 0);
				}
			}
			MAP_DEF(Buf);
			GET_RES_DEF(Buf);
		};

		class MTexture2D :public ITexture2D, public MTexture {
			ACS_IACS_CHILD;
			GET_DESC(TEXTURE2D_DESC);
		private:
			CComPtr<ID3D12Resource> Buf;
			DXGI_FORMAT Fomat;
			uint32_t access;
			D3D12_PLACED_SUBRESOURCE_FOOTPRINT pPlace;

			D3D12_RESOURCE_STATES mState;
		public:
			MTexture2D(
				IDraw* Draw,
				ID3D12Device* pDevice,
				ID3D12CommandQueue* comque,
				ID3D12CommandAllocator* comalc,
				ID3D12GraphicsCommandList* comlist,
				const RESOURCE_DESC& pdesc,
				const void* pSource) {
				access = pdesc.AccessFlag;
				auto _desc = static_cast<const TEXTURE2D_DESC*>(pdesc.desc);
				desc = *_desc;
				CComPtr<ID3D12Resource> UpdBuffer;
				CComPtr<ID3D12Resource> Texture;
				HRESULT hr;
				
				
				//フォーマット設定---
				
				//適応するフォーマットなし
				if ((desc.useflag & TEXUSE_DEPTHSTENCIL) && (desc.useflag & TEXUSE_TARGET))throw(RESOURCE_CREATE_ERR());
				DXGI_FORMAT fmt;
				if (desc.useflag & TEXUSE_DEPTHSTENCIL) {
					fmt = defaultDepthMakeFomat;
					Fomat = defaultDepthFomat;
				}
				else {
					fmt = defaultColorFomat;
					Fomat = defaultColorFomat;
				}

				//リソースの初期ステータス設定---
				if (desc.useflag & TEXUSE_DEPTHSTENCIL) {
					mState =  D3D12_RESOURCE_STATE_DEPTH_WRITE;
				}
				else if (desc.useflag & TEXUSE_TARGET) {
					if (desc.useflag & TEXUSE_RENDER_RESOURCE) {
						mState = D3D12_RESOURCE_STATE_GENERIC_READ;
					}
					else {
						mState = D3D12_RESOURCE_STATE_RENDER_TARGET;
					}
				}
				else {
					mState = D3D12_RESOURCE_STATE_GENERIC_READ;
				}

				UINT RowCount;
				UINT64 RowSize;
				UINT64 UpdBufferBytes;

				//メモリ確保
				{
					D3D12_HEAP_PROPERTIES prop;
					D3D12_RESOURCE_DESC rdesc;

					rdesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
					rdesc.Width = desc.Width;
					rdesc.Height = desc.Height;
					rdesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
					rdesc.Format = fmt;
					rdesc.Alignment = 0;
					rdesc.DepthOrArraySize = 1;
					rdesc.MipLevels = 1;
					rdesc.SampleDesc = { 1, 0 };
					rdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
					if(desc.useflag & TEXUSE_DEPTHSTENCIL)rdesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
					if (desc.useflag & TEXUSE_TARGET)rdesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

					if (!(desc.useflag & TEXUSE_DEPTHSTENCIL)) {
						pDevice->GetCopyableFootprints(
							&rdesc,
							0,
							1,
							0,
							&pPlace,
							&RowCount,
							&RowSize,
							&UpdBufferBytes);
					}

					//GPUアクセス用テクスチャ作成
					if (pdesc.AccessFlag == RESOURCE_ACCESS_NONE) {
						prop.Type = D3D12_HEAP_TYPE_DEFAULT;
						prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
						prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
						prop.CreationNodeMask = 1;
						prop.VisibleNodeMask = 1;
						float col[4] = { 0,0,0,0 };
						D3D12_CLEAR_VALUE clv = {};
						D3D12_CLEAR_VALUE *optcv = nullptr;
						if (desc.useflag & TEXUSE_DEPTHSTENCIL) {
							clv.Format = Fomat;
							clv.DepthStencil.Depth = 1;
							optcv = &clv;
						}
						else if (desc.useflag & TEXUSE_TARGET) {
							clv.Format = Fomat;
							clv.Color[0] = clv.Color[1] = clv.Color[2] = clv.Color[3] = 0;
							optcv = &clv;	
						}
						hr = pDevice->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &rdesc,
							mState,
							optcv, IID_PPV_ARGS(&Texture));
						if (FAILED(hr))throw(RESOURCE_CREATE_ERR());
					}
					//CPU->GPUアップロード用バッファ作成
					if ((pSource || (pdesc.AccessFlag & RESOURCE_ACCESS_WRITE)) &&
						!(desc.useflag & TEXUSE_DEPTHSTENCIL)) {
						rdesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
						rdesc.Width = UpdBufferBytes;
						rdesc.Height = 1;
						rdesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
						rdesc.Format = DXGI_FORMAT_UNKNOWN;
						rdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
						prop.Type = D3D12_HEAP_TYPE_UPLOAD;
						prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
						prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
						prop.CreationNodeMask = 1;
						prop.VisibleNodeMask = 1;
						hr = pDevice->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &rdesc,
							mState, nullptr, IID_PPV_ARGS(&UpdBuffer));
						if (FAILED(hr))throw(RESOURCE_CREATE_ERR());
					}
				}

				//アップロード用バッファにデータ書き込み
				if (pSource && UpdBuffer) {
					byte * buffer;
					hr = UpdBuffer->Map(0, 0, (void**)&buffer);
					if (FAILED(hr))throw(RESOURCE_CREATE_ERR());
					const byte* source = static_cast<const byte*>(pSource);
					for (UINT i = 0; i < RowCount; i++)
					{
						memcpy(buffer, source, static_cast<size_t>(RowSize));
						buffer += pPlace.Footprint.RowPitch;
						source += RowSize;
					}
					UpdBuffer->Unmap(0, 0);
					if (FAILED(hr))throw(RESOURCE_CREATE_ERR());

					//GPU用バッファにに書き込んだ内容をコピー
					if (Texture) {
						Draw->WaitDrawDone();
						HRESULT hr;
						// コマンドアロケータをリセット
						hr = comalc->Reset();
						if (FAILED(hr))throw(RESOURCE_CREATE_ERR());

						// コマンドリストをリセット
						hr = comlist->Reset(comalc, nullptr);
						if (FAILED(hr))throw(RESOURCE_CREATE_ERR());

						D3D12_RESOURCE_BARRIER barrier = {};
						barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
						barrier.Transition.pResource = Texture;
						barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
						barrier.Transition.StateBefore = mState;
						barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
						comlist->ResourceBarrier(1, &barrier);

						D3D12_TEXTURE_COPY_LOCATION dst;
						D3D12_TEXTURE_COPY_LOCATION src;

						dst.pResource = Texture;
						dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
						dst.SubresourceIndex = 0;

						src.pResource = UpdBuffer;
						src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
						src.PlacedFootprint = pPlace;
						comlist->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

						barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
						barrier.Transition.StateAfter = mState;
						comlist->ResourceBarrier(1, &barrier);


						// コマンドリストをクローズする
						hr = comlist->Close();
						if (FAILED(hr))throw(RESOURCE_CREATE_ERR());

						// コマンドリストを実行する
						ID3D12CommandList* ppCommandLists[] = { comlist };
						comque->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
						Draw->WaitDrawDone();
					}
				}

				if (Texture) {
					Buf = Texture;
				}
				else if(UpdBuffer){
					Buf = UpdBuffer;
				}
				else {
					throw(RESOURCE_CREATE_ERR());
				}
			}

			bool CreateView(ID3D12Device* Device, VIEW_TYPE id, void** out){
				try {
					switch (id)
					{
					case acex::draw::VIEW_RENDER_RESOURCE:
						*out = static_cast<IRenderResource*>(new MRenderResource(Device, this, Buf, (Fomat != defaultDepthFomat) ? Fomat: defaultDepthResourceFomat));
						break;
					case acex::draw::VIEW_TARGET:
						MTARGET_INIT_DESC idesc;
						idesc.fomat = DXGI_FORMAT_R8G8B8A8_UNORM;
						idesc.resource = Buf;
						if (desc.useflag & TEXUSE_RENDER_RESOURCE)idesc.state = D3D12_RESOURCE_STATE_GENERIC_READ;
						else idesc.state = D3D12_RESOURCE_STATE_GENERIC_READ;
						idesc.vp.Width = static_cast<float>(desc.Width);
						idesc.vp.Height = static_cast<float>(desc.Height);
						idesc.vp.TopLeftX = idesc.vp.TopLeftY = 0.0f;
						idesc.vp.MinDepth = 0.0f;
						idesc.vp.MaxDepth = 1.0f;
						idesc.rect.top = idesc.rect.left = 0;
						idesc.rect.right = desc.Width;
						idesc.rect.bottom = desc.Height;
						*out = static_cast<ITarget*>(new MDefaultTarget(Device, this, idesc));
						break;
					case acex::draw::VIEW_DEPTHSTENCIL:
						*out = static_cast<MDepthStencil*>(new MDepthStencil(Device, this, Buf, Fomat));
						break;
					default:
						throw(VIEW_CREATE_ERR());
						break;
					}
				}
				catch (const VIEW_CREATE_ERR& err) {
					OutputDebugStringA(err.what());
					return 0;
				}
				return 1;
			}
			const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& GetCopyInfo() {
				return pPlace;
			}
			GET_RES_DEF(Buf);

			D3D12_RESOURCE_STATES LastState(D3D12_RESOURCE_STATES ls) {
				auto out = mState;
				mState = ls;
				return out;
			}
			virtual ID3D12Resource* Resource() {
				return Buf;
			}
		};

		inline ::D3D_PRIMITIVE_TOPOLOGY CVPRIM(PRIMITIVE_TOPOLOGY tp)throw() {
			switch (tp)
			{
			case PT_TRIANGLELIST:
				return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				break;
			case PT_TRIANGLESTRIP:
				return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
				break;
			default:
				return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				break;
			}
		}
		ACS_EXCEPTION_DEF(DRAW_INIT_ERR, "DRAW_INIT_ERR\n");
		void GetHardwareAdapter(IDXGIFactory4* pFactory, IDXGIAdapter1** ppAdapter, D3D_FEATURE_LEVEL MiniLevel)
		{
			*ppAdapter = nullptr;
			for (UINT adapterIndex = 0; ; ++adapterIndex)
			{
				CComPtr<IDXGIAdapter1> pAdapter = nullptr;
				if (DXGI_ERROR_NOT_FOUND == pFactory->EnumAdapters1(adapterIndex, &pAdapter))
				{
					break;
				}
				if (SUCCEEDED(D3D12CreateDevice(pAdapter, MiniLevel, _uuidof(ID3D12Device), nullptr)))
				{
					*ppAdapter = pAdapter.Detach();
					return;
				}
			}
		}
		class MDraw :public IDraw, public IDrawer, public IUpdater {
			ACS_IACS_CHILD;
		private:
			std::recursive_mutex mut;//更新用

			CComPtr<ID3D12Device> d3device;
			CComPtr<ID3D12CommandQueue> comque;
			CComPtr<IDXGIFactory4> DxFact;

			CComPtr<ID3D12CommandAllocator> comalc;
			CComPtr<ID3D12GraphicsCommandList> comlist;
			CComPtr<ID3D12Fence> Fence;

			acs::SIACS<MScreenTarget> target;
		
			//0 = index u16[4]{0,1,2,3}
			//8 = vertex float[3][4]{}
			//56
			CComPtr<ID3D12Resource>spriteBuffer;
			D3D12_INDEX_BUFFER_VIEW spriteIndexView;
			D3D12_VERTEX_BUFFER_VIEW spriteVertexView;

			CComPtr<ID3D12DescriptorHeap> PointSampler;
			CComPtr<ID3D12DescriptorHeap> LinearSampler;
			CComPtr<ID3D12DescriptorHeap> AnisotropicSampler;
			CHandle DrawDoneEvent;

			CComPtr<ID3D12RootSignature> rootsign;//default
			CComPtr<ID3D12RootSignature> root_tex;//default
			CComPtr<ID3D12RootSignature> root_world;//world
			CComPtr<ID3D12RootSignature> root_worldtex;
			CComPtr<ID3D12RootSignature> root_norworld;
			CComPtr<ID3D12RootSignature> root_norworldtex;

			CComPtr<ID3D12RootSignature> root_sprite;

			CComPtr<ID3D12RootSignature> root_depth;
			CComPtr<ID3D12RootSignature> root_shadow;

			CComPtr<ID3D12PipelineState> pipe[2];
			CComPtr<ID3D12PipelineState> pipe_tex[2];
			CComPtr<ID3D12PipelineState> pipe_world[2];
			CComPtr<ID3D12PipelineState> pipe_worldtex[2];
			CComPtr<ID3D12PipelineState> pipe_worldtex_c[2];

			CComPtr<ID3D12PipelineState> pipe_worldcoltex[2];
			CComPtr<ID3D12PipelineState> pipe_worldcoltex_c[2];

			CComPtr<ID3D12PipelineState> pipe_norworld[2];
			CComPtr<ID3D12PipelineState> pipe_norworldtex[2];

			CComPtr<ID3D12PipelineState> pipe_sprite_c[2];
			CComPtr<ID3D12PipelineState> pipe_sprite_col_c[2];

			CComPtr<ID3D12PipelineState> pipe_depth;
			CComPtr<ID3D12PipelineState> pipe_shadow[2];

			UINT RenderPipelineMode = 0;

		public:
			explicit MDraw(INIT_DESC* desc) {
				HRESULT hr;
	#ifdef _DEBUG
				// デバッグバージョンではデバッグレイヤーを有効化する
				{
					ID3D12Debug* debugController;
					if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
					{
						debugController->EnableDebugLayer();
						debugController->Release();
					}
				}
	#endif
				{
					//デバイス作成----------------------------------------------------------
					{
						IDXGIAdapter1* pAdapter;
						hr = CreateDXGIFactory(IID_PPV_ARGS(&DxFact));
						if (FAILED(hr))throw(DRAW_INIT_ERR());
						if(desc->useWarpDevice)DxFact->EnumWarpAdapter(IID_PPV_ARGS(&pAdapter));
						else GetHardwareAdapter(DxFact, &pAdapter, D3D_FEATURE_LEVEL_11_0);
						if (!pAdapter)throw(DRAW_INIT_ERR());
						hr = D3D12CreateDevice(
							pAdapter,
							D3D_FEATURE_LEVEL_11_0,
							IID_PPV_ARGS(&d3device)
						);
						pAdapter->Release();
						if (FAILED(hr))throw(DRAW_INIT_ERR());
					}
					//コマンドキュー作成----------------------------------------------------------------------------
					{
						D3D12_COMMAND_QUEUE_DESC desc = {};
						desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;		// GPUタイムアウトが有効
						desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;		// 直接コマンドキュー

						hr = d3device->CreateCommandQueue(&desc, IID_PPV_ARGS(&comque));
						if (FAILED(hr))throw(DRAW_INIT_ERR());
					}

					// コマンドアロケータを作成-------------------------------------------------------------------
					hr = d3device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&comalc));
					if (FAILED(hr))throw(DRAW_INIT_ERR());
					// コマンドリスト作成---------------------------------------------------------------------------------
					hr = d3device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, comalc, nullptr, IID_PPV_ARGS(&comlist));
					if (FAILED(hr))throw(DRAW_INIT_ERR());
					hr = comlist->Close();
					if (FAILED(hr))throw(DRAW_INIT_ERR());

					//フェンス作成-------------------------------------------------------------------------------------
					hr = d3device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence));
					if (FAILED(hr))throw(DRAW_INIT_ERR());

				}
			
				//ルート----------------------
				{
					D3D12_RASTERIZER_DESC rasterDesc = {};
					rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
					rasterDesc.CullMode = D3D12_CULL_MODE_BACK;
					rasterDesc.FrontCounterClockwise = false;
					rasterDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
					rasterDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
					rasterDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
					rasterDesc.DepthClipEnable = true;
					rasterDesc.MultisampleEnable = false;
					rasterDesc.AntialiasedLineEnable = false;
					rasterDesc.ForcedSampleCount = 0;
					rasterDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

					D3D12_BLEND_DESC blendDesc = {};
					blendDesc.AlphaToCoverageEnable = false;
					blendDesc.IndependentBlendEnable = false;
					blendDesc.RenderTarget[0].BlendEnable = true;
					blendDesc.RenderTarget[0].LogicOpEnable = false;
					blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
					blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
					blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
					blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
					blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_DEST_ALPHA;
					blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
					blendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
					blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

					D3D12_GRAPHICS_PIPELINE_STATE_DESC pdesc = { {} };
					pdesc.RasterizerState = rasterDesc;
					pdesc.BlendState = blendDesc;
					pdesc.DSVFormat = defaultDepthFomat;
					pdesc.DepthStencilState.DepthEnable = TRUE;
					pdesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
					pdesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
					pdesc.DepthStencilState.StencilEnable = FALSE;
					pdesc.SampleMask = UINT_MAX;
					pdesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
					pdesc.NumRenderTargets = 1;
					pdesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
					pdesc.SampleDesc.Count = 1;
				
					auto CreateRoot = [&](D3D12_ROOT_SIGNATURE_DESC& rsdesc ,ID3D12RootSignature** root) {
						CComPtr<ID3DBlob> blob;
						hr = D3D12SerializeRootSignature(&rsdesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, nullptr);
						if (hr != S_OK)throw(DRAW_INIT_ERR());
						hr = d3device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(root));
						if (hr != S_OK)throw(DRAW_INIT_ERR());
						blob.Release();
					};
					auto CreatePSOA_1 = [&](ID3D12PipelineState** out, D3D12_GRAPHICS_PIPELINE_STATE_DESC& gps) ->void {
						hr = d3device->CreateGraphicsPipelineState(&gps, IID_PPV_ARGS(out));
						if (FAILED(hr))throw(DRAW_INIT_ERR());
					};
					auto CreatePSO = [&](CComPtr<ID3D12PipelineState>* out) ->void {
						pdesc.DepthStencilState.DepthEnable = FALSE;
						hr = d3device->CreateGraphicsPipelineState(&pdesc, IID_PPV_ARGS(&out[0]));
						if (FAILED(hr))throw(DRAW_INIT_ERR());
						pdesc.DepthStencilState.DepthEnable = TRUE;
						hr = d3device->CreateGraphicsPipelineState(&pdesc, IID_PPV_ARGS(&out[1]));
						if (FAILED(hr))throw(DRAW_INIT_ERR());
					};
					auto CreatePSO_1 = [&](CComPtr<ID3D12PipelineState>* out, D3D12_GRAPHICS_PIPELINE_STATE_DESC& gps) ->void {
						gps.DepthStencilState.DepthEnable = FALSE;
						hr = d3device->CreateGraphicsPipelineState(&gps, IID_PPV_ARGS(&out[0]));
						if (FAILED(hr))throw(DRAW_INIT_ERR());
						gps.DepthStencilState.DepthEnable = TRUE;
						hr = d3device->CreateGraphicsPipelineState(&gps, IID_PPV_ARGS(&out[1]));
						if (FAILED(hr))throw(DRAW_INIT_ERR());
					};
					{
						// ルートシグネチャ作成

						D3D12_DESCRIPTOR_RANGE ranges[2];
						ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
						ranges[0].NumDescriptors = 1;
						ranges[0].BaseShaderRegister = 0;
						ranges[0].RegisterSpace = 0;
						ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

						ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
						ranges[1].NumDescriptors = 1;
						ranges[1].BaseShaderRegister = 0;
						ranges[1].RegisterSpace = 0;
						ranges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

						//worldのみ
						{
							D3D12_ROOT_PARAMETER rootParameters[3];

							rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
							rootParameters[0].Constants.Num32BitValues = 16;
							rootParameters[0].Constants.ShaderRegister = 0;
							rootParameters[0].Constants.RegisterSpace = 0;
							rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

							rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
							rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
							rootParameters[1].DescriptorTable.pDescriptorRanges = &ranges[0];
							rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

							rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
							rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
							rootParameters[2].DescriptorTable.pDescriptorRanges = &ranges[1];
							rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

							D3D12_ROOT_SIGNATURE_DESC desc;
							desc.NumParameters = 0;
							desc.pParameters = rootParameters;
							desc.NumStaticSamplers = 0;
							desc.pStaticSamplers = nullptr;
							desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

							CComPtr<ID3DBlob> blob;
							hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, nullptr);
							if (FAILED(hr))throw(DRAW_INIT_ERR());
							hr = d3device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&rootsign));
							if (FAILED(hr))throw(DRAW_INIT_ERR());
							blob.Release();

							desc.NumParameters = 2;
							desc.pParameters = &rootParameters[1];
							hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, nullptr);
							if (FAILED(hr))throw(DRAW_INIT_ERR());
							hr = d3device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&root_tex));
							if (FAILED(hr))throw(DRAW_INIT_ERR());
							blob.Release();

							desc.NumParameters = 1;
							desc.pParameters = rootParameters;
							hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, nullptr);
							if (FAILED(hr))throw(DRAW_INIT_ERR());
							hr = d3device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&root_world));
							if (FAILED(hr))throw(DRAW_INIT_ERR());
							blob.Release();

							desc.NumParameters = 3;
							hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, nullptr);
							if (FAILED(hr))throw(DRAW_INIT_ERR());
							hr = d3device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&root_worldtex));
							if (FAILED(hr))throw(DRAW_INIT_ERR());
							blob.Release();
						}
						//normal付き
						{
							D3D12_ROOT_PARAMETER rootParameters[4];
							size_t i = 0;

							i = 0;
							rootParameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
							rootParameters[i].Constants.Num32BitValues = 16;
							rootParameters[i].Constants.ShaderRegister = 0;
							rootParameters[i].Constants.RegisterSpace = 0;
							rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
							i++;
							rootParameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
							rootParameters[i].Constants.Num32BitValues = 16;
							rootParameters[i].Constants.ShaderRegister = 1;
							rootParameters[i].Constants.RegisterSpace = 0;
							rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
							i++;
							rootParameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
							rootParameters[i].DescriptorTable.NumDescriptorRanges = 1;
							rootParameters[i].DescriptorTable.pDescriptorRanges = &ranges[0];
							rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
							i++;
							rootParameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
							rootParameters[i].DescriptorTable.NumDescriptorRanges = 1;
							rootParameters[i].DescriptorTable.pDescriptorRanges = &ranges[1];
							rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

							D3D12_ROOT_SIGNATURE_DESC desc;
							desc.NumParameters = 2;
							desc.pParameters = rootParameters;
							desc.NumStaticSamplers = 0;
							desc.pStaticSamplers = nullptr;
							desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

							CComPtr<ID3DBlob> blob;
							hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, nullptr);
							if (FAILED(hr))throw(DRAW_INIT_ERR());
							hr = d3device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&root_norworld));
							if (FAILED(hr))throw(DRAW_INIT_ERR());
							blob.Release();

							desc.NumParameters = 4;

							hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, nullptr);
							if (FAILED(hr))throw(DRAW_INIT_ERR());
							hr = d3device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&root_norworldtex));
							if (FAILED(hr))throw(DRAW_INIT_ERR());
							blob.Release();
						}
						//sprite_root
						{
							D3D12_ROOT_PARAMETER rootParameters[3];
							size_t i = 0;

							i = 0;
							rootParameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
							rootParameters[i].Constants.Num32BitValues = 16;
							rootParameters[i].Constants.ShaderRegister = 0;
							rootParameters[i].Constants.RegisterSpace = 0;
							rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_GEOMETRY;
							i++;
							rootParameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
							rootParameters[i].DescriptorTable.NumDescriptorRanges = 1;
							rootParameters[i].DescriptorTable.pDescriptorRanges = &ranges[0];
							rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
							i++;
							rootParameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
							rootParameters[i].DescriptorTable.NumDescriptorRanges = 1;
							rootParameters[i].DescriptorTable.pDescriptorRanges = &ranges[1];
							rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

							D3D12_ROOT_SIGNATURE_DESC desc;
							desc.NumParameters = 3;
							desc.pParameters = rootParameters;
							desc.NumStaticSamplers = 0;
							desc.pStaticSamplers = nullptr;
							desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

							CreateRoot(desc, &root_sprite);
						}
						//depth_root
						{
							const uint32_t parmCnt = 1;
							D3D12_ROOT_PARAMETER rootParameters[parmCnt];
							size_t i = 0;

							i = 0;
							rootParameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
							rootParameters[i].Constants.Num32BitValues = 16;
							rootParameters[i].Constants.ShaderRegister = 0;
							rootParameters[i].Constants.RegisterSpace = 0;
							rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

							D3D12_ROOT_SIGNATURE_DESC desc;
							desc.NumParameters = parmCnt;
							desc.pParameters = rootParameters;
							desc.NumStaticSamplers = 0;
							desc.pStaticSamplers = nullptr;
							desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

							CreateRoot(desc, &root_depth);
						}
						//shadow_root
						{
							D3D12_ROOT_PARAMETER rootParameters[5];
							size_t i = 0;

							i = 0;
							rootParameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
							rootParameters[i].Constants.Num32BitValues = 16;
							rootParameters[i].Constants.ShaderRegister = 0;
							rootParameters[i].Constants.RegisterSpace = 0;
							rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
							i++;
							rootParameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
							rootParameters[i].Constants.Num32BitValues = 16;
							rootParameters[i].Constants.ShaderRegister = 1;
							rootParameters[i].Constants.RegisterSpace = 0;
							rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
							i++;
							rootParameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
							rootParameters[i].Constants.Num32BitValues = 16;
							rootParameters[i].Constants.ShaderRegister = 0;
							rootParameters[i].Constants.RegisterSpace = 0;
							rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
							i++;
							rootParameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
							rootParameters[i].DescriptorTable.NumDescriptorRanges = 1;
							rootParameters[i].DescriptorTable.pDescriptorRanges = &ranges[0];
							rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
							i++;
							rootParameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
							rootParameters[i].DescriptorTable.NumDescriptorRanges = 1;
							rootParameters[i].DescriptorTable.pDescriptorRanges = &ranges[1];
							rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

							D3D12_ROOT_SIGNATURE_DESC desc;
							desc.NumParameters = 5;
							desc.pParameters = rootParameters;
							desc.NumStaticSamplers = 0;
							desc.pStaticSamplers = nullptr;
							desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

							CreateRoot(desc, &root_shadow);
						}

						// PSOを作成

						//def
						{
							D3D12_INPUT_ELEMENT_DESC elementDescs[] = {
								{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
								{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
								{ "WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
							};

							pdesc.InputLayout = { elementDescs, _countof(elementDescs) };
							pdesc.pRootSignature = rootsign;
							pdesc.VS = { EasyVS, _countof(EasyVS) };
							pdesc.PS = { EasyPS, _countof(EasyPS) };
							CreatePSO(pipe);
						}
						//Tex
						{
							D3D12_INPUT_ELEMENT_DESC elementDescs[] = {
							{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
							{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
							{ "WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
							{ "WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
							{ "WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
							{ "WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
							};

							pdesc.InputLayout = { elementDescs, _countof(elementDescs) };
							pdesc.pRootSignature = root_tex;
							pdesc.VS = { TextureVS, _countof(TextureVS) };
							pdesc.PS = { TexturePS, _countof(TexturePS) };
							CreatePSO(pipe_tex);
						}
						//world
						{
							D3D12_INPUT_ELEMENT_DESC elementDescs[] = {
								{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
								{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
								{ "WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
							};

							pdesc.InputLayout = { elementDescs, _countof(elementDescs) };
							pdesc.pRootSignature = root_world;
							pdesc.VS = { WorldVS, _countof(WorldVS) };
							pdesc.PS = { EasyPS, _countof(EasyPS) };
							CreatePSO(pipe_world);
						}
						//WorldTex
						{
							D3D12_INPUT_ELEMENT_DESC elementDescs[] = {
								{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
								{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
								{ "WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "TEXOFFS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
							};

							pdesc.InputLayout = { elementDescs, _countof(elementDescs) };
							pdesc.pRootSignature = root_worldtex;
							pdesc.VS = { WorldTexVS, sizeof(WorldTexVS) };
							pdesc.PS = { TexturePS, sizeof(TexturePS) };
							CreatePSO(pipe_worldtex);

							pdesc.VS = { WorldTexVS, sizeof(WorldTexVS) };
							pdesc.PS = { TexturePS_C, sizeof(TexturePS_C) };
							CreatePSO(pipe_worldtex_c);
						}
						//WorldColTex
						{
							D3D12_INPUT_ELEMENT_DESC elementDescs[] = {
						{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
						{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
						{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
						{ "WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
						{ "WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
						{ "WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
						{ "WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
						{ "TEXOFFS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 4, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
							};
							pdesc.InputLayout = { elementDescs, _countof(elementDescs) };
							pdesc.pRootSignature = root_worldtex;
							pdesc.VS = { WorldColTexVS, sizeof(WorldColTexVS) };
							pdesc.PS = { WorldColTexPS, sizeof(WorldColTexPS) };
							CreatePSO(pipe_worldcoltex);
							pdesc.PS = { WorldColTexPS_C, sizeof(WorldColTexPS_C) };
							CreatePSO(pipe_worldcoltex_c);
						}
						//NormalWorld
						{
							D3D12_INPUT_ELEMENT_DESC elementDescs[] = {
								{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
								{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
								{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
								{ "WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 }
							};

							pdesc.InputLayout = { elementDescs, _countof(elementDescs) };
							pdesc.pRootSignature = root_norworld;
							pdesc.VS = { NormalWorldVS, sizeof(NormalWorldVS) };
							pdesc.PS = { NormalPS, sizeof(NormalPS) };
							CreatePSO(pipe_norworld);
						}
						//NormalWorldTex
						{
							D3D12_INPUT_ELEMENT_DESC elementDescs[] = {
								{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
								{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
								{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
								{ "WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "TEXOFFS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 4, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
							};

							pdesc.InputLayout = { elementDescs, _countof(elementDescs) };
							pdesc.pRootSignature = root_norworldtex;
							pdesc.VS = { NormalWorldTexVS, sizeof(NormalWorldTexVS) };
							pdesc.PS = { NormalTexPS, sizeof(NormalTexPS) };
							CreatePSO(pipe_norworldtex);
						}
						//Sprite
						{
							D3D12_INPUT_ELEMENT_DESC elementDescs[] = {
								{ "WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "TEXOFFS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
							};
							D3D12_GRAPHICS_PIPELINE_STATE_DESC pdesc = { {} };
							pdesc.RasterizerState = rasterDesc;
							pdesc.BlendState = blendDesc;
							pdesc.DSVFormat = defaultDepthFomat;
							pdesc.DepthStencilState.DepthEnable = TRUE;
							pdesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
							pdesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
							pdesc.DepthStencilState.StencilEnable = FALSE;
							pdesc.SampleMask = UINT_MAX;
							pdesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
							pdesc.NumRenderTargets = 1;
							pdesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
							pdesc.SampleDesc.Count = 1;
							pdesc.InputLayout = { elementDescs, _countof(elementDescs) };
							pdesc.pRootSignature = root_sprite;
							pdesc.VS = { SpriteVS, sizeof(SpriteVS) };
						
							pdesc.GS = { SpriteGS, sizeof(SpriteGS) };
							pdesc.PS = { TexturePS_C, sizeof(TexturePS_C) };
							CreatePSO_1(pipe_sprite_c, pdesc);
						}
						//SpriteCol
						{
							D3D12_INPUT_ELEMENT_DESC elementDescs[] = {
								{ "WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "TEXOFFS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
							};
							D3D12_GRAPHICS_PIPELINE_STATE_DESC pdesc = { {} };
							pdesc.RasterizerState = rasterDesc;
							pdesc.BlendState = blendDesc;
							pdesc.DSVFormat = defaultDepthFomat;
							pdesc.DepthStencilState.DepthEnable = TRUE;
							pdesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
							pdesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
							pdesc.DepthStencilState.StencilEnable = FALSE;
							pdesc.SampleMask = UINT_MAX;
							pdesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
							pdesc.NumRenderTargets = 1;
							pdesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
							pdesc.SampleDesc.Count = 1;
							pdesc.InputLayout = { elementDescs, _countof(elementDescs) };
							pdesc.pRootSignature = root_sprite;
							pdesc.VS = { SpriteColVS, sizeof(SpriteColVS) };
							pdesc.GS = { SpriteColGS, sizeof(SpriteColGS) };
							pdesc.PS = { WorldColTexPS_C, sizeof(WorldColTexPS_C) };
							CreatePSO_1(pipe_sprite_col_c, pdesc);
						}
						//depth
						{
							D3D12_INPUT_ELEMENT_DESC elementDescs[] = {
								{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
								{ "WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
							};
							D3D12_RASTERIZER_DESC drasterDesc = {};
							drasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
							drasterDesc.CullMode = D3D12_CULL_MODE_BACK;
							drasterDesc.FrontCounterClockwise = false;
							drasterDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
							drasterDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
							drasterDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
							drasterDesc.DepthClipEnable = true;
							drasterDesc.MultisampleEnable = false;
							drasterDesc.AntialiasedLineEnable = false;
							drasterDesc.ForcedSampleCount = 0;
							drasterDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
							D3D12_GRAPHICS_PIPELINE_STATE_DESC psdesc = { };
							psdesc.RasterizerState = drasterDesc;
							psdesc.BlendState = blendDesc;
							psdesc.DSVFormat = defaultDepthFomat;
							psdesc.DepthStencilState.DepthEnable = TRUE;
							psdesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
							psdesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
							psdesc.DepthStencilState.StencilEnable = FALSE;
							psdesc.SampleMask = UINT_MAX;
							psdesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
							psdesc.NumRenderTargets = 0;
							psdesc.InputLayout = { elementDescs, _countof(elementDescs) };
							psdesc.pRootSignature = root_depth;
							psdesc.SampleDesc.Count = 1;
							psdesc.VS = { DepthVS, sizeof(DepthVS) };
						//	psdesc.PS = { DepthPS, sizeof(DepthPS) };
							CreatePSOA_1(&pipe_depth, psdesc);
						}
						//shadow
						{
							D3D12_INPUT_ELEMENT_DESC elementDescs[] = {
								{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
								{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
								{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
								{ "WORLD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "WORLD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "WORLD", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
								{ "WORLD", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 }
							};
							D3D12_RASTERIZER_DESC drasterDesc = {};
							drasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
							drasterDesc.CullMode = D3D12_CULL_MODE_BACK;
							drasterDesc.FrontCounterClockwise = false;
							drasterDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
							drasterDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
							drasterDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
							drasterDesc.DepthClipEnable = true;
							drasterDesc.MultisampleEnable = false;
							drasterDesc.AntialiasedLineEnable = false;
							drasterDesc.ForcedSampleCount = 0;
							drasterDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
							D3D12_GRAPHICS_PIPELINE_STATE_DESC psdesc = { {} };
							psdesc.RasterizerState = drasterDesc;
							psdesc.BlendState = blendDesc;
							psdesc.DSVFormat = defaultDepthFomat;
							psdesc.DepthStencilState.DepthEnable = TRUE;
							psdesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
							psdesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
							psdesc.DepthStencilState.StencilEnable = FALSE;
							psdesc.SampleMask = UINT_MAX;
							psdesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
							psdesc.NumRenderTargets = 1;
							psdesc.RTVFormats[0] = defaultColorFomat;
							psdesc.SampleDesc.Count = 1;
							psdesc.InputLayout = { elementDescs, _countof(elementDescs) };
							psdesc.pRootSignature = root_shadow;
							psdesc.VS = { ShadowMapVS, sizeof(ShadowMapVS) };
							psdesc.PS = { ShadowMapPS, sizeof(ShadowMapPS) };
							CreatePSO_1(pipe_shadow, psdesc);
						}
					}
				}

				::DescriptorSize_RTV
					= d3device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
				{
					target.Attach(new MScreenTarget(DxFact, d3device, comque, desc));
				}

				//-------------------------------------------------------------------------------
				auto CreateSampler = [&](ID3D12DescriptorHeap** ppOut, D3D12_FILTER Filter)->void {
					D3D12_DESCRIPTOR_HEAP_DESC descHeapSampler = {};
					descHeapSampler.NumDescriptors = 1;
					descHeapSampler.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
					descHeapSampler.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
					hr = d3device->CreateDescriptorHeap(&descHeapSampler, __uuidof(ID3D12DescriptorHeap), (void**)ppOut);
					if (FAILED(hr))throw(DRAW_INIT_ERR());

					D3D12_SAMPLER_DESC samplerDesc;
					ZeroMemory(&samplerDesc, sizeof(D3D12_SAMPLER_DESC));
					samplerDesc.Filter = Filter;
					samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
					samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
					samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
					samplerDesc.MinLOD = 0;
					samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
					samplerDesc.MipLODBias = 0.0f;
					samplerDesc.MaxAnisotropy = 1;
					samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
					d3device->CreateSampler(&samplerDesc,
						(*ppOut)->GetCPUDescriptorHandleForHeapStart());
					if (FAILED(hr))throw(DRAW_INIT_ERR());
				};
			
				CreateSampler(&PointSampler, D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR);
				CreateSampler(&LinearSampler, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
				CreateSampler(&AnisotropicSampler, D3D12_FILTER_ANISOTROPIC);

				//静的リソース作成
				{
					//インデックスバッファ
					CreateBuffer(d3device, 2 * 4 + 4 * 3 * 4, &spriteBuffer);
					uint16_t index[] = { 0,1,2,3 };
					VERTEX_POSITION pos[] = { {-0.5,-0.5,0 },{ 0.5,-0.5,0 },{ -0.5,0.5,0 },{ 0.5,0.5,0 }	};

					unsigned char* buffer;
					spriteBuffer->Map(0, nullptr, (void**)&buffer);
					auto address = spriteBuffer->GetGPUVirtualAddress();

					spriteIndexView.BufferLocation = address;
					spriteIndexView.Format = DXGI_FORMAT_R16_UINT;
					spriteIndexView.SizeInBytes = 2;
					memcpy(buffer, index, 8);
					buffer += 8;
					address += 8;

					spriteVertexView.BufferLocation = address;
					spriteVertexView.StrideInBytes = 4 * 3;
					spriteVertexView.SizeInBytes = 4 * 3 * 4;
					memcpy(buffer, pos, 4 * 3 * 4);
					buffer += 4 * 3 * 4;
					address += 4 * 3 * 4;
			
					spriteBuffer->Unmap(0, nullptr);
				}

				DrawDoneEvent.Attach(CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS));
				if (!DrawDoneEvent)throw(DRAW_INIT_ERR());
			
			}
			virtual ~MDraw() {
				WaitDrawDone();
			}

			bool isEnable()throw() {
				if (nullptr == d3device)return false;
				return SUCCEEDED(d3device->GetDeviceRemovedReason());
			}

			bool ACS_TCALL GetScreenTarget(ITarget** t) {
				target.Addref();
				*t = target;
				return true;
			}
			virtual bool ACS_TCALL ResizeScreenTarget(const SIZE& size) final{
				WaitDrawDone();
				try
				{
					target->ResizeScreenTarget(d3device, size);
				}
				catch (const std::exception& ex)
				{
					OutputDebugStringA(ex.what());
					return false;
				}
				return true;
			}

			template<typename Out, typename Create, typename ...Arg>
			Out* CRES(Arg ...arg){
				return static_cast<Out*>(new Create(arg...));
			}
			virtual bool ACS_TCALL CreateResource(
				const RESOURCE_DESC& desc,
				const void* pData,
				void** ppOut)final {
				try{
					switch (desc.type)
					{
					case RESOURCE_TYPE_INDEX:
						*ppOut = CRES<IIndex, MIndex>(d3device, desc, pData);
						break;
					case RESOURCE_TYPE_VERTEXPOS:
						*ppOut = CRES<IVPosition, MVertex>(d3device, desc, pData);
						break;
					case RESOURCE_TYPE_VERTEXCOLOR:
						*ppOut = CRES<IVColor, MVertex>(d3device, desc, pData);
						break;
					case RESOURCE_TYPE_VERTEXUV:
						*ppOut = CRES<IVUv, MVertex>(d3device, desc, pData);
						break;
					case RESOURCE_TYPE_VERTEXNORMAL:
						*ppOut = CRES<IVNormal, MVertex>(d3device, desc, pData);
						break;
					case RESOURCE_TYPE_INSWORLD:
						*ppOut = CRES<IIWorld, MVertex>(d3device, desc, pData);
						break;
					case RESOURCE_TYPE_INSTEXSTATE:
						*ppOut = CRES<IITexState, MVertex>(d3device, desc, pData);
						break;
					case RESOURCE_TYPE_CAMPRO:
						*ppOut = CRES<ICamPro, MCamPro>(d3device, desc, pData);
						break;
					case RESOURCE_TYPE_LIGHT:
						*ppOut = CRES<ILight, MLight>(d3device, desc, pData);
						break;
					case RESOURCE_TYPE_TEXTURE2D:
						mut.lock();
						*ppOut = CRES<ITexture2D, MTexture2D>(this,d3device, comque,comalc, comlist, desc, pData);
						mut.unlock();
						break;
					default:
						break;
					}
				}
				catch (const std::exception& ex) {
					OutputDebugStringA(ex.what());
					return false;
				}
				return true;
			}
			virtual bool ACS_TCALL CreateView(ITexture2D* tex, VIEW_TYPE id, void** out)final {
				auto res = dynamic_cast<MTexture2D*>(tex);
				res->CreateView(d3device, id, out);
				return true;
			}
	
		public:

			virtual bool ACS_TCALL BeginUpdate(IUpdater**ppADU)final {
				WaitDrawDone();

				*ppADU = this;
				mut.lock();
				return true;
			}
			virtual bool ACS_TCALL EndUpdate()final {
				mut.unlock();
				return true;
			}

		private:
			MTarget* pTarget_using[16] = { nullptr };
			uint32_t now_target_cnt = 0;
			ID3D12DescriptorHeap* pDepth_using = nullptr;
			ID3D12DescriptorHeap* GetSamplerHeap(TEXSAMPLE_MODE mode) {
				switch (mode)
				{
				case TS_POINT:
					return PointSampler;
					break;
				case TS_LINEAR:
					return LinearSampler;
					break;
				case TS_ANISOTROPIC:
					return AnisotropicSampler;
					break;
				default:
					return  PointSampler;
					break;
				}
			}
			RENDER_MODE LastMode = RM_NONE;

			AREA m_area = { 0,0,1,1 };

			virtual bool ACS_TCALL BeginDraw(IDrawer**ppADC)final {
				WaitDrawDone();
			
				mut.lock();
				HRESULT hr;
				// コマンドアロケータをリセット
				hr = comalc->Reset();
				if FAILED(hr)return false;

				// コマンドリストをリセット
				hr = comlist->Reset(comalc, nullptr);
				if FAILED(hr)return false;

				*ppADC = this;
				return true;
			}
			virtual bool ACS_TCALL EndDraw()final {
				LastMode = RM_NONE;

				BarrierSet(comlist, D3D12_RESOURCE_STATE_PRESENT, target);
				for (uint32_t i = 0; i < now_target_cnt; i++)
				{
					pTarget_using[i] = nullptr;
				}
				now_target_cnt = 0;
				pDepth_using = nullptr;

				HRESULT hr;
				// コマンドリストをクローズする
				hr = comlist->Close();
				if (FAILED(hr))return false;

				// コマンドリストを実行する
				ID3D12CommandList* ppCommandLists[] = { comlist };
				comque->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

				mut.unlock();

				return true;
			}

			//シザー短形設定
			void setRect(const AREA& area) {
				D3D12_RECT rect[16];
				for (uint32_t i = 0; i < now_target_cnt; i++) {
					auto p = pTarget_using[i];
					rect[i] = p->GetInitDesc()->rect;

					rect[i].left = static_cast<LONG>(m_area.left * rect[i].right);
					rect[i].top = static_cast<LONG>(m_area.top * rect[i].bottom);
					rect[i].right = static_cast<LONG>(m_area.right * rect[i].right);
					rect[i].bottom = static_cast<LONG>(m_area.bottom * rect[i].bottom);	
				}
				comlist->RSSetScissorRects(now_target_cnt, rect);
			}

			virtual void ACS_TCALL SetRenderArea(const AREA& area)final {
				m_area = area;
				setRect(m_area);
			}
			virtual AREA ACS_TCALL GetRenderArea()final {
				return m_area;
			}

			virtual void ACS_TCALL SetTargets(uint32_t TargetCnt, ITarget*const* pTarget, IDepthStencil* pDepth = nullptr)final {
				if (pTarget == nullptr && TargetCnt != 0)return;
				ITarget* pADT = nullptr;
				D3D12_CPU_DESCRIPTOR_HANDLE hnd[16] = {0};
				D3D12_CPU_DESCRIPTOR_HANDLE d_hnd;
				D3D12_VIEWPORT vp[16];
			
				now_target_cnt = TargetCnt;
				if (pTarget) {
					for (uint32_t i = 0; i < TargetCnt; i++) {
						auto p = dynamic_cast<MTarget*>(pTarget[i]);
						BarrierSet(comlist, D3D12_RESOURCE_STATE_RENDER_TARGET, p->GetTexture());
						hnd[i] = p->GetRD();
						vp[i] = p->GetInitDesc()->vp;
						pTarget_using[i] = p;
					}
				}

				if (pDepth) {
					auto p = dynamic_cast<MDepthStencil*>(pDepth);
					BarrierSet(comlist, D3D12_RESOURCE_STATE_COMMON | D3D12_RESOURCE_STATE_DEPTH_WRITE, p->GetTexture());
					pDepth_using = p->GetHEAP();
					d_hnd = pDepth_using->GetCPUDescriptorHandleForHeapStart();
				}
				else {
					pDepth_using = nullptr;
				}

				if (pDepth)RenderPipelineMode = 1;
				else RenderPipelineMode = 0;

				if (pDepth)comlist->OMSetRenderTargets(TargetCnt, hnd, false, &d_hnd);
				else comlist->OMSetRenderTargets(TargetCnt, hnd, false, nullptr);
				comlist->RSSetViewports(TargetCnt, vp);
				setRect(m_area);

				LastMode = RM_NONE;
			}
			virtual void ACS_TCALL ClearTarget(uint32_t index, float color[4])final {
				if (pTarget_using[index]) {
					float* col = (color != nullptr) ? color : color_zero;
					D3D12_CPU_DESCRIPTOR_HANDLE hnd;
					hnd = pTarget_using[index]->GetRD();
					comlist->ClearRenderTargetView(hnd, col, 0, nullptr);
				}
			}
			virtual void ACS_TCALL ClearDepthStencill()final {
				if (pDepth_using) {
					D3D12_CPU_DESCRIPTOR_HANDLE hnd;
					hnd = pDepth_using->GetCPUDescriptorHandleForHeapStart();
					comlist->ClearDepthStencilView(hnd, D3D12_CLEAR_FLAG_DEPTH, 1.0, 0, 0, nullptr);
				}
			}

			virtual void ACS_TCALL CopyTexture(ITexture* Dst, ITexture* Src)throw() final {
				auto tdst = dynamic_cast<MTexture2D*>(Dst);
				auto tsrc = dynamic_cast<MTexture2D*>(Src);
				BarrierSet(comlist, D3D12_RESOURCE_STATE_COPY_DEST, tdst);
				BarrierSet(comlist, D3D12_RESOURCE_STATE_COPY_SOURCE, tsrc);

				D3D12_TEXTURE_COPY_LOCATION dst;
				D3D12_TEXTURE_COPY_LOCATION src;

				dst.pResource = tdst->GetRES();
				dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
				dst.SubresourceIndex = 0;

				src.pResource = tsrc->GetRES();
				src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
				src.PlacedFootprint = tsrc->GetCopyInfo();
				comlist->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
			}

			virtual void ACS_TCALL Draw(RENDER_MODE tp, const void* RenderData)final {
	#define SET_LINE(root, pipe) if (LastMode != tp) { RenderLineSet(comlist, root, pipe); }
				switch (tp)
				{
				case RM_DEFAULT:
					SET_LINE(rootsign, pipe[RenderPipelineMode]);
					RenderDef(comlist, static_cast<const RENDER_DATA_DEFAULT*>(RenderData));
					break;
				case RM_TEXTURE:
					SET_LINE(root_tex, pipe_tex[RenderPipelineMode]);
					RenderTexture(comlist, static_cast<const RENDER_DATA_TEXTURE*>(RenderData));
					break;
				case RM_WORLD:
					SET_LINE(root_world, pipe_world[RenderPipelineMode]);
					RenderWorld(comlist, static_cast<const RENDER_DATA_WORLD*>(RenderData));
					break;
				case RM_WORLD_TEX:
					SET_LINE(root_worldtex, pipe_worldtex[RenderPipelineMode]);
					RenderWorldTex(comlist, static_cast<const RENDER_DATA_WORLD_TEX*>(RenderData));
					break;
				case RM_WORLD_TEX_C:
					SET_LINE(root_worldtex, pipe_worldtex_c[RenderPipelineMode]);
					RenderWorldTex(comlist, static_cast<const RENDER_DATA_WORLD_TEX*>(RenderData));
					break;
				case RM_WCT:
					SET_LINE(root_worldtex, pipe_worldcoltex[RenderPipelineMode]);
					RenderWCT(comlist, static_cast<const RENDER_DATA_WCT*>(RenderData));
					break;
				case RM_WCT_C:
					SET_LINE(root_worldtex, pipe_worldcoltex_c[RenderPipelineMode]);
					RenderWCT(comlist, static_cast<const RENDER_DATA_WCT*>(RenderData));
					break;
				case RM_NORMAL_WORLD:
					SET_LINE(root_norworld, pipe_norworld[RenderPipelineMode]);
					RenderNormalWorld(comlist, static_cast<const RENDER_DATA_NORMAL_WORLD*>(RenderData));
					break;
				case RM_NORMAL_WORLD_TEX:
					SET_LINE(root_norworldtex, pipe_norworldtex[RenderPipelineMode]);
					RenderNormalWorldTex(comlist, static_cast<const RENDER_DATA_NORMAL_WORLD_TEX*>(RenderData));
					break;
				case RM_SPRITE:
					SET_LINE(root_sprite, pipe_sprite_c[RenderPipelineMode]);
					RenderSprite(comlist, static_cast<const RENDER_DATA_SPRITE*>(RenderData));
					break;
				case RM_SPRITE_COLOR:
					SET_LINE(root_sprite, pipe_sprite_col_c[RenderPipelineMode]);
					RenderSpriteCol(comlist, static_cast<const RENDER_DATA_SPRITE_COLOR*>(RenderData));
					break;
				case RM_DEPTH:
					SET_LINE(root_depth, pipe_depth);
					RenderDepth(comlist, static_cast<const RENDER_DATA_DEPTH*>(RenderData));
					break;
				case RM_SHADOWED:
					SET_LINE(root_shadow, pipe_shadow[RenderPipelineMode]);
					RenderShadowed(comlist, static_cast<const RENDER_DATA_SHADOW*>(RenderData));
					break;
				default:
					break;
				}
				LastMode = tp;
	#undef SET_LINE
			}

		private:
			void RenderLineSet(ID3D12GraphicsCommandList* comlist, ID3D12RootSignature* rs, ID3D12PipelineState* pl) {
				comlist->SetPipelineState(pl);
				comlist->SetGraphicsRootSignature(rs);
			}
			void RenderDef(ID3D12GraphicsCommandList* comlist, const RENDER_DATA_DEFAULT* data) {
				//インデックスバッファセット
				{
					auto p = dynamic_cast<MIndex*>(data->index);
					comlist->IASetIndexBuffer(&p->GetIBV());
				}
				//頂点バッファセット
				{
					auto p0 = dynamic_cast<MVertex*>(data->positions);
					auto p1 = dynamic_cast<MVertex*>(data->colors);
					auto p2 = dynamic_cast<MVertex*>(data->worlds);
					D3D12_VERTEX_BUFFER_VIEW VBV[3] = {
						p0->GetVBV(),p1->GetVBV(),p2->GetVBV()
					};
					comlist->IASetVertexBuffers(0, 3, VBV);
				}
				comlist->IASetPrimitiveTopology(CVPRIM(data->topology));
				comlist->DrawIndexedInstanced(data->IndexCount, data->InstanceCount, 0, 0, 0);
			}
			void RenderTexture(ID3D12GraphicsCommandList* comlist, const RENDER_DATA_TEXTURE* data) {
				//インデックスバッファセット
				{
					auto p = dynamic_cast<MIndex*>(data->index);
					comlist->IASetIndexBuffer(&p->GetIBV());
				}
				//頂点バッファセット
				{
					auto p0 = dynamic_cast<MVertex*>(data->positions);
					auto p1 = dynamic_cast<MVertex*>(data->uvs);
					auto p2 = dynamic_cast<MVertex*>(data->worlds);
					D3D12_VERTEX_BUFFER_VIEW VBV[3] = {
						p0->GetVBV(),p1->GetVBV(),p2->GetVBV()
					};
					comlist->IASetVertexBuffers(0, 3, VBV);
				}
				//定数バッファセット
				auto cTexture = dynamic_cast<MRenderResource*>(data->texture);
				auto cSampler = GetSamplerHeap(data->sample);
				ID3D12DescriptorHeap* heaps[] = { cTexture->GetHEAP(), cSampler };

				BarrierSet(comlist, D3D12_RESOURCE_STATE_GENERIC_READ, cTexture->GetTexture());
				comlist->SetDescriptorHeaps(2, heaps);
				comlist->SetGraphicsRootDescriptorTable(0, cTexture->GetHEAP()->GetGPUDescriptorHandleForHeapStart());
				comlist->SetGraphicsRootDescriptorTable(1, cSampler->GetGPUDescriptorHandleForHeapStart());

				comlist->IASetPrimitiveTopology(CVPRIM(data->topology));
				comlist->DrawIndexedInstanced(data->IndexCount, data->InstanceCount, 0, 0, 0);
				BarrierSet(comlist, D3D12_RESOURCE_STATE_COMMON, cTexture->GetTexture());//ここでバリアを張らないとGPUが間違って読み込むことがある,そして次の描画で変に表示される(要検証)
			}
			void RenderWorld(ID3D12GraphicsCommandList* comlist, const RENDER_DATA_WORLD* data) {
				//インデックスバッファセット
				auto i = dynamic_cast<MIndex*>(data->index);
				comlist->IASetIndexBuffer(&i->GetIBV());
				
				//頂点バッファセット
				auto v0 = dynamic_cast<MVertex*>(data->positions);
				auto v1 = dynamic_cast<MVertex*>(data->colors);
				auto v2 = dynamic_cast<MVertex*>(data->worlds);
				D3D12_VERTEX_BUFFER_VIEW VBV[3] = {
					v0->GetVBV(),v1->GetVBV(),v2->GetVBV()
				};
				comlist->IASetVertexBuffers(0, 3, VBV);
				
				//定数バッファセット
				auto c0 = dynamic_cast<MCamPro*>(data->campro);
				comlist->SetGraphicsRootConstantBufferView(0, c0->GetRES()->GetGPUVirtualAddress());
				
				comlist->IASetPrimitiveTopology(CVPRIM(data->topology));
				comlist->DrawIndexedInstanced(data->IndexCount, data->InstanceCount, 0, 0, 0);
			}
			void RenderWCT(ID3D12GraphicsCommandList* comlist, const RENDER_DATA_WCT* data) {
				//インデックスバッファセット
				auto i = dynamic_cast<MIndex*>(data->index);
				comlist->IASetIndexBuffer(&i->GetIBV());

				//頂点バッファセット
				auto v0 = dynamic_cast<MVertex*>(data->positions);
				auto v1 = dynamic_cast<MVertex*>(data->colors);
				auto v2 = dynamic_cast<MVertex*>(data->uvs);
				auto v3 = dynamic_cast<MVertex*>(data->worlds);
				auto v4 = dynamic_cast<MVertex*>(data->texstates);
				D3D12_VERTEX_BUFFER_VIEW VBV[5] = {
					v0->GetVBV(),v1->GetVBV(),v2->GetVBV(),v3->GetVBV(),v4->GetVBV()
				};
				comlist->IASetVertexBuffers(0, 5, VBV);

				//定数バッファセット
				auto c0 = dynamic_cast<MCamPro*>(data->campro);
				auto cTexture = dynamic_cast<MRenderResource*>(data->texture);
				auto cSampler = GetSamplerHeap(data->sample);
				ID3D12DescriptorHeap* heaps[] = { cTexture->GetHEAP(), cSampler };
				BarrierSet(comlist, D3D12_RESOURCE_STATE_GENERIC_READ, cTexture->GetTexture());
				comlist->SetDescriptorHeaps(2, heaps);
				comlist->SetGraphicsRootConstantBufferView(0, c0->GetRES()->GetGPUVirtualAddress());
				comlist->SetGraphicsRootDescriptorTable(1, cTexture->GetHEAP()->GetGPUDescriptorHandleForHeapStart());
				comlist->SetGraphicsRootDescriptorTable(2, cSampler->GetGPUDescriptorHandleForHeapStart());

				comlist->IASetPrimitiveTopology(CVPRIM(data->topology));
				comlist->DrawIndexedInstanced(data->IndexCount, data->InstanceCount, 0, 0, 0);
				BarrierSet(comlist, D3D12_RESOURCE_STATE_COMMON, cTexture->GetTexture());
			}
			void RenderWorldTex(ID3D12GraphicsCommandList* comlist, const RENDER_DATA_WORLD_TEX* data) {
				//インデックスバッファセット
				auto i = dynamic_cast<MIndex*>(data->index);
				comlist->IASetIndexBuffer(&i->GetIBV());

				//頂点バッファセット
				auto v0 = dynamic_cast<MVertex*>(data->positions);
				auto v1 = dynamic_cast<MVertex*>(data->uvs);
				auto v2 = dynamic_cast<MVertex*>(data->worlds);
				auto v3 = dynamic_cast<MVertex*>(data->texstates);
				D3D12_VERTEX_BUFFER_VIEW VBV[4] = {
					v0->GetVBV(),v1->GetVBV(),v2->GetVBV(),v3->GetVBV()
				};
				comlist->IASetVertexBuffers(0, 4, VBV);

				//定数バッファセット
				auto c0 = dynamic_cast<MCamPro*>(data->campro);
				auto cTexture = dynamic_cast<MRenderResource*>(data->texture);
				auto cSampler = GetSamplerHeap(data->sample);
				ID3D12DescriptorHeap* heaps[] = { cTexture->GetHEAP(), cSampler };
				BarrierSet(comlist, D3D12_RESOURCE_STATE_GENERIC_READ, cTexture->GetTexture());
				comlist->SetDescriptorHeaps(2, heaps);
				comlist->SetGraphicsRootConstantBufferView(0, c0->GetRES()->GetGPUVirtualAddress());
				comlist->SetGraphicsRootDescriptorTable(1, cTexture->GetHEAP()->GetGPUDescriptorHandleForHeapStart());
				comlist->SetGraphicsRootDescriptorTable(2, cSampler->GetGPUDescriptorHandleForHeapStart());

				comlist->IASetPrimitiveTopology(CVPRIM(data->topology));
				comlist->DrawIndexedInstanced(data->IndexCount, data->InstanceCount, 0, 0, 0);
				BarrierSet(comlist, D3D12_RESOURCE_STATE_COMMON, cTexture->GetTexture());
			}
			void RenderNormalWorld(ID3D12GraphicsCommandList* comlist, const RENDER_DATA_NORMAL_WORLD* data) {
				//インデックスバッファセット
				auto i = dynamic_cast<MIndex*>(data->index);
				comlist->IASetIndexBuffer(&i->GetIBV());

				//頂点バッファセット
				auto v0 = dynamic_cast<MVertex*>(data->positions);
				auto v1 = dynamic_cast<MVertex*>(data->colors);
				auto v2 = dynamic_cast<MVertex*>(data->normal);
				auto v3 = dynamic_cast<MVertex*>(data->worlds);
				D3D12_VERTEX_BUFFER_VIEW VBV[4] = {
					v0->GetVBV(),v1->GetVBV(),v2->GetVBV(),v3->GetVBV()
				};
				comlist->IASetVertexBuffers(0, 4, VBV);

				//定数バッファセット
				auto p0 = dynamic_cast<MCamPro*>(data->campro);
				auto p1 = dynamic_cast<MLight*>(data->light);
				static const UINT DcCount = 2;
				comlist->SetGraphicsRootConstantBufferView(0, p0->GetRES()->GetGPUVirtualAddress());
				comlist->SetGraphicsRootConstantBufferView(1, p1->GetRES()->GetGPUVirtualAddress());

				comlist->IASetPrimitiveTopology(CVPRIM(data->topology));
				comlist->DrawIndexedInstanced(data->IndexCount, data->InstanceCount, 0, 0, 0);
			}
			void RenderNormalWorldTex(ID3D12GraphicsCommandList* comlist, const RENDER_DATA_NORMAL_WORLD_TEX* data) {
				//インデックスバッファセット
				auto i = dynamic_cast<MIndex*>(data->index);
				comlist->IASetIndexBuffer(&i->GetIBV());

				//頂点バッファセット
				auto v0 = dynamic_cast<MVertex*>(data->positions);
				auto v1 = dynamic_cast<MVertex*>(data->uvs);
				auto v2 = dynamic_cast<MVertex*>(data->normal);
				auto v3 = dynamic_cast<MVertex*>(data->worlds);
				auto v4 = dynamic_cast<MVertex*>(data->texstates);
				D3D12_VERTEX_BUFFER_VIEW VBV[5] = {
					v0->GetVBV(),v1->GetVBV(),v2->GetVBV(),v3->GetVBV(),v4->GetVBV()
				};
				comlist->IASetVertexBuffers(0, 5, VBV);

				//定数バッファセット
				auto c0 = dynamic_cast<MCamPro*>(data->campro);
				auto c1 = dynamic_cast<MLight*>(data->light);
				auto cTexture = dynamic_cast<MRenderResource*>(data->texture);
				auto cSampler = GetSamplerHeap(data->sample);
				ID3D12DescriptorHeap* heaps[] = { cTexture->GetHEAP(), cSampler };
				BarrierSet(comlist, D3D12_RESOURCE_STATE_GENERIC_READ, cTexture->GetTexture());
				comlist->SetDescriptorHeaps(1, heaps);
				comlist->SetGraphicsRootConstantBufferView(0, c0->GetRES()->GetGPUVirtualAddress());
				comlist->SetGraphicsRootConstantBufferView(1, c1->GetRES()->GetGPUVirtualAddress());
				comlist->SetGraphicsRootDescriptorTable(2, cTexture->GetHEAP()->GetGPUDescriptorHandleForHeapStart());
				comlist->SetGraphicsRootDescriptorTable(3, cSampler->GetGPUDescriptorHandleForHeapStart());

				comlist->IASetPrimitiveTopology(CVPRIM(data->topology));
				comlist->DrawIndexedInstanced(data->IndexCount, data->InstanceCount, 0, 0, 0);
				BarrierSet(comlist, D3D12_RESOURCE_STATE_COMMON, cTexture->GetTexture());
			}
			void RenderSprite(ID3D12GraphicsCommandList* comlist, const RENDER_DATA_SPRITE* data) {
				//インデックスバッファセット
				comlist->IASetIndexBuffer(&spriteIndexView);

				//頂点バッファセット
				auto v0 = dynamic_cast<MVertex*>(data->worlds);
				auto v1 = dynamic_cast<MVertex*>(data->texstates);
				D3D12_VERTEX_BUFFER_VIEW VBV[] = {
					v0->GetVBV(),v1->GetVBV()
				};
				comlist->IASetVertexBuffers(0, 2, VBV);

				//定数バッファセット
				auto c0 = dynamic_cast<MCamPro*>(data->campro);
				auto cTexture = dynamic_cast<MRenderResource*>(data->texture);
				auto cSampler = GetSamplerHeap(data->sample);
				ID3D12DescriptorHeap* heaps[] = { cTexture->GetHEAP(), cSampler };
				BarrierSet(comlist, D3D12_RESOURCE_STATE_GENERIC_READ, cTexture->GetTexture());
				comlist->SetDescriptorHeaps(2, heaps);
				comlist->SetGraphicsRootConstantBufferView(0, c0->GetRES()->GetGPUVirtualAddress());
				comlist->SetGraphicsRootDescriptorTable(1, cTexture->GetHEAP()->GetGPUDescriptorHandleForHeapStart());
				comlist->SetGraphicsRootDescriptorTable(2, cSampler->GetGPUDescriptorHandleForHeapStart());

				comlist->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
				comlist->DrawIndexedInstanced(1, data->InstanceCount, 0, 0, data->InstanceOffs);
				BarrierSet(comlist, D3D12_RESOURCE_STATE_COMMON, cTexture->GetTexture());
			}
			void RenderSpriteCol(ID3D12GraphicsCommandList* comlist, const RENDER_DATA_SPRITE_COLOR* data) {
				//インデックスバッファセット
				comlist->IASetIndexBuffer(&spriteIndexView);
				
				//頂点バッファセット
				auto v0 = dynamic_cast<MVertex*>(data->worlds);
				auto v1 = dynamic_cast<MVertex*>(data->colors);
				auto v2 = dynamic_cast<MVertex*>(data->texstates);
				D3D12_VERTEX_BUFFER_VIEW VBV[] = {
					v0->GetVBV(),v1->GetVBV(),v2->GetVBV()
				};
				comlist->IASetVertexBuffers(0, 3, VBV);
				
				//定数バッファセット
				auto c0 = dynamic_cast<MCamPro*>(data->campro);
				auto cTexture = dynamic_cast<MRenderResource*>(data->texture);
				auto cSampler = GetSamplerHeap(data->sample);
				ID3D12DescriptorHeap* heaps[] = { cTexture->GetHEAP(), cSampler };
				BarrierSet(comlist, D3D12_RESOURCE_STATE_GENERIC_READ, cTexture->GetTexture());
				comlist->SetDescriptorHeaps(2, heaps);
				comlist->SetGraphicsRootConstantBufferView(0, c0->GetRES()->GetGPUVirtualAddress());
				comlist->SetGraphicsRootDescriptorTable(1, cTexture->GetHEAP()->GetGPUDescriptorHandleForHeapStart());
				comlist->SetGraphicsRootDescriptorTable(2, cSampler->GetGPUDescriptorHandleForHeapStart());

				comlist->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
				comlist->DrawIndexedInstanced(1, data->InstanceCount, 0, 0, data->InstanceOffs);
				BarrierSet(comlist, D3D12_RESOURCE_STATE_COMMON, cTexture->GetTexture());
			}
			void RenderDepth(ID3D12GraphicsCommandList* comlist, const RENDER_DATA_DEPTH* data) {
				//インデックスバッファセット
				auto i = dynamic_cast<MIndex*>(data->index);
				comlist->IASetIndexBuffer(&i->GetIBV());

				//頂点バッファセット
				auto v0 = dynamic_cast<MVertex*>(data->positions);
				auto v1 = dynamic_cast<MVertex*>(data->worlds);
				D3D12_VERTEX_BUFFER_VIEW VBV[] = {
					v0->GetVBV(),v1->GetVBV()
				};
				comlist->IASetVertexBuffers(0, 2, VBV);
				
				//定数バッファセット
				auto c0 = dynamic_cast<MCamPro*>(data->campro);
				comlist->SetGraphicsRootConstantBufferView(0, c0->GetRES()->GetGPUVirtualAddress());

				comlist->IASetPrimitiveTopology(CVPRIM(data->topology));
				comlist->DrawIndexedInstanced(data->IndexCount, data->InstanceCount, data->IndexOffs, data->VertexOffs, data->InstanceOffs);
			}
			void RenderShadowed(ID3D12GraphicsCommandList* comlist, const RENDER_DATA_SHADOW* data) {
				//インデックスバッファセット
				auto i = dynamic_cast<MIndex*>(data->index);
				comlist->IASetIndexBuffer(&i->GetIBV());

				//頂点バッファセット
				auto v0 = dynamic_cast<MVertex*>(data->positions);
				auto v1 = dynamic_cast<MVertex*>(data->colors);
				auto v2 = dynamic_cast<MVertex*>(data->normal);
				auto v3 = dynamic_cast<MVertex*>(data->worlds);
				D3D12_VERTEX_BUFFER_VIEW VBV[] = {
					v0->GetVBV(),v1->GetVBV(),v2->GetVBV(),v3->GetVBV()
				};
				comlist->IASetVertexBuffers(0, 4, VBV);

				//定数バッファセット
				auto cCam = dynamic_cast<MCamPro*>(data->campro);
				auto cLightCam = dynamic_cast<MCamPro*>(data->Llightcp);
				auto cLight = dynamic_cast<MLight*>(data->light);
				auto cShadow= dynamic_cast<MRenderResource*>(data->shadow);
				ID3D12DescriptorHeap* cShadowSampler = AnisotropicSampler;
				ID3D12DescriptorHeap* heaps[] = { cShadow->GetHEAP(), cShadowSampler };
				BarrierSet(comlist, D3D12_RESOURCE_STATE_GENERIC_READ, cShadow->GetTexture());
				comlist->SetDescriptorHeaps(2, heaps);
				comlist->SetGraphicsRootConstantBufferView(0, cCam->GetRES()->GetGPUVirtualAddress());
				comlist->SetGraphicsRootConstantBufferView(1, cLightCam->GetRES()->GetGPUVirtualAddress());
				comlist->SetGraphicsRootConstantBufferView(2, cLight->GetRES()->GetGPUVirtualAddress());
				comlist->SetGraphicsRootDescriptorTable(3, cShadow->GetHEAP()->GetGPUDescriptorHandleForHeapStart());
				comlist->SetGraphicsRootDescriptorTable(4, cShadowSampler->GetGPUDescriptorHandleForHeapStart());
				
				comlist->IASetPrimitiveTopology(CVPRIM(data->topology));
				comlist->DrawIndexedInstanced(data->IndexCount, data->InstanceCount, data->IndexOffs, data->VertexOffs, data->InstanceOffs);
				BarrierSet(comlist, D3D12_RESOURCE_STATE_COMMON, cShadow->GetTexture());
			}

			virtual bool ACS_TCALL Map(IResource* res, void** out) final {
				auto p = dynamic_cast<MResource*>(res);
				return p->Map(out);
			}
			virtual void ACS_TCALL Unmap(IResource* res)final {
				auto p = dynamic_cast<MResource*>(res);
				p->Unmap();
			}

			virtual bool ACS_TCALL UpdateTexture2D(ITexture2D* tex, const TEXTURE2D_SOURCE_INFO* uinfo, const COLOR* src)final {
				auto p = dynamic_cast<MTexture2D*>(tex);
				auto buf = p->GetRES();
				acex::draw::COLOR* dest;
				HRESULT hr = buf->Map(0, 0, (void**)&dest);
				if (FAILED(hr))return false;
				auto info = p->GetCopyInfo();

				size_t row_copy_size = (uinfo->sourceWidth < info.Footprint.Width) ? uinfo->sourceWidth : info.Footprint.Width;
				size_t row_copy_count = (uinfo->sourceRowCount < info.Footprint.Height) ? uinfo->sourceRowCount : info.Footprint.Height;
				for (size_t i = 0; i < row_copy_count; i++)
				{
					memcpy(dest, src, static_cast<size_t>(row_copy_size));
					dest += info.Footprint.RowPitch;
					src += uinfo->sourceRowPitch;
				}
				buf->Unmap(0, 0);
				return true;
			}
			virtual bool ACS_TCALL ReadTexture2D(ITexture2D* tex, const TEXTURE2D_DEST_INFO* uinfo, COLOR* dest)final {
				auto p = dynamic_cast<MTexture2D*>(tex);
				auto buf = p->GetRES();
				acex::draw::COLOR* src;
				HRESULT hr = buf->Map(0, 0, (void**)&src);
				if (FAILED(hr))return false;
				auto info = p->GetCopyInfo();

				size_t row_copy_size = (uinfo->destWidth < info.Footprint.Width) ? uinfo->destWidth : info.Footprint.Width;
				size_t row_copy_count = (uinfo->destRowCount < info.Footprint.Height) ? uinfo->destRowCount : info.Footprint.Height;
				for (size_t i = 0; i < row_copy_count; i++)
				{
					memcpy(dest, src, static_cast<size_t>(row_copy_size));
					src += info.Footprint.RowPitch;
					dest += uinfo->destRowPitch;
				}
				buf->Unmap(0, 0);
				return true;
			}
		
			virtual bool ACS_TCALL Present(int interval)final {
				WaitDrawDone();
				return (target->Present(interval));
			}

			private:
				std::atomic_uint64_t fenceValue = 1;
			virtual void ACS_TCALL WaitDrawDone()final {
				mut.lock();
				UINT64 fvalue;
				ID3D12Fence* fence = this->Fence;
				comque->Signal(fence, fvalue = (fenceValue));

				if (fence->GetCompletedValue() < fvalue)
				{
					fence->SetEventOnCompletion(fvalue, DrawDoneEvent);
					WaitForSingleObject(DrawDoneEvent, INFINITE);
				}

				fenceValue++;
				mut.unlock();
			}
		};
	}
}
