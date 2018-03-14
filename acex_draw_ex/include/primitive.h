#pragma once

#include "acs\include\attribute.h"
#include "acs\include\matrix.h"

#include "model.h"

namespace acex {
	namespace draw {
		namespace ex {
			namespace primitive {
				struct SQUARE_INIT_STATE {
					float l, t, r, b;//left,top,right,bottom
				};
				class Square : public acs::CopyDisable {
					acs::SIACS<acex::draw::IIndex> rIndex;
					acs::SIACS<acex::draw::IVPosition> rPos;
					acs::SIACS<acex::draw::IVUv> rUv;
				public:
					Square() {}
					Square(acex::draw::IDraw* draw, const SQUARE_INIT_STATE& st) {
						uint16_t IndexData[] = { 0,1,2,3 };
						acex::draw::VERTEX_POSITION VertexPos[] = { { st.l,st.t,0 },{ st.r,st.t,0 },{ st.l,st.b,0 },{ st.r,st.b,0 } };
						acex::draw::VERTEX_UV VertexUv[] = { { 0,0 },{ 1,0 },{ 0,1 },{ 1,1 } };
						acex::draw::RESOURCE_DESC rdesc;
						acex::draw::INDEXBUFFER_DESC idesc;
						idesc.ArraySize = 4;
						idesc.fmt = acex::draw::IFMT_U16;
						rdesc.AccessFlag = acex::draw::RESOURCE_ACCESS_NONE;
						rdesc.type = acex::draw::RESOURCE_TYPE_INDEX;
						rdesc.desc = &idesc;
						if (!draw->CreateResource(rdesc, IndexData, (void**)&rIndex))throw(ACEXDrawExCreateResourceFailedException());
						acex::draw::VERTEXBUFFER_DESC vdesc;
						vdesc.ArraySize = 4;
						rdesc.AccessFlag = acex::draw::RESOURCE_ACCESS_NONE;
						rdesc.type = acex::draw::RESOURCE_TYPE_VERTEXPOS;
						rdesc.desc = &vdesc;
						if (!draw->CreateResource(rdesc, VertexPos, (void**)&rPos))throw(ACEXDrawExCreateResourceFailedException());
						rdesc.type = acex::draw::RESOURCE_TYPE_VERTEXUV;
						if (!draw->CreateResource(rdesc, VertexUv, (void**)&rUv))throw(ACEXDrawExCreateResourceFailedException());
					};
					virtual ~Square() {};

					acex::draw::IIndex* getIIndex() {
						return rIndex;
					}
					acex::draw::IVPosition* getIVPos() {
						return rPos;
					}
					acex::draw::IVUv* getVUV() {
						return rUv;
					}
					acex::draw::PRIMITIVE_TOPOLOGY getPT() {
						return acex::draw::PRIMITIVE_TOPOLOGY::PT_TRIANGLESTRIP;
					}
				};

				class Line : public acs::CopyDisable, public model::Drawable, public model::Moveable, public model::Hideable {
					acs::SIACS<acex::draw::IIndex> rIndex;
					acs::SIACS<acex::draw::IVPosition> rPos;
					acs::SIACS<acex::draw::IVColor> rColor;
					acs::SIACS<acex::draw::IIWorld> rWorld;
					acs::SIACS<acex::draw::ICamPro> Camera;
					acs::vector::f2d mSegment[2] = { {0,0},{0, 0} };
					acex::draw::FCOLOR mColor;
					float mThickness = 0;
				public:
					explicit Line(acex::draw::IDraw* draw, acex::draw::FCOLOR color) :mColor(color) {
						uint16_t IndexData[] = { 0,1,2,3 };
						acex::draw::FCOLOR ColorData[] = { color,color,color,color };
						if (!acex::draw::ex::CreateIIndex(&rIndex, draw, acex::draw::IFMT_U16, 4, acex::draw::RESOURCE_ACCESS_READ | acex::draw::RESOURCE_ACCESS_WRITE, IndexData))throw(ACEXDrawExCreateResourceFailedException());
						if (!acex::draw::ex::CreateIVPosition(&rPos, draw, 4, acex::draw::RESOURCE_ACCESS_READ | acex::draw::RESOURCE_ACCESS_WRITE, nullptr))throw(ACEXDrawExCreateResourceFailedException());
						if (!acex::draw::ex::CreateIVColor(&rColor, draw, 4, acex::draw::RESOURCE_ACCESS_READ | acex::draw::RESOURCE_ACCESS_WRITE, ColorData))throw(ACEXDrawExCreateResourceFailedException());
						if (!acex::draw::ex::CreateIIWorld(&rWorld, draw, 1, acex::draw::RESOURCE_ACCESS_READ | acex::draw::RESOURCE_ACCESS_WRITE, nullptr))throw(ACEXDrawExCreateResourceFailedException());
					};
					virtual ~Line() {};

