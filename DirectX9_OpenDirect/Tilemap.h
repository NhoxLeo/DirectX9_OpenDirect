#pragma once
#include "stdafx.h"
#include<map>

class Tilemap :public Object
{
public:
	Tilemap();
	Tilemap(Sprite* sprite, const wchar_t * _imagePath, const wchar_t * _txtPath, int _columns, int _rows, int _mapTileColumnCount, int _mapTileRowCount, std::string _name);
	~Tilemap();
	void Update(float deltaTime) override;
	void Render() override;
	void EraseObject(int id);
	vector<int>* dataMap;
private:
	Sprite* tileSprite;
	bool initialized = false;
	std::vector<Object*> objList;
	/*int mapColumn;
	int mapRow;
	int tileColumn;
	int tileRow;*/
};

