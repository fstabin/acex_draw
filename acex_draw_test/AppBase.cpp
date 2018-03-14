#include "stdafx.h"
#define CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <thread>
#include <atomic>
#include <combaseapi.h>
#include <shared_mutex>
#include <mutex>
#include <WindowsX.h>

#include "resource.h"
#include "AppBase.h"
#include "Main.h"

#define MAX_LOADSTRING 100

#define WM_USER_SCREEN_SETUP ( WM_USER + 1 )
#define WM_USER_SCREEN_SETSIZE ( WM_USER_SCREEN_SETUP + 1 )

// グローバル変数:
HINSTANCE AppBase::hInstance;
HWND AppBase::hMainWindow;

//std::mutex WindowGuard;
AppBase::SCREEN_SIZE Screen;

WCHAR szTitle[MAX_LOADSTRING];                  // タイトル バーのテキスト
WCHAR szWindowClass[MAX_LOADSTRING];            // メイン ウィンドウ クラス名

std::shared_mutex mutCursor;
POINT pCursor;
std::atomic_bool bWindowClosed = false;
std::atomic_bool bLClick = false;
std::atomic_bool bRClick = false;

std::atomic_bool bKeyStates[255];


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
ATOM                RegisterWindowClass(HINSTANCE hInstance, WNDPROC proc);
HWND                CreateMyWindow(HINSTANCE, int);

void MainThreadFunc();
int WindowThreadFunc();
using AppBase::hMainWindow;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	_CrtDumpMemoryLeaks();
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// グローバル文字列を初期化
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_WNDCLASS, szWindowClass, MAX_LOADSTRING);

	if (FAILED(
		CoInitializeEx(nullptr, COINIT_MULTITHREADED)))
	{
		return FALSE;
	}

	AppBase::hInstance = hInstance;
	//初期化
	
	{

		//ウィンドウ作成
		auto a = RegisterWindowClass(hInstance, WndProc);
		if (NULL == (hMainWindow = CreateMyWindow(hInstance, nCmdShow)))
		{
			return FALSE;
		}

	}
	for (size_t i = 0; i < 255; i++)
	{
		bKeyStates[i] = false;
	}
	int RenderResult;
	{
		std::thread thAppMain;
		thAppMain = std::thread(MainThreadFunc);
		RenderResult = WindowThreadFunc();
		thAppMain.join();
	}

	CoUninitialize();
	return RenderResult;
}

void MainThreadFunc() {
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	std::unique_ptr<App::Main> main = std::make_unique<App::Main>();
	try
	{
		main->Func();
	}
	catch (const std::exception& except)
	{
		MessageBoxA(hMainWindow, except.what(), "Err", MB_ICONINFORMATION);
	}
	PostMessage(hMainWindow, WM_CLOSE, 0, 0);
	CoUninitialize();
}
int WindowThreadFunc() {
	MSG msg;
	HACCEL hAccelTable = LoadAccelerators(AppBase::hInstance, MAKEINTRESOURCE(IDC_ACCELERATORS));
	while (true) {
		if (GetMessage(&msg, nullptr, 0, 0) > 0)
		{
			if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else break;
	}
	bWindowClosed = true;
	return (int)msg.wParam;
}

ATOM RegisterWindowClass(HINSTANCE hInstance, WNDPROC proc)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = proc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = 0;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = 0;

	return RegisterClassExW(&wcex);
}
HWND CreateMyWindow(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_MINIMIZEBOX | WS_CAPTION | WS_SYSMENU,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return nullptr;
	}
	return hWnd;
}
void SetBestWindowSize(HWND hWnd) {
	RECT rClient;
	RECT rWindow;
	::GetClientRect(hWnd, &rClient);
	::GetWindowRect(hWnd, &rWindow);

	int newWidth = (rWindow.right - rWindow.left) - (rClient.right - rClient.left) + Screen.width;
	int newHeight = (rWindow.bottom - rWindow.top) - (rClient.bottom - rClient.top) + Screen.height;

	::SetWindowPos(hWnd, nullptr, 0, 0, newWidth, newHeight, SWP_NOMOVE | SWP_NOZORDER);
}
void SetWindowMode(HWND hWnd) {
	::ShowWindow(hWnd, SW_SHOWDEFAULT);
	::UpdateWindow(hWnd);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_MOUSEMOVE:
	{
		std::lock_guard<std::shared_mutex>(std::ref(mutCursor));
		pCursor.x = GET_X_LPARAM(lParam);
		pCursor.y = GET_Y_LPARAM(lParam);
	}
	break;
	case WM_LBUTTONDOWN:
		bLClick = true;
		break;
	case WM_RBUTTONDOWN:
		bRClick = true;
		break;
	case WM_KEYDOWN:
		if (wParam > 255)break;
		bKeyStates[wParam] = true;;
		break;
	case WM_KEYUP:
		if (wParam > 255)break;
		bKeyStates[wParam] = false;
		break;
	case WM_KILLFOCUS:
		for (size_t i = 0; i < 255; i++)
		{
			bKeyStates[i] = false;
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_USER_SCREEN_SETUP:
		SetWindowMode(hWnd);
		break;
	case WM_USER_SCREEN_SETSIZE:
		SetBestWindowSize(hWnd);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

bool AppBase::AppWait(acs::ulong dwMillisec) {
	//ウィンドウが閉じられたかどうかを返す
	for (DWORD i = 0; i <= dwMillisec; i++)
	{
		if (bWindowClosed)return false;
		SleepEx(1, false);
	}
	return true;
}

void AppBase::StartLClickCheck() {
	bLClick = false;
}
bool AppBase::CheckLClicked() {
	return bLClick;
}

void AppBase::StartRClickCheck() {
	bRClick = false;
}
bool AppBase::CheckRClicked() {
	return bRClick;
}

bool AppBase::CheckKeyDown(unsigned char code) {
	return bKeyStates[code];
}

bool AppBase::GetScreenSize(AppBase::SCREEN_SIZE& size) {
	size = Screen;
	return true;
}

bool AppBase::ScreenSetup() {
	::PostMessageW(hMainWindow, WM_USER_SCREEN_SETUP, 0, 0);
	return true;
}
bool AppBase::ScreenSetSize(const AppBase::SCREEN_SIZE& size) {
	Screen = size;
	::PostMessageW(hMainWindow, WM_USER_SCREEN_SETSIZE, 0, 0);
	return true;
}