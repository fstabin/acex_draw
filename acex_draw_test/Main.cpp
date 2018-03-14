#include "stdafx.h"
#include "Main.h"
//#include "acex::draw.h"
//#include "acex::draw::ex.h"

#include "..\acex_draw\include\acex_draw.h"

#include <crtdbg.h>
#include <DirectXMath.h>
#include <thread>
#include <atomic>

#include "acex_draw_ex\include\acex_draw_ex.h"

#pragma comment(lib,"acex_draw_dx12.lib")
#pragma comment(lib,"acex_draw_ex.lib")
#pragma comment(lib,"acs_ioImage.lib")

#define EndIfFalse(x) if(!x)return -1;

static LPCWSTR	kWindowTitle = L"DX12 Sample";
static const int	kWindowWidth = 960;
static const int	kWindowHeight = 540;

static int	ClientWidth = 960;
static int	ClientHeight = 540;
bool resize = false;

uint16_t IndexData[] = { 0,1,2,3 };
acex::draw::VERTEX_POSITION VertexPos[] = { { -0.5,0.5,0 },{ 0.5,0.5,0 },{ -0.5,-0.5,0 },{ 0.5,-0.5,0 } };
acex::draw::VERTEX_COLOR VertexCol[] = { { 1,1,0,1 },{ 1,1,0,1 },{ 1,1,0,1 },{ 1,1,0,1 } };
acex::draw::VERTEX_UV VertexUV[] = { { 0,0 },{ 1,0 },{ 0,1 },{ 1,1 } };
acex::draw::VERTEX_NORMAL VertexNor[] = { { 0,0, -1 },{ 0,0, -1 },{ 0,0, -1 },{ 0,0, -1 } };
acex::draw::COLOR TCol[] = { { 255,255,255,255 },{ 255,255,255,255 },{ 255,255,255,255 },{ 255,255,255,255 } ,
{ 255,255,0,255 },{ 100,255,255,255 },{ 0,255,0,255 },{ 255,0,255,255 } ,
{ 255,255,0,255 },{ 100,255,255,255 },{ 0,255,0,255 },{ 255,0,255,255 } ,
{ 255,255,0,255 },{ 100,255,255,255 },{ 0,255,0,255 },{ 255,0,255,255 } };
acex::draw::INS_WORLD wor = { 10,0,0,0, 0,10,0,0, 0,0,1,5, 0,0,0,1 };
acex::draw::C_CAMPRO DCamPro;
acex::draw::C_CAMPRO DCamPro2d;
acex::draw::INS_TEXSTATE DTexs = { 0,0,1,1 };

uint16_t IndexBox[] = { 0,1,2, 1,3,2, 4,5,6, 5,7,6, 8,9,10, 9,11,10, 12,13,14, 13,15,14, 16,17,18, 17,19,18, 20,21,22, 21,23,22};
acex::draw::VERTEX_POSITION VBoxPos[] = { 
	{ -0.5,0.5,-0.5 },{ 0.5,0.5,-0.5 },{ -0.5,-0.5,-0.5 },{ 0.5,-0.5,-0.5 },
	{ -0.5,0.5,0.5 },{ 0.5,0.5,0.5 },{ -0.5,0.5,-0.5 },{ 0.5,0.5,-0.5 },
	{ 0.5,0.5,-0.5 },{ 0.5,0.5,0.5 },{ 0.5,-0.5,-0.5 },{ 0.5,-0.5,0.5 },
	{ 0.5,-0.5,0.5 },{ -0.5,-0.5,0.5 },{ 0.5,-0.5,-0.5 },{ -0.5,-0.5,-0.5 },
	{ -0.5,0.5,0.5 },{ -0.5,0.5,-0.5 },{ -0.5,-0.5,0.5 },{ -0.5,-0.5,-0.5 },
	{ 0.5,0.5,0.5 },{ -0.5,0.5,0.5 },{ 0.5,-0.5,0.5 },{ -0.5,-0.5,0.5 }, };
