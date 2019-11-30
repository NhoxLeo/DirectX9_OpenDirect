#include "Tilemap.h"
#include <iostream>
#include <fstream>
#include <vector>


Tilemap::Tilemap()
{
	initialized = false;
}

Tilemap::Tilemap(Sprite * sprite, const wchar_t * _imagePath, const wchar_t * _txtPath, int _columns, int _rows, int _mapTileColumnCount, int _mapTileRowCount, std::string _name)
{
	/*mapColumn = _columns;
	mapRow = _rows;
	tileColumn = _mapTileColumnCount;
	tileRow = _mapTileRowCount;*/

	initialized = false;

	tileSprite = Sprite::Create(_imagePath);
	tileSprite->SetAnchorPoint(0.f, 0.f);
	tileSprite->SetPosition(0, 0);
	AddChild(tileSprite);

	dataMap = new std::vector<int>();
	std::ifstream file(_txtPath);
	if (file.good())
	{
		std::string curData = "";
		int i = 0;
		while (file >> curData)
		{
			i++;
			int x = atoi(curData.c_str());
			dataMap->push_back(x);
		}
		file.close();
	}
}

Tilemap::~Tilemap()
{
}

void Tilemap::Update(float deltaTime)
{
	Object::Update(deltaTime);
	//if (!initialized)
	//{
	//	for (size_t i = 0; i < dataMap->size(); i++)
	//	{
	//		int _index = dataMap->at(i);
	//		if (_index == 4)
	//		{
	//			int _tileRow = (int)(i / 26);
	//			int _tileColumn = i % 26;
	//			Object* obj = new Object();
	//			obj->SetSize(16, 16);
	//			obj->SetPosition(_tileColumn * 16 + 8, _tileRow * 16 + 8);
	//			obj->AddComponent<BoxCollider>(new BoxCollider());
	//			Director::GetInstance()->GetScene()->AddChild(obj);
	//		}
	//	}
	//	initialized = !initialized;
	//}
}

void Tilemap::Render()
{
	Object::Render();
	for (size_t i = 0; i < 26; i++)
	{
		for (size_t j = 0; j < 26; j++)
		{
			int _index = dataMap->at((i * 26) + j);
			int _tileRow = (int)(_index / 20);
			int _tileColumn = _index % 20;

			tileSprite->SetPosition(j * 16, i * 16);
			RECT rect;
			rect.top = (_tileRow) * 16;
			rect.bottom = (_tileRow + 1) * 16;
			rect.left = (_tileColumn) * 16;
			rect.right = (_tileColumn + 1) * 16;
			tileSprite->SetSourceRect(rect);
			tileSprite->Render();
		}
	}
}