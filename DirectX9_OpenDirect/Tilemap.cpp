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
	for (size_t i = 0; i < 26; i++)
	{
		for (size_t j = 0; j < 26; j++)
		{
			int _index = dataMap->at((i * 26) + j);
			if (_index != 0)
			{
				Object* obj = new Object();
				obj->SetName(std::to_string((i * 26) + j));
				obj->SetSize(16, 16);
				obj->SetPosition(Vector2(j * 16, i * 16) + obj->GetSize() / 2);
				if (_index == 4)obj->SetTag("Brick");
				else if (_index == 13)obj->SetTag("Wall");
				else if (_index == 49 || _index == 50 || _index == 53 || _index == 54) obj->SetTag("Eagle");
				AddChild(obj);
				objList.push_back(obj);
			}
		}
	}

	RECT rect;
	rect.top = rect.bottom = rect.left = rect.right = 0;
	tileSprite->SetSourceRect(rect);
}

Tilemap::~Tilemap()
{
}

void Tilemap::Update(float deltaTime)
{
	Object::Update(deltaTime);
	if (!initialized)
	{
		initialized = !initialized;
		for (size_t i = 0; i < objList.size(); i++)
		{
			Director::GetInstance()->GetScene()->AddChild(objList.at(i));
		}
	}
}

void Tilemap::Render()
{
	Object::Render();
	for (size_t i = 0; i < m_Children.size(); i++)
	{
		string tag = m_Children.at(i)->GetTag();
		int _index = 0;
		if (tag == "Brick")			_index = 4;
		else if (tag == "Wall")			_index = 13;
		else if (tag == "Eagle")			_index = 49;
		else _index = 0;

		if (_index != 0)
		{
			int _tileRow = (int)(_index / 20);
			int _tileColumn = _index % 20;
			tileSprite->SetPosition(m_Children.at(i)->GetPosition() - m_Children.at(i)->GetSize() / 2);
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

void Tilemap::EraseObject(int id)
{
	for (int i = 0; i < objList.size(); i++)
	{
		if (objList.at(i)->GetName() == std::to_string(id)) objList.erase(objList.begin() + i);
	}
}

void Tilemap::EraseObject(Vector2 objPosition)
{
	for (int i = 0; i < objList.size(); i++)
	{
		if (objList.at(i)->GetPosition().x == objPosition.x && objList.at(i)->GetPosition().y == objPosition.y)  objList.erase(objList.begin() + i);;
	}
}
