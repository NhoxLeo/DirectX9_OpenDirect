#include "Precompiled.h"
#include "TestMapScene.h"

INT WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, INT)
{
	Application* app = Application::GetInstance();

	wchar_t* asd = (wchar_t*)L"Test";

	//app->Initialize(L"Universe2D.Sample1", 1280, 720, false);
	app->Initialize(asd, 640, 480, false);

	Director::GetInstance()->SetScene(TestMapScene::Create());

	app->Run();
	app->Release();
}