#pragma once

#include <memory>
#include <list>

#include "acs_ioImage\include\acs_ioImage.h"
#include "acs\include\def.h"
#include "acs\include\vector.h"
#include "acs\include\attribute.h"

#include "base.h"

namespace acex {
	namespace draw {
		namespace ex {
			//Figure... 数字クラスの元、サポート系
			//...Figure  様々な数字クラス（本体）

			// 共有リソース

			class FigureHost {
				acs::SIACS<acex::draw::IIndex> Index;
				acs::SIACS<acex::draw::IVPosition> Mesh;
				acs::SIACS<acex::draw::IVUv> UV;
				acs::SIACS<acex::draw::IRenderResource> TexFigure;
			public:
				ACS_NO_COPY(FigureHost);
				FigureHost() {

				}
				FigureHost(acex::draw::IDraw* draw, acs::image::IioImage* ioimage, const wchar_t* pass) {
					uint16_t ind[4] = { 0,1,2,3 };
					if (!acex::draw::ex::CreateIIndex(&Index, draw, acex::draw::IFMT_U16, 4, 0, ind)) throw(ACEXDrawExCreateResourceFailedException());
					acex::draw::VERTEX_POSITION ver[4] = { {0 , 0, 0},{ 1.f , 0,0 },{  0, -1.f ,0 },{ 1, -1.f,0 } };
					if (!acex::draw::ex::CreateIVPosition(&Mesh, draw, 4, 0, ver)) throw(ACEXDrawExCreateResourceFailedException());
					acex::draw::VERTEX_UV uvd[4] = { {0,0},{1,0},{0,1},{1,1} };
					if (!acex::draw::ex::CreateIVUv(&UV, draw, 4, 0, uvd)) throw(ACEXDrawExCreateResourceFailedException());
					if (!acex::draw::ex::CreateIRenderResourceP(draw, ioimage, pass, &TexFigure)) throw(ACEXDrawExCreateResourceFailedException());
				}
				inline acex::draw::IIndex* GetIndex() {
					return Index;
				}
				inline acex::draw::IVPosition* GetMesh() {
					return Mesh;
				}
				inline acex::draw::IVUv* GetUV() {
					return UV;
				}
				inline acex::draw::IRenderResource* GetTex() {
					return TexFigure;
				}
			};

			// リソース

			class FigureColor {
				acs::SIACS<acex::draw::IVColor> col;
			public:
				ACS_NO_COPY(FigureColor);
				FigureColor() {

				}
				FigureColor(acex::draw::IDraw* draw, const acex::draw::VERTEX_COLOR color[4]) {
					if (!acex::draw::ex::CreateIVColor(&col, draw, 4, 0, color)) throw(ACEXDrawExCreateResourceFailedException());
				}
				void init(acex::draw::IDraw* draw, const acex::draw::VERTEX_COLOR color[4]) {
					this->~FigureColor();
					new (this) FigureColor(draw, color);
				}
				inline acex::draw::IVColor* GetColor() {
					return col;
				}
			};

			class Figure:public acs::CopyDisable{
			protected:
				FigureHost* hostRef = nullptr;
				acs::SIACS<acex::draw::IIWorld> dWorld;
				acs::SIACS<acex::draw::IITexState> dTexstate;
				uint32_t disit;
				uint32_t d_keta = 0;
				std::unique_ptr<acs::byte[]> vx;
				float size = 16;
				bool zeroMode = false;
				acs::vector::f3d pos;

				bool mustupdate = true;
			public:
				Figure() {};
				Figure(acex::draw::IDraw* draw, FigureHost* host, uint32_t _keta) :hostRef(host), disit(_keta) {
					if (_keta < 2)disit = _keta = 2;
					if (!acex::draw::ex::CreateIIWorld(&dWorld, draw, _keta, acex::draw::RESOURCE_ACCESS_WRITE)) throw(ACEXDrawExCreateResourceFailedException());
					if (!acex::draw::ex::CreateIITexState(&dTexstate, draw, _keta, acex::draw::RESOURCE_ACCESS_WRITE)) throw(ACEXDrawExCreateResourceFailedException());
				}

