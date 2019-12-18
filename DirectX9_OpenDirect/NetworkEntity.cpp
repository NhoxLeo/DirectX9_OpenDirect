#include "NetworkEntity.h"


NetworkEntity::NetworkEntity()
{
	isActive = false;
}

NetworkEntity::NetworkEntity(int _newId, Sprite * _newSprite)
{
	id = _newId;
	entity = _newSprite;
	this->SetSize(_newSprite->GetSize().x, _newSprite->GetSize().y);
	isActive = false;
	AddChild(entity);
}


NetworkEntity::~NetworkEntity()
{
}

