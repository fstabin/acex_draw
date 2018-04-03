#pragma once
#include <cstdint>
#include <memory>
#include <limits>

#include "acs\include\vector.h"
#include "acs\include\matrix.h"
#include "acs\include\limits.h"

#include "acex_draw_ex\include\base.h"
#include "acex_draw_ex\include\model.h"

namespace acex {
	namespace draw {
		namespace ex {

			namespace sprite {

				class Sprite : public model::Drawable {
					acs::SIACS<acex::draw::IDraw> m_Draw;
					size_t m_size;

					acs::SIACS<acex::draw::ICamPro> mr_camera;
					acs::SIACS<acex::draw::IRenderResource> mr_renderResource;
					acex::draw::TEXSAMPLE_MODE m_TSMode = acex::draw::TS_LINEAR;

					acs::SIACS<acex::draw::IIWorld> m_world;
					acs::SIACS<acex::draw::IITexState> m_texstate;
				protected:
					acex::draw::IIWorld* getWorld()const noexcept {
						return m_world;
					}
					acex::draw::IITexState* getTexState()const noexcept {
						return m_texstate;
					}
					explicit Sprite(acex::draw::IDraw* a_draw, size_t a_size, acex::draw::ICamPro* a_cp, acex::draw::IRenderResource* a_res)
						: m_Draw(a_draw), m_size(a_size), mr_camera(a_cp), mr_renderResource(a_res) {
						acex::draw::RESOURCE_DESC rdesc;
						{
							acex::draw::INSTANCEBUFFER_DESC mdesc;
							if (a_size > std::numeric_limits<decltype(mdesc.ArraySize)>::max())throw(ACEXDrawExCreateResourceFailedException("sprite  constracter:a_size is invalid value"));
							mdesc.ArraySize = static_cast<decltype(mdesc.ArraySize)>(a_size);//‰Šú‰»Žž‚É m_size ‚Ì‘å‚«‚³•Ûá
							rdesc.desc = &mdesc;
							rdesc.AccessFlag = acex::draw::RESOURCE_ACCESS_WRITE;
							rdesc.type = acex::draw::RESOURCE_TYPE_INSWORLD;
							if (!a_draw->CreateResource(rdesc, nullptr, (void**)&m_world))throw(acex::draw::ex::ACEXDrawExCreateResourceFailedException());
							rdesc.type = acex::draw::RESOURCE_TYPE_INSTEXSTATE;
							if (!a_draw->CreateResource(rdesc, nullptr, (void**)&m_texstate))throw(acex::draw::ex::ACEXDrawExCreateResourceFailedException());
						}
					}
				public:
					acex::draw::IDraw* getDraw() {
						return m_Draw;
					}
					size_t getSize()const noexcept {
						return m_size;
					}
					bool isEnable() const noexcept {
						return mr_camera.hasPtr() && mr_renderResource.hasPtr();
					}

					void setCamera(acex::draw::ICamPro* campro) noexcept {
						mr_camera = campro;
					}
					acex::draw::ICamPro* getCamera()const noexcept {
						return mr_camera;
					}
					void setRenderResource(acex::draw::IRenderResource* res)noexcept {
						mr_renderResource = res;
					}
					acex::draw::IRenderResource* getRenderResource()const noexcept {
						return mr_renderResource;
					}
					void setSamplerMode(acex::draw::TEXSAMPLE_MODE mode)noexcept {
						m_TSMode = mode;
					}
					acex::draw::TEXSAMPLE_MODE getSamplerMode()const noexcept {
						return m_TSMode;
					}
				};

				struct SPRITE_STATE {
					bool flag;
					acs::vector::f3d pos;
					acs::vector::f2d size;
					acs::vector::f3d rotate;
					acex::draw::INS_TEXSTATE texstate;
				};
				class SimpleSprite : public  Sprite {
					std::unique_ptr<SPRITE_STATE[]> states;
				public:
					virtual ~SimpleSprite() {};
					explicit SimpleSprite(acex::draw::IDraw* draw, size_t spriteCount, acex::draw::ICamPro* a_op_cam = nullptr, acex::draw::IRenderResource* a_op_resource = nullptr)
						:Sprite(draw, spriteCount, a_op_cam, a_op_resource), states(std::make_unique<SPRITE_STATE[]>(spriteCount)) {
						for (size_t i = 0; i < spriteCount; i++)
						{
							states[i] = { false,{ 0,0,0 },{ 1,1 },{ 0,0,0 },{ 0,0,1,1 } };
						}
					}

					SPRITE_STATE& operator[](size_t i) {
						return getState(i);
					}
					const SPRITE_STATE& operator[](size_t i)const {
						return getState(i);
					}
					SPRITE_STATE& getState(size_t index) {
						return states[index];
					}
					const SPRITE_STATE& getState(size_t index) const {
						return states[index];
					}
					void setState(size_t index, SPRITE_STATE stat) {
						states[index] = stat;
					}

					virtual void Draw(acex::draw::IDrawer* context)final {
						if (!this->isEnable()) {
							return;
						}
						acex::draw::RENDER_DATA_SPRITE rd;
						rd.InstanceCount = static_cast<decltype(rd.InstanceCount)>(this->getSize());
						rd.InstanceOffs = 0;

						rd.worlds = this->getWorld();
						rd.texstates = this->getTexState();

						rd.campro = this->getCamera();
						rd.texture = this->getRenderResource();
						rd.sample = this->getSamplerMode();
						context->Draw(acex::draw::RM_SPRITE, &rd);
					}
					virtual void Update(acex::draw::IUpdater* pupd) final {
						acex::draw::ex::Mapped<acex::draw::INS_WORLD>pworld(pupd, this->getWorld());
						acex::draw::ex::Mapped<acex::draw::INS_TEXSTATE>ptexs(pupd, this->getTexState());
						for (size_t i = 0; i < this->getSize(); i++)
						{
							auto& ref = states[i];
							auto& refWor = pworld[i].f4x4;
							if (ref.flag) {
								acs::matrix::m4x4<float> mat;
								acs::matrix::set_pos_size_rotationxyz<float>(mat, ref.pos, acs::vector::f3d(ref.size.x, ref.size.y, 0), ref.rotate);
								memcpy(&refWor, &mat, 4 * 4 * 4);
							}
							else {
								refWor = {
									0,0,0,0,
									0,0,0,0,
									0,0,0,0,
									0,0,0,1
								};
							}

							ptexs[i] = ref.texstate;
						}
					}
				};