acex::draw::VERTEX_COLOR VBoxCol[] = { 
	{ 0.5f,0.5f,1,1 },{ 0.5,0.5,1,1 },{ 0.5,0.5,1,1 },{ 0.5,0.5,1,1 },
	{ 0.5,0.5,1,1 },{ 0.5,0.5,1,1 },{ 0.5,0.5,1,1 },{ 0.5,0.5,1,1 },
	{ 0.5f,0.5f,1,1 },{ 0.5,0.5,1,1 },{ 0.5,0.5,1,1 },{ 0.5,0.5,1,1 },
	{ 0.5,0.5,1,1 },{ 0.5,0.5,1,1 },{ 0.5,0.5,1,1 },{ 0.5,0.5,1,1 },
	{ 0.5f,0.5f,1,1 },{ 0.5,0.5,1,1 },{ 0.5,0.5,1,1 },{ 0.5,0.5,1,1 },
	{ 0.5,0.5,1,1 },{ 0.5,0.5,1,1 },{ 0.5,0.5,1,1 },{ 0.5,0.5,1,1 },
};
acex::draw::VERTEX_UV VBoxUV[] = { 
	{ 0,0 },{ 1,0 },{ 0,1 },{ 1,1 },
	{ 0,0 },{ 1,0 },{ 0,1 },{ 1,1 },
	{ 0,0 },{ 1,0 },{ 0,1 },{ 1,1 },
	{ 0,0 },{ 1,0 },{ 0,1 },{ 1,1 },
	{ 0,0 },{ 1,0 },{ 0,1 },{ 1,1 },
	{ 0,0 },{ 1,0 },{ 0,1 },{ 1,1 },
};
acex::draw::VERTEX_NORMAL VBoxNor[] = { 
	{0,0,-1},{ 0,0,-1 },{ 0,0,-1 },{ 0,0,-1 },
	{0,1,0},{ 0,1,0 },{ 0,1,0 },{ 0,1,0 }, 
	{ 1,0,0 },{ 1,0,0 },{ 1,0,0 },{ 1,0,0 },
	{ 0,-1,0 },{ 0,-1,0 },{ 0,-1,0 },{ 0,-1,0 },
	{-1,0,0},{ -1,0,0 },{ -1,0,0 },{ -1,0,0 },
	{ 0,0,1 },{ 0,0,1 },{ 0,0,1 },{ 0,0,1 }
};
acex::draw::INS_WORLD IBoxWor = { 1,0,0,0, 0,1,0,0, 0,0,1,2, 0,0,0,1 };

acex::draw::C_LIGHT lt = { { -10,10,-10 },{ 1,1,1 },{ 0.2,0.2,0.2 },{ 0,0,0 } };
acex::draw::C_CAMPRO LightCPro;


