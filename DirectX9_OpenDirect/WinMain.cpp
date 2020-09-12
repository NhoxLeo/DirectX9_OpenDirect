#include "Precompiled.h"
#include "TestMapScene.h"


#define win32_lean_and_mean         // get rid of rarely-used stuff from windows headers
#define directinput_version 0x800   // tell directinput what version to use
// link header files required to compile the project
#define _winsock_deprecated_no_warnings
#include "stdafx.h"
#include <commctrl.h>   // common controls for windows (needed for the ip address control)

INT WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, INT)
{
	Application* app = Application::GetInstance();

	wchar_t* asd = (wchar_t*)L"Test";
	app->Initialize(asd, 416, 416, false);
	//app->Initialize(L"Universe2D.Sample1", 1280, 720, false);
	Director::GetInstance()->SetScene(TestMapScene::Create());
	app->Run();
	app->Release();
}