				struct COLORSPRITE_STATE {
					bool flag;
					acs::vector::f3d pos;
					acs::vector::f2d size;
					acs::vector::f3d rotate;
					acex::draw::INS_TEXSTATE texstate;
					acex::draw::FCOLOR color;
				};
				class ColorSprite : public  Sprite {
					std::unique_ptr<COLORSPRITE_STATE[]> states;
					acs::SIACS<acex::draw::IVColor> m_color;

				public:
					virtual ~ColorSprite() {};
					//throw acex::draw::exCreateResourceFailedException
					explicit ColorSprite(acex::draw::IDraw* draw, size_t spriteCount, acex::draw::ICamPro* a_op_cam = nullptr, acex::draw::IRenderResource* a_op_resource = nullptr)
						:Sprite(draw, spriteCount, a_op_cam, a_op_resource), states(std::make_unique<COLORSPRITE_STATE[]>(spriteCount)) {
						for (size_t i = 0; i < spriteCount; i++)
						{
							states[i] = { false,{ 0,0,0 },{ 1,1 },{ 0,0,0 },{ 0,0,1,1 },{ 1,1,1,1 } };
						}
						if (!CreateIVColor(&m_color, draw, static_cast<uint32_t>(spriteCount), acex::draw::RESOURCE_ACCESS_WRITE))throw(ACEXDrawExCreateResourceFailedException());
					}

					//throw acex::draw::exCreateResourceFailedException
					explicit ColorSprite(acex::draw::IDraw* draw, acex::draw::COLOR col, size_t spriteCount)
						:Sprite(draw, spriteCount, nullptr, nullptr) {
						acs::SIACS<acex::draw::IRenderResource> resource;
						if (!acex::draw::ex::CreateIRenderResourceM(draw, 1, 1, &col, &resource))throw(ACEXDrawExCreateResourceFailedException());
						this->setRenderResource(resource);
					}

					COLORSPRITE_STATE& operator[](size_t i) {
						return getState(i);
					}
					const COLORSPRITE_STATE& operator[](size_t i) const {
						return getState(i);
					}
					COLORSPRITE_STATE& getState(size_t index) {
						return states[index];
					}
					const COLORSPRITE_STATE& getState(size_t index) const {
						return states[index];
					}
					void setState(size_t index, COLORSPRITE_STATE stat) {
						states[index] = stat;
					}

					virtual void Draw(acex::draw::IDrawer* context) {
						if (!this->isEnable()) { return; }
						RenderSpriteColor(context, static_cast<uint32_t>(this->getSize()), this->getSamplerMode(), this->getWorld(), m_color, this->getTexState(), this->getCamera(), this->getRenderResource());
					}
					virtual void Update(acex::draw::IUpdater* pupd) {
						acex::draw::ex::Mapped<acex::draw::INS_WORLD>pworld(pupd, this->getWorld());
						acex::draw::ex::Mapped<acex::draw::INS_TEXSTATE>ptexs(pupd, this->getTexState());
						acex::draw::ex::Mapped<acex::draw::VERTEX_COLOR>pcol(pupd, this->getTexState());
						for (size_t i = 0; i < getSize(); i++)
						{
							auto& ref = states[i];
							auto& refWor = pworld[i].f4x4;
							if (ref.flag) {
								refWor = {
									ref.size.x,0,0,	ref.pos.x,
									0,	ref.size.y,0,ref.pos.y,
									0,0,1,ref.pos.z,
									0,0,0,1
								};
								float sinx, siny, sinz;
								float cosx, cosy, cosz;
								sinx = std::sin(ref.rotate.x);
								cosx = 1.f - sinx * sinx;

								siny = std::sin(ref.rotate.y);
								cosy = 1.f - siny * siny;

								sinz = std::sin(ref.rotate.z);
								cosz = 1.f - sinz * sinz;
								float m[9] = { cosx * cosy, sinx * cosy, -sinx,
									cosx *siny * sinz - sinx * cosz, sinx * siny * sinz + cosx * cosz, cosy * sinz,
									cosx *siny * cosz + sinx * sinz, sinx * siny * cosz - cosx * sinz, cosy * cosz
								};
								refWor = {
									refWor.m[0][0] * m[0] ,refWor.m[1][1] * m[1], refWor.m[2][2] * m[2],refWor.m[0][3],
									refWor.m[0][0] * m[3] ,refWor.m[1][1] * m[4], refWor.m[2][2] * m[5],refWor.m[1][3],
									refWor.m[0][0] * m[6],refWor.m[1][1] * m[7] ,refWor.m[2][2] * m[8] ,refWor.m[2][3],
									refWor.m[3][0] ,refWor.m[3][1] ,refWor.m[3][2] ,refWor.m[3][3] };

								pcol[i] = states[i].color;
							}
							else {
								refWor = {
									0,0,0,0,
									0,0,0,0,
									0,0,0,0,
									0,0,0,1
								};
							}

							ptexs[i] = ref.texstate;
						}
					}
				};
			}
		}
	}
}
