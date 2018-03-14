#pragma once

#include <algorithm>
#include <memory>

#include "acex_draw_ex\include\text_resource.h"
#include "acex_draw_ex\include\sprite.h"
#include "acs\include\codeset.h"

namespace acex {
	namespace draw {
		namespace ex {
			namespace text {
				enum FONT_LANGUAGE_TYPE {
					FLT_JAPANESE,
				};

				//テキストイメージファクトリ
				class TextImageFactory {
					HDC dc;
				public:
					ACS_NO_COPY(TextImageFactory);
					TextImageFactory(acex::draw::IDraw* draw)
						try {
						init(draw);
					}
					catch (const ACEXDrawExCreateResourceFailedException&)
					{
						throw;
					}

					~TextImageFactory() {
						destroy();
					}

					bool init(acex::draw::IDraw* draw) {
						destroy();
						dc = CreateCompatibleDC(nullptr);
						return true;
					}
					void destroy() {
						if (dc)DeleteDC(dc);
					}

					int CreateTextImage(
						acex::draw::IDraw* draw,
						acex::draw::IRenderResource** oRS,
						acex::draw::ex::text::Font* font, acex::draw::ex::text::Brush* chara, acex::draw::ex::text::Brush* back,
						const wchar_t* text, int texlen,
						acs::byte  align_mode = AM_LEFT | AM_TOP,
						bool cut = true,
						acex::draw::SIZE* size = nullptr);

				private:
					bool createImageFromBMP(acex::draw::IDraw* draw, HBITMAP bmp, acs::uint w, acs::uint h, bool cut, acs::vector::by3d bcol, acex::draw::IRenderResource** rs) {
						//コピー用メモリ確保
						std::unique_ptr<acex::draw::COLOR[]>bitBuffer = std::make_unique<acex::draw::COLOR[]>(w * h);
						GetBitmapBits(
							bmp,
							w * h * 4,
							bitBuffer.get()
						);

						//データを変換+コピー
						for (size_t i = 0; i < w * h; i++)
						{
							if (cut) {
								if (bcol == acs::vector::by3d(bitBuffer[i].r, bitBuffer[i].g, bitBuffer[i].b)) {
									bitBuffer[i].a = 0x00;
									continue;
								}
							}
							bitBuffer[i].a = 0xff;
						}

						//描画リソース作成
						if (!acex::draw::ex::CreateIRenderResourceM(draw, w, h, bitBuffer.get(), rs))return false;
						return true;
					}

					//フォントイメージ作成
					bool CreateFontImage(
						FontImage*& oRS,
						acex::draw::IDraw* draw,
						acex::draw::ex::text::Font* font, acex::draw::ex::text::Brush* chara, acex::draw::ex::text::Brush* back,
						std::unordered_map<wchar_t, size_t>& codes,
						bool cut = true) {

						//背景ブラシ設定
						acex::draw::ex::text::Brush br;
						if (!back) {
							new (&br)acex::draw::ex::text::Brush(0, 0, 0);
							back = &br;
						}
						int v;					//返り値
						acs::uint wlet = (1 << 12) / font->fontSize, hlet;	//イメージの幅,高さ
						acs::uint w = wlet * font->fontSize, h;	//文字の個数　横　縦
						acs::uint ch;	//文字の高さ
						RECT Frect = { 0,0,1,1 };
						SelectObject(dc, font->font);

						ch = font->fontSize;
						acs::vector::by3d fcol = chara->color;
						acs::vector::by3d bcol = back->color;

						hlet = (static_cast<acs::uint>(codes.size() - 1) / wlet) + 1;
						h = hlet * font->fontSize;
						acex::draw::ex::text::SOBJ<HBITMAP> hbit(CreateBitmap(
							w, h,
							1,
							32,
							nullptr
						));
						SelectObject(dc, hbit);

						v = FillRect(dc, &Frect, back->hnd);//背景塗り
						v = SetBkMode(dc, TRANSPARENT);//文字背景透過
						SetTextColor(dc, RGB(fcol.z, fcol.y, fcol.x));//文字色設定

						//イメージ書き込み
						Frect = { 0,0, (LONG)w, (LONG)font->fontSize };
						for (std::pair<wchar_t, size_t> p : codes)
						{
							Frect.left = ch * static_cast<LONG>(p.second % wlet);
							Frect.right = Frect.left + ch;
							Frect.top = ch * static_cast<LONG>(p.second / wlet);
							Frect.bottom = Frect.top + ch;
							v = DrawTextW(
								dc,
								&p.first,
								1,
								&Frect,
								DT_CENTER | DT_VCENTER
							);
						}

						//描画リソース作成
						acs::SIACS<acex::draw::IRenderResource> rs;
						if (!createImageFromBMP(draw, hbit, w, h, cut, bcol, &rs))return false;
						oRS = new FontImage(rs, codes, wlet, hlet);
						return true;
					}
				public:

