#include "PhysicsManager.h"



PhysicsManager::PhysicsManager()
{
	rigidbodies = new RigidbodyList();
}


PhysicsManager::~PhysicsManager()
{
}

bool PhysicsManager::Update(float deltaTime)
{
	for (size_t i = 0; i < rigidbodies->size(); i++)
	{
		//rigidbodies->at(i)->SetVelocity(0.0f, 9.81f);
	}
	return true;
}

void PhysicsManager::RegisterRigidbody(Rigidbody* rigidbody)
{
	rigidbodies->push_back(rigidbody);
}
