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
					acs::SIACS<acex::draw::IRenderResource> mrImage;
					size_t mUseCharCnt;
					std::unordered_map<wchar_t, size_t> mCharOffs;
					size_t mCharRowLen, mCharRowAmount;
				public:
					FontImage(acex::draw::IRenderResource* rImage, const wchar_t* useChars, size_t useCharCnt, size_t charRowLen, size_t charRowAmount) 
						:mrImage(rImage), mUseCharCnt(useCharCnt), mCharRowLen(charRowLen), mCharRowAmount(charRowAmount) {
						mCharOffs.reserve(mUseCharCnt);
						for (size_t i = 0; i < mUseCharCnt; i++)
						{
							mCharOffs[useChars[i]] = i;
						}
					}
					FontImage(acex::draw::IRenderResource* rImage, std::unordered_map<wchar_t, size_t>&& useCharOffsMap, size_t w, size_t h)
						:mCharOffs(useCharOffsMap), mrImage(rImage), mCharRowLen(w), mCharRowAmount(h) {
							mUseCharCnt = mCharOffs.size();
					}

					acex::draw::IRenderResource* getRenderResource() {
						return mrImage;
					}

					//画像の文字のオフセット計算
					//文字がないときfalse
					bool calcTexstate(wchar_t c, acex::draw::INS_TEXSTATE& ts) {
						auto index = mCharOffs.find(c);
						if (index == mCharOffs.end())return false;
						size_t x = index->second % mCharRowLen;
						size_t y = index->second / mCharRowLen;
						ts = { ((float)x + 0.0001f) / (float)mCharRowLen, ((float)y + 0.0001f) / (float)mCharRowAmount, 0.9998f / (float)mCharRowLen,  0.9998f / (float)mCharRowAmount };
						return true;
					}
				
					size_t getCharRowLen()const { return mCharRowLen; };
					size_t getCharRowAmount() const{ return mCharRowAmount; }
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