					//フォントイメージ作成
					bool CreateFontImage(
						FontImage*& oRS,
						acex::draw::IDraw* draw,
						acex::draw::ex::text::Font* font, acex::draw::ex::text::Brush* chara, acex::draw::ex::text::Brush* back,
						const wchar_t* text, int texlen,
						bool cut = true) {
						std::unordered_map<wchar_t, size_t> m;
						int i = 0;
						for (size_t i = 0;i < texlen;i++)
						{
							m.insert(std::make_pair(text[i], i));
						}
						return CreateFontImage(oRS, draw, font, chara, back, m);
					}

					//フォントイメージ作成
					bool CreateFontImage(
						FontImage*& oRS,
						acex::draw::IDraw* draw,
						acex::draw::ex::text::Font* font, acex::draw::ex::text::Brush* chara, acex::draw::ex::text::Brush* back,
						acs::code::CodeSet cset,
						bool cut = true) {
						std::unordered_map<wchar_t, size_t> m;
						acs::uint i = 0;
						for (std::pair<wchar_t, wchar_t> p : cset.getSet())
						{
							for (wchar_t c = p.first; c <= p.second; c++)
							{
								if (m.count(c) == 0) {
									m.insert(std::make_pair(c, i));
									i++;
								}
							}
						}
						return CreateFontImage(oRS, draw, font, chara, back, m);
					}

					HDC getDC() {
						return dc;
					}
				};

				class TextObj : public model::Drawable {
				};

				//テキスト
				class ImageText :public TextObj {
				private:
					sprite::ColorSprite m_sprite;
					acs::vector::f3d m_pos;//左上座標
					acs::vector::f2d m_letter_size;//文字サイズ
					std::wstring m_str;//文字列
					size_t m_dlen;//表示文字数

				protected:
					FontImage * const m_image;
					sprite::ColorSprite& getSprite() {
						return m_sprite;
					}

				public:
					ACS_NO_COPY(ImageText);
					explicit ImageText(acex::draw::IDraw* draw, FontImage* fontimg, size_t capacity) :m_sprite(draw, capacity), m_image(fontimg) {
						m_sprite.setRenderResource(fontimg->getRenderResource());
						m_sprite.setSamplerMode(acex::draw::TS_LINEAR);
						setTextColor({ 1,1,1,1 });
					}

					//表示　位置　大きさ等----------------------------------------------

					void setPos(acs::vector::f3d pos3) {
						m_pos = pos3;
					}
					const acs::vector::f3d& getPos()const {
						return m_pos;
					}

					void setLetterSize(float size) {
						m_letter_size = acs::vector::f2d(1, 1) * size;
					}
					void setLetterSize(acs::vector::f2d& size) {
						m_letter_size = acs::vector::f2d(size);
					}
					const acs::vector::f2d& getLetterSize() const {
						return m_letter_size;
					}

					//文字列--------------------------------------------------

					void setText(const wchar_t* text) {
						m_str = text;
						m_dlen = static_cast<size_t>(m_str.length());
					}
					const wchar_t* getText()const {
						return m_str.data();
					}
					size_t getTextLength() const {
						return m_str.size();
					}
					void setTextColor(const acex::draw::FCOLOR& a_color) {
						for (size_t i = 0;i < m_sprite.getSize();i++)
						{
							m_sprite[i].color = a_color;
						}
					}
					const acex::draw::FCOLOR& getTextColor()const {
						return m_sprite[0].color;
					}
					void setTextDisplayLength(size_t len) {
						m_dlen = len;
					}
					size_t getTextDisplayLength() const {
						return m_dlen;
					}

					//その他-------------------------------------------------------

