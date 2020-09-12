#include "Precompiled.h"
#include "Director.h"

#include"stdafx.h"

Director::Director() : m_CurrentScene(nullptr)
{
}
Director::~Director()
{
	Release();
}

bool Director::Initialize()
{
	return true;
}
bool Director::Release()
{
	m_CurrentScene->~Scene();
	return true;
}

bool Director::Update(float deltaTime)
{
	if (m_CurrentScene == nullptr)
		return false;

	m_CurrentScene->Update(deltaTime);

	return true;
}
bool Director::Render()
{
	if (m_CurrentScene == nullptr)
		return false;

	m_CurrentScene->Render();

	return true;
}

bool Director::SetScene(Scene* scene)
{
	if (m_CurrentScene)
	{
		SAFE_DELETE(m_CurrentScene);
	}

	m_CurrentScene = scene;

	return true;
}

Vector2 Director::GetWindowSize()
{
	return Vector2(
		static_cast<float>(Application::GetInstance()->GetWidth()),
		static_cast<float>(Application::GetInstance()->GetHeight()));
}
