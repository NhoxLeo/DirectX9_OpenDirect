#pragma once
#include "stdafx.h"
class CollisionManager: public Singleton<CollisionManager>
{
public:
	CollisionManager();
	~CollisionManager();

	void SweptAABB();
	float SweptAABB(Box b1, Box b2, float& normalx, float& normaly);
};

