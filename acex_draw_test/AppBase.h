#pragma once
#if defined WIN32 | defined _WIN64
#include <minwindef.h>
#endif
#include "acs\include\def.h"
namespace AppBase {
	extern HINSTANCE hInstance;
	extern HWND hMainWindow;
	struct SCREEN_SIZE {
		acs::uint width;
		acs::uint height;
	};

	bool ScreenSetup();//スクリーンの表示
	bool ScreenSetSize(const SCREEN_SIZE&);//スクリーンのサイズ設定(ピクセル)

										   //アプリが終了しなければならないときfalseを返す
	bool AppWait(acs::ulong dwMillisec);
	void StartLClickCheck();
	bool CheckLClicked();
	void StartRClickCheck();
	bool CheckRClicked();

	bool CheckKeyDown(unsigned char);

	bool GetScreenSize(AppBase::SCREEN_SIZE&);
}