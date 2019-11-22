#include "Tilemap.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <algorithm>
#include <array>


Tilemap::Tilemap()
{
}

Tilemap::Tilemap(Sprite * sprite, const wchar_t * _imagePath, const wchar_t * _txtPath, int _columns, int _rows, int _mapTileColumnCount, int _mapTileRowCount, std::string _name)
{
	tileSprite = sprite;
	tileSprite->SetPosition(0, 0);
	AddChild(tileSprite);
	tileSprite->SetPosition(320, 10);
	std::vector<int> ObjectMarkedTilesetDataNUmber;

	int ads = ObjectMarkedTilesetDataNUmber.size();
	mapSize = Vector2(0, 0);
	scale = Vector2(1, 1);
	position = Vector2(0, 0);

	worldToScreenPosition = position;
	ObjectList = new std::vector<Object*>();
	std::ifstream file(_txtPath);
	data = new std::vector<int>();
	positionList = new std::map<int, Vector2>();
	if (file.good())
	{
		std::string curData = "";

		int i = 0;
		while (file >> curData)
		{
			i++;
			int x = atoi(curData.c_str());
			if (_name == "Pittsburgh")
			{
				if (x != 0) data->push_back(x);
			}
			else
				data->push_back(x);
		}
		file.close();
	}
	//INIT
	for (size_t m = 0; m < _rows; m++)
	{
		for (size_t n = 0; n < _columns; n++)
		{
			RECT* sourceRECT = new RECT();
			int tileSetHorizontalCount = _columns;
			int tileSetVerticalCount = _rows;

			//tile index
			int tileID = (m * _columns) + n;
			int dataYIndex = tileID / tileSetHorizontalCount;
			int dataXIndex = tileID % tileSetHorizontalCount;

			sourceRECT->top = dataYIndex * 16;
			sourceRECT->bottom = sourceRECT->top + 16;
			sourceRECT->left = dataXIndex * 16;
			sourceRECT->right = sourceRECT->left + 16;
			listTileID.insert(std::pair<int, RECT*>(tileID, sourceRECT));
			//listRECTPositions.insert(std::pair<int,Vector2>(tileID, Vector2((n * tileDataWidth) + position.x, (m * tileDataHeight) + position.y, 0)));
		}
	}
	for (size_t m = 0; m < _mapTileRowCount; m++)
	{
		for (size_t n = 0; n < _mapTileColumnCount; n++)
		{
			Vector2* currentPosition = new Vector2(((n * 16) + 16 / 2), ((m * 16) + 16 / 2));
			//currentPosition->x = currentPosition->x * scale.x;
			//currentPosition->y = currentPosition->x * scale.y;
			int tileIndex = (m * _mapTileColumnCount) + n;
			positionList->insert(std::pair<int, Vector2>(tileIndex, *currentPosition));
			for (int tileDex = 0; tileDex < ObjectMarkedTilesetDataNUmber.size(); tileDex++)
			{
				if (data->at(tileIndex) == ObjectMarkedTilesetDataNUmber[tileDex])
				{
					Object* obj = new Object();
					obj->SetPosition(position + *currentPosition);
					obj->SetScale(Vector2(16 * scale.x, 16 * scale.y));
					ObjectList->push_back(obj);
				}
			}
		}
	}

	//thisRenderer = new Renderer(_deviceResource, _imagePath);
	//thisRenderer->SetPivot(Vector2(16 / 2, 16 / 2, 0));

	int a = data->size();
	mapSize.x = _mapTileColumnCount * scale.x * 16;
	mapSize.y = _mapTileRowCount * scale.y * 16;
}

Tilemap::~Tilemap()
{
}

void Tilemap::Update(float deltaTime)
{
	Object::Update(deltaTime);
}

void Tilemap::Render()
{
	Object::Render();
	for (int i = 0; i < data->size(); i++)
	{
		if (listTileID[data->at(i)] != NULL)
		{
			tileSprite->SetSourceRect(*listTileID[data->at(i)]);
			tileSprite->SetPosition(positionList->at(data->at(i)));
			tileSprite->Render();
		}
	}
}