#pragma once

#include <list>
#include <unordered_map>

#include "acs\include\safety.h"
#include "acs\include\vector.h"

#include "acex_draw_ex.h"

namespace acex {
	namespace draw {
		namespace ex {
			namespace text {

				template<typename T>
				struct OBJ_detach {
					void operator() (T& obj)const {
						if (obj)DeleteObject(obj);
					}
				};

				template<typename T>
				using SOBJ = acs::safety::safe_obj< T, acs::safety::default_attach<T>, OBJ_detach<T> >;

				class TextImageFactory;

				class FontImage;

				struct Brush {
					acs::vector::by3d color;
					SOBJ<HBRUSH> hnd;
					Brush() :hnd(nullptr), color() {};
					Brush(acs::byte r, acs::byte g, acs::byte b) :color(r, g, b), hnd(CreateSolidBrush(RGB(r, g, b))) {
						if (hnd == nullptr)throw (ACEXDrawExCreateResourceFailedException());
					}
					Brush(acs::vector::by3d v) :color(v), hnd(CreateSolidBrush(RGB(v.x, v.y, v.z))) {
						if (hnd == nullptr)throw (ACEXDrawExCreateResourceFailedException());
					}
				};

				struct Font {
					int fontSize;
					SOBJ<HFONT> font;
					Font(TextImageFactory* dev, int _fontSize, const wchar_t* fontName);
					Font() :fontSize(0), font(nullptr) {};
				};

				//フォントイメージ（リソース）
				class FontImage {
					acs::SIACS<acex::draw::IRenderResource> resource;
					size_t slen;
					std::unordered_map<wchar_t, size_t> char_offset;
					size_t wid, hei;
				public:
					FontImage(acex::draw::IRenderResource* res, const wchar_t* str, size_t strlen, size_t w, size_t h) :resource(res), slen(strlen), wid(w), hei(h) {
						for (size_t i = 0; i < strlen; i++)
						{
							char_offset[str[i]] = i;
						}
					}
					FontImage(acex::draw::IRenderResource* res, std::unordered_map<wchar_t, size_t>& m, size_t w, size_t h) :resource(res), wid(w), hei(h) {
						m.swap(char_offset);
						slen = char_offset.size();
					}

					acex::draw::IRenderResource* getRenderResource() {
						return resource;
					}

					//画像の文字のオフセット計算
					//文字がないときfalse
					bool calcTexstate(wchar_t c, acex::draw::INS_TEXSTATE& ts) {
						auto index = char_offset.find(c);
						if (index == char_offset.end())return false;
						size_t x = index->second % wid;
						size_t y = index->second / wid;
						ts = { ((float)x + 0.0001f) / (float)wid, ((float)y + 0.0001f) / (float)hei, 0.9998f / (float)wid,  0.9998f / (float)hei };
						return true;
					}
				};

				enum ALIGN_MODE : acs::byte {
					AM_TOP = 0b1,
					AM_BASELINE = 0b10,
					AM_BOTTOM = 0b11,

					AM_LEFT = 0b100,
					AM_CENTER = 0b1000,
					AM_RIGH = 0b1100,
				};
			}
		}
	}
}