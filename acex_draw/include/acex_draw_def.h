#pragma once
#include "acs\include\def.h"

namespace acex {
	namespace draw {
		using ulongValue = uint64_t;//リソースのサイズとか大きめのやつ
		using uintValue = uint32_t;//フラグとか普通サイズのやつ
		using ushortValue = uint16_t;//ちょい短めのやつ
		using ubyteValue = uint8_t;//バイトデータとか用

		struct F4X4 {
			float m[4][4];
		};
		struct SIZE {
			unsigned int Width;
			unsigned int Height;
		};
		struct COLOR {
			ubyteValue r, g, b, a;
		};
		struct AREA {
			float left, top;
			float right, bottom;
		};

		struct TEXTURE2D_SOURCE_INFO {
			uintValue sourceRowPitch;
			uintValue sourceWidth;
			uintValue sourceRowCount;
		};
		struct TEXTURE2D_DEST_INFO {
			uintValue destRowPitch;
			uintValue destWidth;
			uintValue destRowCount;
		};

		struct FVECTOR3 {
			float x;
			float y;
			float z;
		};
		using FPOSITION = FVECTOR3;
		struct FCOLOR {
			float r;
			float g;
			float b;
			float a;
		};

		using VERTEX_POSITION = FPOSITION;
		using VERTEX_COLOR = FCOLOR;
		struct VERTEX_UV {
			float u;
			float v;
		};
		using VERTEX_NORMAL = FVECTOR3;

		struct INS_WORLD {
			F4X4 f4x4;
		};
		struct INS_TEXSTATE {
			float Xoffs;
			float Yoffs;
			float Xsize;
			float Ysize;
		};

		struct SPRITE_STATE {
			float x, y, z;
			float sx, sy, sz;
			float r, g, b, a;
			float u, v, su, sv;
		};

		struct C_CAMERA {
			F4X4 f4x4;
		};
		struct C_PROJECTION {
			F4X4 f4x4;
		};
		struct C_CAMPRO {
			C_CAMERA cam;
			C_PROJECTION pro;
		};

		struct C_LIGHT {
			float pos[4];
			float diffuse[4];//光が当たっているとき
			float ambient[4];//光が当たっていないとき
			float emissive[4];
		};

		/* ----- descs ----- */
		enum RESOURCE_TYPE :uintValue {
			RESOURCE_TYPE_INDEX = 0,
			RESOURCE_TYPE_VERTEXPOS = 1000,
			RESOURCE_TYPE_VERTEXCOLOR = 1001,
			RESOURCE_TYPE_VERTEXUV = 1002,
			RESOURCE_TYPE_VERTEXNORMAL = 1003,
			RESOURCE_TYPE_INSWORLD = 2000,
			RESOURCE_TYPE_INSTEXSTATE = 2001,
			RESOURCE_TYPE_CAMPRO = 3000,
			RESOURCE_TYPE_LIGHT = 3010,

			RESOURCE_TYPE_TEXTURE2D = 10000,
		};
		enum RESOURCE_ACCESS_FLAG :uintValue {
			RESOURCE_ACCESS_NONE = 0,
			RESOURCE_ACCESS_WRITE = 1,
			RESOURCE_ACCESS_READ = 2,
		};
		struct RESOURCE_DESC {
			RESOURCE_TYPE type;
			uintValue AccessFlag;//RESOURCE_ACCESS_FLAG
			const void* desc;//??_DESC
		};

		enum INDEX_FOMAT :uintValue {
			IFMT_U16 = 2,
			IFMT_U32 = 4,
		};
		struct INDEXBUFFER_DESC {
			INDEX_FOMAT fmt;
			uintValue ArraySize;
		};

		struct BUFFER_DESC {
			uintValue ArraySize;
		};

		typedef BUFFER_DESC VERTEXBUFFER_DESC;
		typedef BUFFER_DESC INSTANCEBUFFER_DESC;

		struct CAMPRO_DESC {
		};
		struct LIGHT_DESC {
		};

		enum TEXTURE_USE_FLAG : uintValue {
			TEXUSE_IO = 0,
			TEXUSE_RENDER_RESOURCE = 1,
			TEXUSE_TARGET = 2,
			TEXUSE_DEPTHSTENCIL = 4,
		};
		struct TEXTURE2D_DESC {
			uintValue Width;
			uintValue Height;
			uintValue useflag;
		};

		/* ----- IDs ----- */
		enum VIEW_TYPE :uintValue {
			VIEW_RENDER_RESOURCE = 1,
			VIEW_TARGET = 2,
			VIEW_DEPTHSTENCIL = 3,
		};
	}
}
