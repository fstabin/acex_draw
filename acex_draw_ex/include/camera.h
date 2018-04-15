#pragma once

#include "acex_draw_ex.h"
#include "acs\include\vector.h"
#include "acs\include\matrix.h"

namespace acex {
	namespace draw {
		namespace ex {
			ACS_EXCEPTION_DEF1(ACEXDrawExCreateCameraFailedException, , ACEXDrawExCreateResourceFailedException);
			enum CAMERA_MODE {
				CM_2D, CM_3D
			};
			class Camera {
			private:
				typedef acex::draw::ICamPro* _icp;
				acs::SIACS<acex::draw::IDraw> parents;
				acs::SIACS<acex::draw::ICamPro> iCameraMap;
				bool update = true;
				CAMERA_MODE mode = CM_2D;

				acs::vector::f3d pos;
				acs::vector::f3d direction;
				acs::vector::f3d up;

				acs::vector::f2d size;
				acs::vector::f2d Znearfar;

			public:
				Camera(acex::draw::IDraw* draw) {
					if (!acex::draw::ex::CreateICamPro(&iCameraMap, draw, acex::draw::RESOURCE_ACCESS_WRITE, nullptr))throw(ACEXDrawExCreateResourceFailedException());
				}

				void setpos(acs::vector::f3d _pos) {
					update = true;
					pos = _pos;
				}
				acs::vector::f3d getpos() {
					return pos;
				}
				void setdirection(acs::vector::f3d _direction) {
					update = true;
					direction = _direction;
				}
				acs::vector::f3d getdirection() {
					return direction;
				}
				void setup(acs::vector::f3d _up) {
					update = true;
					up = _up;
				}
				acs::vector::f3d getup() {
					return up;
				}

				void setsize(acs::vector::f2d _size) {
					update = true;
					size = _size;
				}
				acs::vector::f2d getsize() {
					return size;
				}
				void setzrange(acs::vector::f2d _nf) {
					update = true;
					Znearfar = _nf;
				}
				acs::vector::f2d getzrange() {
					return Znearfar;
				}

				void setmode(CAMERA_MODE b) {
					update = true;
					mode = b;
				}
				CAMERA_MODE getmode() {
					return mode;
				}

				void Update(acex::draw::IUpdater* updater) {
					if (update) {
						update = false;
						acex::draw::ex::Mapped<acex::draw::C_CAMPRO> icam(updater, iCameraMap);
						acs::byte* ptr = reinterpret_cast<acs::byte*>(icam.data());
						acs::matrix::m4x4<float> mat;
						acs::matrix::m4x4<float> mat1;
						switch (mode)
						{
						case acex::draw::ex::CM_2D:
							acs::matrix::set_camera_direction(mat, pos, direction, up);
							memcpy(ptr, &mat, sizeof(float) * 16);
							ptr += sizeof(float) * 16;
							acs::matrix::set_orthographicLH(mat, size, Znearfar);
							memcpy(ptr, &mat, sizeof(float) * 16);
							break;
						case acex::draw::ex::CM_3D:
							acs::matrix::set_camera_direction(mat, pos, direction, up);
							memcpy(ptr, &mat, sizeof(float) * 16);
							ptr += sizeof(float) * 16;
							acs::matrix::set_perspectiveLH(mat, size, Znearfar);
							memcpy(ptr, &mat, sizeof(float) * 16);
							break;
						default:
							break;
						}
					}
				}

				acex::draw::IDraw* getParents() {
					return parents;
				}

				_icp Ptr()const{
#if _DEBUG
					if (iCameraMap == nullptr)throw ACEXDrawExResourceNotInitedException();
#endif
					return iCameraMap;
				}
				operator _icp()const {
					return Ptr();
				}
				_icp* operator&() noexcept {
#if _DEBUG
					if (iCameraMap)assert("Warning!! p != unllptr");
#endif
					return &iCameraMap;
				}
				_icp operator-> ()const noexcept {
					return Ptr();
				}
			};
		}
	}
}