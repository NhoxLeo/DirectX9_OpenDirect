#include "CollisionManager.h"
#include "Object.h"

CollisionManager::CollisionManager()
{
}


CollisionManager::~CollisionManager()
{
}

float CollisionManager::CheckSweptAABB(Object * obj1, Object * obj2, float & normalx, float & normaly)
{
	Box* b1 = new Box();
	Box* b2 = new Box();

	b1->x = obj1->GetPosition().x - obj1->GetSize().x / 2;
	b1->y = obj1->GetPosition().y - obj1->GetSize().y / 2;
	b1->w = obj1->GetSize().x;
	b1->h = obj1->GetSize().y;

	b2->x = obj2->GetPosition().x - obj2->GetSize().x / 2;
	b2->y = obj2->GetPosition().y - obj2->GetSize().y / 2;
	b2->w = obj2->GetSize().x;
	b2->h = obj2->GetSize().y;

	if (!obj1->GetComponent<Rigidbody>()) b1->vx = b1->vy = 0;
	else
	{
		b1->vx = obj1->GetComponent<Rigidbody>()->GetVelocity().x;
		b1->vy = obj1->GetComponent<Rigidbody>()->GetVelocity().y;
	}
	if (!obj2->GetComponent<Rigidbody>()) b2->vx = b2->vy = 0;
	else
	{
		b2->vx = obj2->GetComponent<Rigidbody>()->GetVelocity().x;
		b2->vy = obj2->GetComponent<Rigidbody>()->GetVelocity().y;
	}

	Box boxI = GetSweptBroadphaseBox(*b1);
	Box boxII = GetSweptBroadphaseBox(*b2);

	float result = (AABBCheck(boxI, boxII)) ? SweptAABB(*b1, *b2, normalx, normaly) : 1;
	delete b1;
	delete b2;
	return result;
}
