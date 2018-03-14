#include "include\text.h"
#define ThrowIfFalse(x) if(!x) throw (ACEXDrawExCreateResourceFailedException());
namespace {
	int MakeTexture(
		acex::draw::IDraw* draw,
		HDC dc,
		acex::draw::IRenderResource** oRS,
		acex::draw::ex::text::Font* font, acex::draw::ex::text::Brush* chara, acex::draw::ex::text::Brush* back,
		const wchar_t* text, int texlen,
		UINT fomat,
		bool cut = false,
		acex::draw::SIZE* size = nullptr) {
		
		int v;
		acs::uint w, h;
		RECT Frect = { 0,0,1,1 };
		SelectObject(dc, font->font);
		v = DrawTextW(
			dc,
			text,
			texlen,
			&Frect,    // テキストを描画する長方形領域
			fomat | DT_CALCRECT
		);
		Frect.bottom = v;
		acs::vector::by3d fcol = chara->color;
		acs::vector::by3d bcol = back->color;
		acex::draw::ex::text::SOBJ<HBITMAP> hbit(CreateBitmap(
			w = Frect.right - Frect.left, h = Frect.bottom - Frect.top,
			1,
			32,
			nullptr
		));

		SelectObject(dc, hbit);
	
		v = FillRect(dc, &Frect, back->hnd);
		SetTextColor(dc, RGB(fcol.z, fcol.y, fcol.x));
		v = SetBkMode(dc, TRANSPARENT);
		v = DrawTextW(
			dc,
			text,
			texlen,
			&Frect,    // テキストを描画する長方形領域
			fomat
		);
		std::unique_ptr<acex::draw::COLOR[]>bitBuffer = std::make_unique<acex::draw::COLOR[]>(w * h);
		GetBitmapBits(
			hbit,
			w * h * 4,
			bitBuffer.get()
		);

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
		if (!acex::draw::ex::CreateIRenderResourceM(draw, w, h, bitBuffer.get(), oRS))throw (acex::draw::ex::ACEXDrawExCreateResourceFailedException());
		if (size) {
			*size =  {w,h};
		}
		return 1;
	}

}

	acex::draw::ex::text::Font::Font(TextImageFactory* dev, int _fontSize, const wchar_t* fontName) :fontSize(_fontSize), font(
			CreateFontW(
				(_fontSize * GetDeviceCaps(dev->getDC(), LOGPIXELSY)) / 96,              // フォントの高さ
				0,            // 平均文字幅
				0,           // 文字送り方向の角度
				0,          // ベースラインの角度
				FW_NORMAL,              // フォントの太さ
				false,           // 斜体にするかどうか
				false,        // 下線を付けるかどうか
				false,        // 取り消し線を付けるかどうか
				DEFAULT_CHARSET,          // 文字セットの識別子
				OUT_OUTLINE_PRECIS,  // 出力精度
				CLIP_CHARACTER_PRECIS,    // クリッピング精度
				NONANTIALIASED_QUALITY,          // 出力品質
				FF_DONTCARE | DEFAULT_PITCH,   // ピッチとファミリ
				fontName           // フォント名
			)
		) {
			if (font == nullptr)throw (acex::draw::ex::ACEXDrawExCreateResourceFailedException());
		}

	int  acex::draw::ex::text::TextImageFactory::CreateTextImage(
		acex::draw::IDraw* draw,
		acex::draw::IRenderResource** oRS,
		acex::draw::ex::text::Font* font, acex::draw::ex::text::Brush* chara, acex::draw::ex::text::Brush* back,
		const wchar_t* text, int texlen,
		acs::byte  align_mode,
		bool cut,
		acex::draw::SIZE* size) {
		acex::draw::ex::text::Brush br;
		if (!back) {
			new (&br)acex::draw::ex::text::Brush(0,0,0);
			back = &br;
		}
		UINT fomat = 0;
		UINT fomaty[] = { DT_TOP , DT_TOP , DT_VCENTER , DT_BOTTOM };
		UINT fomatx[] = { DT_LEFT , DT_LEFT , DT_CENTER , DT_RIGHT };
		fomat |= fomaty[align_mode & 0b11];
		fomat |= fomatx[(align_mode & 0b1100) >> 2];
		return MakeTexture(draw, dc, oRS, font, chara, back, text, texlen, fomat, cut, size);
	}

	acex::draw::ex::text::StaticText::StaticText(acex::draw::IDraw* draw,
		TextImageFactory* factory,
		acex::draw::ex::text::Font* font,
		acex::draw::ex::text::Brush* chara,
		const wchar_t* text, int texlen,
		acs::byte align) :sprite(draw, 1), align_mode(align) {
		acs::SIACS<acex::draw::IRenderResource>res;
		acex::draw::SIZE size;
		if (!factory->CreateTextImage(
			draw, &res, font, chara, nullptr, text, texlen, align, true, &size))throw(ACEXDrawExCreateResourceFailedException());

		base_size = { static_cast<float>(size.Width),static_cast<float>(size.Height) };
		showsize = static_cast<float>(font->fontSize);
		fontsize = static_cast<float>(font->fontSize);
		sprite.setRenderResource(res);
		sprite.setSamplerMode(acex::draw::TS_LINEAR);
		auto& state = sprite[0];
		state.flag = true;
		state.size = base_size;
		state.pos = acs::vector::f3d(0,0,0);
	}

	void acex::draw::ex::text::StaticText::spos() {
		acs::vector::f2d offset;
		switch (align_mode & 0b11) {
		case AM_TOP:
			offset.y = -showsize / 2;
			break;
		case AM_BOTTOM:
			offset.y = +showsize / 2;
			break;
		default:
			offset.y = 0;
			break;
		}
		switch (align_mode & 0b1100) {
		case AM_LEFT:
			offset.x = +(base_size.x - 2) *  showsize / (fontsize * 2);
			break;
		case AM_RIGH:
			offset.x = -(base_size.x - 2) *  showsize / (fontsize * 2);
			break;
		default:
			offset.x = 0;
			break;
		}
		auto& state = sprite[0];
		state.pos = acs::vector::f3d(pos.x + offset.x, pos.y + offset.y, pos.z);
	}

	acex::draw::ex::text::StaticColorText::StaticColorText(acex::draw::IDraw* draw,
		TextImageFactory* factory,
		acex::draw::ex::text::Font* font,
		acex::draw::ex::text::Brush* chara,
		const wchar_t* text, int texlen,
		acs::byte align) :sprite(draw, 1), align_mode(align) {
		acs::SIACS<acex::draw::IRenderResource>res;
		acex::draw::SIZE size;
		if (!factory->CreateTextImage(
			draw, &res, font, chara, nullptr, text, texlen, align, true, &size))throw(ACEXDrawExCreateResourceFailedException());
		base_size = { static_cast<float>(size.Width),static_cast<float>(size.Height) };
		showsize = static_cast<float>(font->fontSize);
		fontsize = static_cast<float>(font->fontSize);
		sprite.setRenderResource(res);
		sprite.setSamplerMode(acex::draw::TS_LINEAR);
		auto& state = sprite.getState(0);
		state.flag = true;
		state.size = base_size;
	}

	void acex::draw::ex::text::StaticColorText::spos() {
		acs::vector::f2d offset;
		switch (align_mode & 0b11) {
		case AM_TOP:
			offset.y = -showsize / 2;
			break;
		case AM_BOTTOM:
			offset.y = +showsize / 2;
			break;
		default:
			offset.y = 0;
			break;
		}
		switch (align_mode & 0b1100) {
		case AM_LEFT:
			offset.x = +(base_size.x - 2) *  showsize / (fontsize * 2);
			break;
		case AM_RIGH:
			offset.x = -(base_size.x - 2) *  showsize / (fontsize * 2);
			break;
		default:
			offset.x = 0;
			break;
		}
		auto& state = sprite.getState(0);
		state.pos = acs::vector::f3d(pos.x + offset.x, pos.y + offset.y, pos.z);
	}