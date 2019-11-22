#include "NetworkEntity.h"


NetworkEntity::NetworkEntity()
{
}

NetworkEntity::NetworkEntity(int _newId, Sprite * _newSprite)
{
	id = _newId;
	entity = _newSprite;
	AddChild(entity);
}


NetworkEntity::~NetworkEntity()
{
}

