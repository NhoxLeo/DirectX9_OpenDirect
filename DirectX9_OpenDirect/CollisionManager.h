#pragma once
#include "stdafx.h"

class Object;
class CollisionManager : public Singleton<CollisionManager>
{
public:
	CollisionManager();
	~CollisionManager();

	Box GetSweptBroadphaseBox(Box b)
	{
		Box broadphasebox;
		broadphasebox.x = b.vx > 0 ? b.x : b.x + b.vx;
		broadphasebox.y = b.vy > 0 ? b.y : b.y + b.vy;
		broadphasebox.w = b.vx > 0 ? b.vx + b.w : b.w - b.vx;
		broadphasebox.h = b.vy > 0 ? b.vy + b.h : b.h - b.vy;
		return broadphasebox;
	}
	bool AABBCheck(Box b1, Box b2)
	{
		return !(b1.x + b1.w < b2.x || b1.x > b2.x + b2.w || b1.y + b1.h < b2.y || b1.y > b2.y + b2.h);
	}
	float SweptAABB(Box b1, Box b2, float& normalx, float& normaly)
	{
		float xInvEntry, yInvEntry;
		float xInvExit, yInvExit;

		// find the distance between the objects on the near and far sides for both x and y
		if (b1.vx > 0.0f)
		{
			xInvEntry = b2.x - (b1.x + b1.w);
			xInvExit = (b2.x + b2.w) - b1.x;
		}
		else
		{
			xInvEntry = (b2.x + b2.w) - b1.x;
			xInvExit = b2.x - (b1.x + b1.w);
		}

		if (b1.vy > 0.0f)
		{
			yInvEntry = b2.y - (b1.y + b1.h);
			yInvExit = (b2.y + b2.h) - b1.y;
		}
		else
		{
			yInvEntry = (b2.y + b2.h) - b1.y;
			yInvExit = b2.y - (b1.y + b1.h);
		}
		// find time of collision and time of leaving for each axis (if statement is to prevent divide by zero)
		float xEntry, yEntry;
		float xExit, yExit;

		if (b1.vx == 0.0f)
		{
			xEntry = -std::numeric_limits<float>::infinity();
			xExit = std::numeric_limits<float>::infinity();
		}
		else
		{
			xEntry = xInvEntry / b1.vx;
			xExit = xInvExit / b1.vx;
		}

		if (b1.vy == 0.0f)
		{
			yEntry = -std::numeric_limits<float>::infinity();
			yExit = std::numeric_limits<float>::infinity();
		}
		else
		{
			yEntry = yInvEntry / b1.vy;
			yExit = yInvExit / b1.vy;
		}
		// find the earliest/latest times of collision
		float entryTime = (std::max)(xEntry, yEntry);
		float exitTime = (std::min)(xExit, yExit);

		// if there was no collision
		if (entryTime > exitTime || xEntry < 0.0f && yEntry < 0.0f || xEntry > 1.0f || yEntry > 1.0f)
		{
			normalx = 0.0f;
			normaly = 0.0f;
			return 1.0f;
		}
		else // if there was a collision
		{
			// calculate normal of collided surface
			if (xEntry > yEntry)
			{
				if (xInvEntry < 0.0f)
				{
					normalx = 1.0f;
					normaly = 0.0f;
				}
				else
				{
					normalx = -1.0f;
					normaly = 0.0f;
				}
			}
			else
			{
				if (yInvEntry < 0.0f)
				{
					normalx = 0.0f;
					normaly = 1.0f;
				}
				else
				{
					normalx = 0.0f;
					normaly = -1.0f;
				}
			}

			// return the time of collision
			return entryTime;
		}
	}
	float CheckSweptAABB(Object* obj1, Object* obj2, float& normalx, float& normaly);
};
