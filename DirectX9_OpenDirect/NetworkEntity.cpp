#include "NetworkEntity.h"


NetworkEntity::NetworkEntity()
{
}

NetworkEntity::NetworkEntity(int _newId, Sprite * _newSprite)
{
	id = _newId;
	entity = _newSprite;
	this->SetSize(_newSprite->GetSize().x, _newSprite->GetSize().y);
	AddChild(entity);
}


NetworkEntity::~NetworkEntity()
{
}

