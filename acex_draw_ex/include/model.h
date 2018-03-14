#pragma once
#include <cstdint>
#include <memory>

#include "acs\include\vector.h"
#include "acs\include\matrix.h"
#include "acs\include\limits.h"

#include "acex_draw_ex\include\model.h"
#include "acex_draw_ex\include\base.h"

namespace acex {
	namespace draw {
		namespace ex {
			namespace model {

				class Drawable {
				public:
					virtual void Draw(acex::draw::IDrawer* context) = 0;
					virtual void Update(acex::draw::IUpdater* pupd) = 0;
				};

				class Moveable {
					acs::vector::f3d m_pos;
				public:
					void setModelPos(const acs::vector::f3d& pos) {
						m_pos = pos;
					}
					acs::vector::f3d& getModelPos() {
						return m_pos;
					}
					const acs::vector::f3d& getModelPos() const {
						return m_pos;
					}
				};

				class Resizable {
					acs::vector::f3d m_size;
				public:
					void setModelSize(const acs::vector::f3d& size) {
						m_size = size;
					}
					acs::vector::f3d& getModelSize() {
						return m_size;
					}
					const acs::vector::f3d& getModelSize() const {
						return m_size;
					}
				};

				class Hideable {
					bool mShow = false;
				public:
					void showModel() {
						mShow = true;
					}
					void hideModel() {
						mShow = false;
					}
					bool isShownModel() {
						return mShow;
					}
				};

			}
		}
	}
}