					void setCamera(acex::draw::ICamPro* cam) {
						m_sprite.setCamera(cam);
					}

					acex::draw::IRenderResource* getRenderResource() {
						return m_sprite.getRenderResource();
					}

					virtual void Update(acex::draw::IUpdater* upd) {
						acs::vector::f3d p = m_pos + acs::vector::f3d(m_letter_size.x / 2, -m_letter_size.y / 2, 0);
						size_t i = 0;
						size_t lender = std::min<size_t>(m_sprite.getSize(), static_cast<size_t>(m_str.size()));
						lender = std::min<size_t>(m_dlen, lender);
						auto text = m_str.data();
						for (; i < lender; i++)
						{
							if (text[i] == '\n') {
								p.x = m_pos.x + m_letter_size.x / 2;
								p.y += -m_letter_size.y;
							}
							else {
								auto& state = m_sprite[i];
								state.pos = p;
								state.size = m_letter_size;
								state.flag = m_image->calcTexstate(m_str.data()[i], state.texstate);
								p += acs::vector::f3d(m_letter_size.x, 0, 0);
							}
						}
						for (; i < m_sprite.getSize(); i++) {
							auto& state = m_sprite[i];
							state.flag = false;
						}
						m_sprite.Update(upd);
					}

					virtual void Draw(acex::draw::IDrawer* context) {
						m_sprite.Draw(context);
					}
				};

				class TextBox :public ImageText {
					ACS_NO_COPY(TextBox);
				private:
					size_t m_lengthPerLine = 10;
					acs::vector::f2d m_boxSize;
					acs::vector::f2d mTextOffsetPos;

					unsigned int m_textInterval = 0;
					unsigned int m_textTimer = 0;
				public:
					TextBox(acex::draw::IDraw* draw, FontImage* fontimg, size_t capacity, acs::vector::f2d boxSize) :ImageText(draw, fontimg, capacity), m_boxSize(boxSize) {

					}

					void setTextInterval(unsigned int  speed) {
						m_textInterval = speed;
						m_textTimer = 0;
					}
					unsigned int getTextInterval() const {
						return m_textInterval;
					}

					void setTextOffsetPos(acs::vector::f2d offsetPos) {
						mTextOffsetPos = offsetPos;
					}
					acs::vector::f2d getTextOffsetPos() {
						return mTextOffsetPos;
					}

					void setBoxSize(acs::vector::f2d boxSize) {
						m_boxSize = boxSize;
					}
					const acs::vector::f2d& getBoxSize()const {
						return m_boxSize;
					}

					void queryText() {
						if (m_textInterval == 0)return;
						//未表示の文字がまだある
						if (getTextLength() > getTextDisplayLength()) {
							m_textTimer++;
							if (m_textInterval <= m_textTimer) {
								m_textTimer = 0;
								setTextDisplayLength(getTextDisplayLength() + 1);
							}
						}
					}

					virtual void Update(acex::draw::IUpdater* upd) {
						acs::vector::f2d letterSize = getLetterSize();
						acs::vector::f3d pos = getPos();
						acs::vector::f3d p = getPos() + acs::vector::f3d(letterSize.x / 2, -letterSize.y / 2, 0);
						p.x += mTextOffsetPos.x;
						p.y += mTextOffsetPos.y;
						size_t i = 0;
						size_t lender = std::min<size_t>(getSprite().getSize(), static_cast<size_t>(getTextDisplayLength()));
						lender = std::min<size_t>(static_cast<size_t>(getTextLength()), lender);
						auto text = getText();
						for (; i < lender; i++)
						{
							if (text[i] == '\n') {
								p.x = pos.x + getLetterSize().x / 2;
								p.y += -getLetterSize().y;
								++lender;
							}
							else {
								//文字がはみ出すとき改行
								if (p.x + letterSize.x / 2 > pos.x + m_boxSize.x) {
									p.x = pos.x + letterSize.x / 2;
									p.y += -letterSize.y;
									++lender;
								}
								//ｙではみ出すとき終了
								if (p.y - letterSize.y / 2 < pos.y - m_boxSize.y) {
									break;
								}
								auto& state = getSprite()[i];
								state.pos = p;
								state.size = letterSize;
								state.flag = m_image->calcTexstate(text[i], state.texstate);
								p += acs::vector::f3d(letterSize.x, 0, 0);
							}
						}
						for (; i < getSprite().getSize(); i++) {
							auto& state = getSprite()[i];
							state.flag = false;
						}
						getSprite().Update(upd);
					}
				};