					acex::draw::IIndex* getIIndex() {
						return rIndex;
					}
					acex::draw::IVPosition* getIVPos() {
						return rPos;
					}
					acex::draw::IVColor* getIVColor() {
						return rColor;
					}
					acex::draw::PRIMITIVE_TOPOLOGY getPT() {
						return acex::draw::PRIMITIVE_TOPOLOGY::PT_TRIANGLESTRIP;
					}
					acex::draw::IIWorld* getIIWorld() {
						return rWorld;
					}

					void setCamPro(acex::draw::ICamPro* cmapro) {
						Camera = cmapro;
					}
					acex::draw::ICamPro* getICamPro() {
						return Camera;
					}

					void setLineColor(acex::draw::FCOLOR aColor) {
						mColor = aColor;
					}
					acex::draw::FCOLOR getLineColor() {
						return mColor;
					}

					void setLineThickness(float aThickness) {
						mThickness = aThickness;
					}
					float getLineThickness()const { return mThickness; }

					void setSegment(acs::vector::f2d begin, acs::vector::f2d end) {
						mSegment[0] = begin;
						mSegment[1] = end;
					}
					acs::vector::f2d getSegmentBegin() const {
						return mSegment[0];
					}
					acs::vector::f2d getSegmentEnd() const {
						return mSegment[1];
					}

					virtual void Draw(acex::draw::IDrawer* context) {
						if (getIIWorld() != nullptr && Camera.hasPtr() && this->isShownModel())acex::draw::ex::RenderWorld(context, 1, 4, 4, getPT(), getIIndex(), getIVPos(), getIVColor(), getIIWorld(), Camera);
					}
					virtual void Update(acex::draw::IUpdater* pupd) {
						auto v = mSegment[1] - mSegment[0];
						v = acs::vector::normalize(v);
						acs::vector::f2d v2(-v.y, v.x);
						acex::draw::ex::Mapped<acex::draw::VERTEX_POSITION>pvpos(pupd, this->getIVPos());
						auto toVPOS = [&](acs::vector::f2d pos)->acex::draw::VERTEX_POSITION {
							return{ pos.x, pos.y, 0 };
						};
						v2 = v2 * mThickness / 2.;
						pvpos[0] = toVPOS(mSegment[0] + v2);
						pvpos[1] = toVPOS(mSegment[1] + v2);
						pvpos[2] = toVPOS(mSegment[0] - v2);
						pvpos[3] = toVPOS(mSegment[1] - v2);
						acex::draw::ex::Mapped <acex::draw::FCOLOR> pcolor(pupd, this->getIVColor());
						for (size_t i = 0; i < 4; i++)pcolor[i] = mColor;
						acex::draw::ex::Mapped <acs::matrix::m4x4<float> > pworld(pupd, this->getIIWorld());
						acs::matrix::m4x4<float> mat;
						acs::matrix::set_pos_size_rotationxyz<float>(mat, getModelPos(), acs::vector::f3d(1, 1, 1), acs::vector::f3d(0, 0, 0));
						memcpy(pworld.data(), &mat, 4 * 4 * 4);
					}
				};


			}
		}
	}
}