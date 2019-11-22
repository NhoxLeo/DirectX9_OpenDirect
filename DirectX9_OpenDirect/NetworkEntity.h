#pragma once
#include "stdafx.h"

class NetworkEntity : public Object
{
public:
	NetworkEntity();
	NetworkEntity(int _newId, Sprite* _newSprite);
	~NetworkEntity();

	int id;
	Sprite* entity;

};

