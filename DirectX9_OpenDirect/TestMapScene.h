#pragma once
#include "stdafx.h"



class TestMapScene : public Scene {
private:
	//Map* m_Map;

	Object* m_UI;

public:
	TestMapScene();
	~TestMapScene();

public:
	CREATE_FUNC(TestMapScene);

public:
	bool Initialize() override;

public:
	void InitializeUI();

public:
	void Update(float deltaTime) override;
	void UpdateCamera();

public:
	void Render() override;
};