				class StaticText :public TextObj {
				protected:
					sprite::SimpleSprite sprite;
					acs::vector::f3d pos;
					acs::vector::f2d base_size;
					acs::byte align_mode;
					float showsize;
					float fontsize;
					bool updated = true;

					void spos();
				public:
					ACS_NO_COPY(StaticText);
					StaticText(acex::draw::IDraw* draw,
						TextImageFactory* factory,
						acex::draw::ex::text::Font* font,
						acex::draw::ex::text::Brush* chara,
						const wchar_t* text, int texlen,
						acs::byte align = AM_LEFT | AM_TOP);

					void init(acex::draw::IDraw* draw,
						TextImageFactory* factory,
						acex::draw::ex::text::Font* font,
						acex::draw::ex::text::Brush* chara,
						const wchar_t* text, int texlen,
						acs::byte align = AM_LEFT | AM_TOP) {
						this->~StaticText();
						new (this) StaticText(draw, factory, font, chara, text, texlen, align);
					}

					void setSize(float size) {
						showsize = size;
						sprite[0].size = base_size * size / fontsize;
					}
					void setPos(acs::vector::f3d pos3) {
						pos = pos3;
					}
					void setCamera(acex::draw::ICamPro* cam) {
						sprite.setCamera(cam);
					}

					acex::draw::IRenderResource* GetRenderResource() {
						return sprite.getRenderResource();
					}

					virtual void Update(acex::draw::IUpdater* upd) {
						spos();
						sprite.Update(upd);
					}
					virtual void Draw(acex::draw::IDrawer* context) {
						sprite.Draw(context);
					}
				};

				class StaticColorText :public TextObj {
					sprite::ColorSprite sprite;
					acs::vector::f3d pos;
					acs::vector::f2d base_size;
					acs::byte align_mode;
					float showsize;
					float fontsize;
					bool updated = true;

					void spos();
				public:
					ACS_NO_COPY(StaticColorText);
					StaticColorText(acex::draw::IDraw* draw,
						TextImageFactory* factory,
						acex::draw::ex::text::Font* font,
						acex::draw::ex::text::Brush* chara,
						const wchar_t* text, int texlen,
						acs::byte align = AM_LEFT | AM_TOP);

					void init(acex::draw::IDraw* draw,
						TextImageFactory* factory,
						acex::draw::ex::text::Font* font,
						acex::draw::ex::text::Brush* chara,
						const wchar_t* text, int texlen,
						acs::byte align = AM_LEFT | AM_TOP) {
						this->~StaticColorText();
						new (this) StaticColorText(draw, factory, font, chara, text, texlen, align);
					}

					void setSize(float size) {
						showsize = size;
						sprite[0].size = base_size * size / fontsize;
					}
					void setPos(acs::vector::f3d pos3) {
						pos = pos3;
					}
					void setColor(acex::draw::VERTEX_COLOR col) {
						sprite[0].color = col;
					}
					void setCamera(acex::draw::ICamPro* cam) {
						sprite.setCamera(cam);
					}

					virtual void Update(acex::draw::IUpdater* upd) {
						spos();
						sprite.Update(upd);
					}
					virtual void Draw(acex::draw::IDrawer* context) {
						sprite.Draw(context);
					}
				};

				class TextWriter {
					std::list<TextObj*>lobj;
				public:
					ACS_NO_COPY(TextWriter);
					void AddText(TextObj* obj) {
						lobj.push_back(obj);
					}
					void RemoveText(TextObj* obj) {
						lobj.remove(obj);
					}
					void ClearText() {
						lobj.clear();
					}
					virtual void Update(acex::draw::IUpdater* upd) {
						auto iter = lobj.begin();
						while (iter != lobj.end()) {
							(*iter)->Update(upd);
							++iter;
						}
					}
					virtual void Draw(acex::draw::IDrawer* context) {
						auto iter = lobj.begin();
						while (iter != lobj.end()) {
							(*iter)->Draw(context);
							++iter;
						}
					}

				};
			}
		}
	}
}