int App::Main::Func() {
	AppBase::ScreenSetSize({ kWindowWidth ,kWindowHeight });

	acs::matrix::m4x4<> mat;
	acs::matrix::set_camera_direction(mat, { 0,2,-10 }, { 0,-0.1f,1 }, { 0, 1, 0 });
	memcpy(&DCamPro.cam,&mat, 64);
	acs::matrix::set_perspectiveLH(mat, { 1, 1}, {1,20});
	memcpy(&DCamPro.pro, &mat, 64);

	acs::matrix::set_camera_direction(mat, { 0,0,0 }, { 0,0,1 }, { 0, 1, 0 });
	memcpy(&DCamPro2d.cam, &mat, 64);
	acs::matrix::set_orthographicLH(mat, { 2, 2 }, { 0,1 });
	memcpy(&DCamPro2d.pro, &mat, 64);

	acex::draw::INIT_DESC ini;
	ini.hWnd = AppBase::hMainWindow;
	ini.Size = { static_cast<acs::uint>(ClientWidth) ,static_cast<acs::uint>(ClientHeight) };
	acs::SIACS<acex::draw::IDraw> draw;
	if (!acex::draw::CreateDraw(&ini, &draw))return -1;
	acex::draw::RESOURCE_DESC rdesc;
	acs::SIACS<acex::draw::ITarget> screenTarget;
	acs::SIACS<acex::draw::IDepthStencil> screenDepth;
	acs::SIACS<acex::draw::IRenderResource> screenDepthResource;
	if (!draw->GetScreenTarget(&screenTarget))return -1;
	EndIfFalse(acex::draw::ex::CreateIDepthBuffer(&screenDepth, &screenDepthResource,draw, ClientWidth, ClientHeight));

	acs::SIACS<acs::image::IioImage>ioimg;
	acs::image::CreateioImage(&ioimg);

	acex::draw::ex::FigureHost fhost(draw, ioimg, L"figure.png");
	acex::draw::VERTEX_COLOR fvcol[] = { { 0,0,0,1 },{ 0,0,0,1 },{ 0,0,0,1 },{ 0,0,0,1 } };
	acex::draw::ex::FigureColor fcol(draw, fvcol);
	acex::draw::ex::ColorFigure figure(draw, &fhost, &fcol, 3);
	figure.Size(16);
	figure.SetBasePos({ - static_cast<float>(ClientWidth) / 2 ,static_cast<float>(ClientHeight) / 2,0 });

	acs::SIACS<acex::draw::IIndex> index;
	acs::SIACS<acex::draw::IVPosition> pos;
	acs::SIACS<acex::draw::IVColor> color;
	acs::SIACS<acex::draw::IVUv> uv;
	acs::SIACS<acex::draw::IIWorld> world;
	acs::SIACS<acex::draw::IITexState> texstate;

	acs::SIACS<acex::draw::ICamPro> campro;
	acs::SIACS<acex::draw::ICamPro> campro2d;
	EndIfFalse(acex::draw::ex::CreateICamPro(&campro2d, draw, acex::draw::RESOURCE_ACCESS_WRITE, &DCamPro2d));

	acs::SIACS<acex::draw::ITexture2D> tIO;
	acs::SIACS<acex::draw::ITexture2D> t2Res;
	acs::SIACS<acex::draw::ITexture2D> t2ResTarget;
	acs::SIACS<acex::draw::IRenderResource> rRes;
	acs::SIACS<acex::draw::IRenderResource> rtRes;
	acs::SIACS<acex::draw::ITarget> rtTarget;

	acs::SIACS<acex::draw::IVNormal> iNormal;	
	EndIfFalse(acex::draw::ex::CreateIVNormal(&iNormal, draw, 4, acex::draw::RESOURCE_ACCESS_WRITE, VertexNor));
	
	acs::SIACS<acex::draw::ILight> iLight;
	acs::SIACS<acex::draw::ICamPro> LightCP;
	EndIfFalse(acex::draw::ex::CreateILight(&iLight, draw, acex::draw::RESOURCE_ACCESS_WRITE, &lt));
	EndIfFalse(acex::draw::ex::CreateICamPro(&LightCP, draw, acex::draw::RESOURCE_ACCESS_WRITE, &LightCPro));

	acs::SIACS<acex::draw::IRenderResource>ShadowR;
	acs::SIACS<acex::draw::IDepthStencil> ShadowD;
	EndIfFalse(acex::draw::ex::CreateIDepthBuffer(&ShadowD,&ShadowR,draw, ClientWidth, ClientHeight));

	acs::SIACS<acex::draw::IIndex> boxi;
	acs::SIACS<acex::draw::IVPosition> boxvp;
	acs::SIACS<acex::draw::IVColor> boxvc;
	acs::SIACS<acex::draw::IVUv> boxuv;
	acs::SIACS<acex::draw::IVNormal> boxnor;
	acs::SIACS<acex::draw::IIWorld> boxwor;
	EndIfFalse(acex::draw::ex::CreateIIndex(&boxi, draw, acex::draw::IFMT_U16, ARRAYSIZE(IndexBox), 0, IndexBox));
	EndIfFalse(acex::draw::ex::CreateIVPosition(&boxvp, draw,ARRAYSIZE(VBoxPos), 0, VBoxPos));
	EndIfFalse(	acex::draw::ex::CreateIVColor(&boxvc, draw, ARRAYSIZE(VBoxCol), 0, VBoxCol));
	EndIfFalse(acex::draw::ex::CreateIVUv(&boxuv, draw, ARRAYSIZE(VBoxUV), 0, VBoxUV));
	EndIfFalse(acex::draw::ex::CreateIVNormal(&boxnor, draw, ARRAYSIZE(VBoxNor), 0, VBoxNor));
	EndIfFalse(acex::draw::ex::CreateIIWorld(&boxwor, draw, 1, acex::draw::RESOURCE_ACCESS_WRITE, &IBoxWor));

	acex::draw::ex::sprite::SimpleSprite Sptex(draw, 1);

	//頂点
	EndIfFalse(acex::draw::ex::CreateIIndex(&index, draw, acex::draw::IFMT_U16, ARRAYSIZE(IndexData), 0, IndexData));

	EndIfFalse(acex::draw::ex::CreateIVPosition(&pos, draw, ARRAYSIZE(VertexPos), 0, VertexPos));
	EndIfFalse(acex::draw::ex::CreateIVColor(&color, draw, ARRAYSIZE(VertexCol), 0, VertexCol));
	EndIfFalse(acex::draw::ex::CreateIVUv(&uv, draw, ARRAYSIZE(VertexUV), 0, VertexUV));

	EndIfFalse(acex::draw::ex::CreateIIWorld(&world, draw, 1, acex::draw::RESOURCE_ACCESS_WRITE, &wor));
	EndIfFalse(acex::draw::ex::CreateIITexState(&texstate, draw, 1, acex::draw::RESOURCE_ACCESS_WRITE, &DTexs));

	EndIfFalse(acex::draw::ex::CreateICamPro(&campro, draw, acex::draw::RESOURCE_ACCESS_WRITE, &DCamPro));
	//テクスチャ
	{
		acex::draw::TEXTURE2D_DESC mdesc;
		rdesc.desc = &mdesc;
		rdesc.AccessFlag = acex::draw::RESOURCE_ACCESS_WRITE | acex::draw::RESOURCE_ACCESS_READ;
		rdesc.type = acex::draw::RESOURCE_TYPE_TEXTURE2D;

		mdesc.Width = ClientWidth;
		mdesc.Height = ClientHeight;
		mdesc.useflag = acex::draw::TEXUSE_IO;
		if (!draw->CreateResource(rdesc, nullptr, (void**)&tIO))return -1;
		rdesc.AccessFlag = acex::draw::RESOURCE_ACCESS_NONE;
		mdesc.Width = 4;
		mdesc.Height = 4;
		mdesc.useflag = acex::draw::TEXUSE_RENDER_RESOURCE;
		if (!draw->CreateResource(rdesc, TCol, (void**)&t2Res))return -1;
		if (!draw->CreateView(t2Res, acex::draw::VIEW_RENDER_RESOURCE, (void**)&rRes))return -1;
		mdesc.Width = 30;
		mdesc.Height = 30;
		mdesc.useflag = (acex::draw::TEXUSE_RENDER_RESOURCE | acex::draw::TEXUSE_TARGET);;
		if (!draw->CreateResource(rdesc, nullptr, (void**)&t2ResTarget))return -1;
		if (!draw->CreateView(t2ResTarget, acex::draw::VIEW_RENDER_RESOURCE, (void**)&rtRes))return -1;
		if (!draw->CreateView(t2ResTarget, acex::draw::VIEW_TARGET, (void**)&rtTarget))return -1;
	}

	acex::draw::ex::text::TextImageFactory tif(draw);

	acex::draw::ex::text::Brush brush(255,255,255);
	acex::draw::ex::text::Font font(&tif,16,L"MS　ゴシック");
	acex::draw::ex::text::StaticColorText text(draw,&tif,&font,&brush,L"Hello",5);
	text.setCamera(campro2d);
	text.setPos({ 0,0,0.1f });
	text.setSize(0.5f);
	text.setColor({1,0,0,1});
	//text.Image(ShadowR);

	Sptex.setRenderResource(ShadowR);
	Sptex.setCamera(campro2d);
	{
		auto& ref = Sptex[0];
		ref.flag = true;
		ref.pos.x = ref.pos.y = -0.75;
		ref.pos.z = 0.1f;
		ref.rotate = { 0,0,0 };
		ref.size = { 0.5,0.5 };
		ref.texstate = { 0,0,1,1 };
		//ref.color = {1,1,1,1};
	}

	AppBase::ScreenSetup();
		
	while (AppBase::AppWait(0)) {
		{
			draw->WaitDrawDone();
			draw->Present(1);

			static float cx = -10;
			static float cy = 10;
			if (AppBase::CheckKeyDown(VK_UP)) {
				cy += 0.1f;
			}
			if (AppBase::CheckKeyDown(VK_DOWN)) {
				cy -= 0.1f;
			}
			if (AppBase::CheckKeyDown(VK_LEFT)) {
				cx += 0.1f;
			}
			if (AppBase::CheckKeyDown(VK_RIGHT)) {
				cx -= 0.1f;
			}

			static float h = 0;
			//h += 0.05;

			float fh = h;
			{
				acex::draw::ex::Updater updater(draw);
				acex::draw::INS_WORLD* wor;
				float fsih = sin(h);
				float fcoh = cos(h);
				updater->Map(boxwor, (void**)&wor);
				wor->f4x4 = {
					fcoh,0,-fsih,0,
					0,1,0,0,
					fsih,0,fcoh,0,
					0,0,0,1,
				};
				updater->Unmap(boxwor);
				Sptex.Update(updater);

				acex::draw::C_CAMPRO* cp;
				acex::draw::C_LIGHT* cl;
				updater->Map(LightCP, (void**)&cp);
				updater->Map(iLight, (void**)&cl);
				acs::matrix::set_camera_focus(mat, { cx,cy,-10 }, { 0,0,0 }, { 0, 1, 0 });
				memcpy(&cp->cam, &mat, 64);
				acs::matrix::set_perspectiveLH(mat, { 1, 1 }, { 1,20 });
				memcpy(&cp->pro, &mat, 64);
				cl->pos[0] = cx;
				cl->pos[1] = cy;
				updater->Unmap(iLight);
				updater->Unmap(LightCP);

				text.Update(updater);

			}
			
			float clearCol[] = { 0.5,0.5,0.5,0 };
			{
				acex::draw::ex::Drawer context(draw);
				acex::draw::ITarget* d[] = { screenTarget };
				context->SetTargets(1, d, ShadowD);
				context->ClearDepthStencill();
				acex::draw::ex::RenderDepth(context, 1, 4, acex::draw::PT_TRIANGLESTRIP,
					index, pos, world, LightCP);
				acex::draw::ex::RenderDepth(context, 1, 36, acex::draw::PT_TRIANGLELIST,
					boxi, boxvp, boxwor, LightCP);
				

				context->SetTargets(1, d, screenDepth);
				context->ClearTarget(0, clearCol);
				context->ClearDepthStencill();
				acex::draw::ex::RenderShadowed(context, 1, 4, acex::draw::PT_TRIANGLESTRIP,
					index, pos, color, iNormal, world, campro, iLight, LightCP, ShadowR);
				acex::draw::ex::RenderShadowed(context, 1, 36, acex::draw::PT_TRIANGLELIST,
					 boxi, boxvp, boxvc, boxnor, boxwor, campro, iLight, LightCP, ShadowR);

				context->SetTargets(1, d, nullptr);

				Sptex.Draw(context);

				//text.Draw(context);
			}
			draw->WaitDrawDone();
		};
	}

	return 0;
}