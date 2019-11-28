#include "Rigidbody.h"



Rigidbody::Rigidbody()
{
	velocity = new Vector2(0.0f, 0.0f);
}


Rigidbody::~Rigidbody()
{
}

void Rigidbody::Update(float deltatime)
{
	//this->GetParent()->SetPosition(this->GetParent()->GetPosition() + *velocity*deltatime);
}

Vector2 Rigidbody::GetVelocity()
{
	return *velocity;
}

void Rigidbody::SetVelocity(float x, float y)
{
	velocity->x = x;
	velocity->y = y;
}

void Rigidbody::SetVelocity(Vector2 * v)
{
	velocity->x = v->x;
	velocity->y = v->y;
}
