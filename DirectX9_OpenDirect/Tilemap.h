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

	void SetCamera(Camera* _cam) { mainCamera = _cam; }

	std::vector<Object*>* GetListObjects() { return ObjectList; }
	std::vector<Object*>* GetListAreas() { return listAreas; }
	RECT* GetRegion() { return newRegion; }
	Vector2 GetMapSize() { return mapSize; }

	std::map<int, RECT*> GetListTileIDs() { return listTileID; };
	std::vector<int>* GetData() { return data; };
	std::map<int, Vector2>* GetPositionList() { return positionList; };
	Renderer* GetTilepRenderer() { return thisRenderer; }
private:
	Vector2 position, worldToScreenPosition, scale;
	//Tmx::Map *tilemap;
	std::map<int, Sprite*> tilesetSheet;
	std::map<int, RECT*> listTileID;
	std::map<int, Vector2> listRECTPositions;
	std::vector<Object*> listRenderers;
	Camera* mainCamera;
	std::vector<Object*>* ObjectList;
	std::vector<Object*>* listAreas;
	Renderer* thisRenderer;
	RECT* newRegion;
	std::vector<int>* data;
	std::map<int, Vector2>* positionList;
	Vector2 mapSize;

	Sprite* tileSprite;
};

