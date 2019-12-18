#include "Object.h"



Object::Object()
{
}


Object::~Object()
{
}

void Object::OnCollisionEnter(Vector2 normal)
{
	if (this->velocity.x != 0)
	{
		this->position.x += this->velocity.x + normal.x;
		this->floatRotation = (this->velocity.x == 1) ? 90 : 270;
	}
	else if (this->velocity.y != 0)
	{
		this->position.y += this->velocity.y + normal.y;
		this->floatRotation = (this->velocity.y == 1) ? 180 : 0;
	}
}
