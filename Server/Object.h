#pragma once
#include <iostream>
struct Vector2
{
	float x, y;
};
struct Vector3
{
	float x, y, z;
};


class Object
{

public:
	Object();
	~Object();

	/*std::string GetName() { return name; }
	void SetName(std::string _name) { name = _name; }
	Vector2 GetPosition() { return position; }
	void SetPosition(Vector2 _position) { position = _position; }
	Vector2 GetRotation() { return position; }
	void SetRotation(Vector2 _rotation) { rotation = _rotation; }
	Vector2 GetScale() { return position; }
	void SetScale(Vector2 _scale) { scale = _scale; }
	bool IsActive() { return isActive; }
	void SetACtive(bool _active) { isActive = _active; }*/

	std::string id, name, tag;
	bool isActive;
	float floatRotation;
	Vector2 position, rotation, scale, size, velocity;

protected:
	/*std::string id, name, tag;
	bool isActive;
	Vector2 position, rotation, scale;*/
};

