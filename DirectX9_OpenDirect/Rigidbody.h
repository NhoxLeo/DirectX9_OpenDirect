#pragma once
#include "stdafx.h"
class Rigidbody :public Object
{
private:
	Vector2* velocity;
public:
	Rigidbody();
	~Rigidbody();
	void Update(float deltatime) override;
	Vector2 GetVelocity();
	void SetVelocity(float x,float y);
	void SetVelocity(Vector2* v);
};