				inline void SetVal(acs::longlong r) {
					uint32_t i = 0;
					auto p = vx.get();

					if (zeroMode) {
						if (r < 0) {
							*p = 12;
							i++;
							p++;
						}
						else if (r == 0) {
							*p = 0;
							i++;
							p++;
						}
						while (i < disit)
						{
							*p = r % 10;
							r /= 10;
							i++;
							p++;
						}
						d_keta = disit;
					}
					else {
						if (r < 0) {
							*p = 12;
							i++;
							p++;
						}
						else if (r == 0) {
							*p = 0;
							i++;
							p++;
						}
						while (i < disit && r != 0)
						{
							*p = r % 10;
							r /= 10;
							i++;
							p++;
						}
						d_keta = i;
					}
					if (!mustupdate)mustupdate = true;
				}

				inline void Size(float v) {
					size = v;
					if (!mustupdate)mustupdate = true;
				}
				inline void SetZeroMode(bool b) {
					zeroMode = b;
					if (!mustupdate)mustupdate = true;
				}
				inline void SetBasePos(acs::vector::f3d _pos) {
					pos = _pos;
					if (!mustupdate)mustupdate = true;
				}

				virtual void Draw(acex::draw::IDrawer* context, acex::draw::ICamPro* Cam2D) {
					if (d_keta == 0)return;
					acex::draw::ex::RenderWorldTex_C(context, d_keta, 4, 4, acex::draw::PT_TRIANGLESTRIP, acex::draw::TS_POINT,
						hostRef->GetIndex(), hostRef->GetMesh(), hostRef->GetUV(), dWorld, dTexstate, Cam2D, hostRef->GetTex());
				}
				virtual void Update(acex::draw::IUpdater* updater) {
					if (mustupdate) {
						mustupdate = false;
						acex::draw::ex::Mapped<acex::draw::INS_WORLD> wor(updater, dWorld);
						acex::draw::ex::Mapped<acex::draw::INS_TEXSTATE> texs(updater, dTexstate);
						acs::vector::f3d f3 = pos;
						static bool inited = false;
						static acex::draw::INS_TEXSTATE tes[16];
						if (!inited) {
							auto tp = tes;
							for (size_t i = 0; i < 4; i++)
							{
								for (size_t j = 0; j < 4;j++) {
									tp->Xoffs = 8.f * j / 32.f;
									tp->Yoffs = 8.f * i / 32.f;
									tp->Xsize = tp->Ysize = 8.f / 32.f;
									tp++;
								}
							}
							inited = true;
						}

						for (size_t i = 0; i < d_keta; i++)
						{
							wor[i] = {
								size,0,0,f3.x,
								0,size,0,f3.y,
								0,0,1,f3.z,
								0,0,0,1 };
							texs[i] = tes[vx[d_keta - i - 1]];
							f3.x += size;
						}
					}
				}
			};

			class ColorFigure :public Figure {
			protected:
				FigureColor * refFcol = nullptr;
			public:
				ColorFigure() {};
				ColorFigure(acex::draw::IDraw* draw, FigureHost* host, FigureColor* color, uint32_t _keta)
					: Figure(draw, host, _keta), refFcol(color) {}
					
				virtual void Draw(acex::draw::IDrawer* context, acex::draw::ICamPro* Cam2D) {
					if (d_keta == 0)return;
					acex::draw::ex::RenderWCT_C(context, d_keta, 4, 4, acex::draw::PT_TRIANGLESTRIP, acex::draw::TS_POINT,
						hostRef->GetIndex(), hostRef->GetMesh(), refFcol->GetColor(), hostRef->GetUV(), dWorld, dTexstate, Cam2D, hostRef->GetTex());
				}
			};

			class FigureWriter {
				std::list<Figure*> figures;

			public:
				ACS_NO_COPY(FigureWriter);
				FigureWriter() {};
				void SetFigure(Figure* one) {
					figures.push_back(one);
				}
				void RemoveFigure(Figure* one) {
					(figures.remove(one));
				}
				void Draw(acex::draw::IDrawer* context, acex::draw::ICamPro* Cam2D) {
					auto iter = figures.begin();
					while (iter != figures.end()) {
						(*iter++)->Draw(context, Cam2D);
					}
				}
				void Update(acex::draw::IUpdater* updater) {
					auto iter = figures.begin();
					while (iter != figures.end()) {
						(*iter++)->Update(updater);
					}
				}
			};
		}
	}
}
