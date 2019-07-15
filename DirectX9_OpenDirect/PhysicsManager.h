#pragma once
#include "stdafx.h"

class Rigidbody;
class PhysicsManager : public Singleton<PhysicsManager>
{
private:
	typedef std::vector<Rigidbody*> RigidbodyList;
	RigidbodyList* rigidbodies;

public:
	const Vector2 gravity = Vector2(0, 9.81f);

	PhysicsManager();
	~PhysicsManager();
	bool Update(float deltatime);
	void RegisterRigidbody(Rigidbody* rigidbody);
};

