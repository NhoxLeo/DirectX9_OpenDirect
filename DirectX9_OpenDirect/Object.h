#pragma once
#include "stdafx.h"

class Collider;
class Object {
protected:
	bool m_Visible;
	bool m_TransformUpdate;

	Object* m_Parent;
	std::vector<Object*> m_Children;

	Vector2 m_Size;
	Vector2 m_Position;
	Vector2 m_Scale;
	float m_Rotation;

	int m_ZOrder;

	Vector2 m_AnchorPoint;
	Vector2 m_AnchorPointInPoints;

	Matrix m_Matrix;

	std::string m_Name;
	std::string m_Tag;

public:
	static Object* Create();

	Object();
	virtual ~Object();

public:
	virtual bool Initialize();

public:
	virtual void Update(float deltaTime);
	virtual void Render();

	virtual void OnCollision(Collider* collider) {};

	void AddChild(Object* obj);
	void RemoveChild(Object* obj);
	std::vector<Object*> GetChildren() { return m_Children; }

	void CalculateAnchorPointInPoint();
	void CalculateMatrix();

	Object* GetParent() {
		return m_Parent;
	}
	void SetParent(Object* obj) {
		m_Parent = obj;
	}

	virtual const Vector2& GetPosition() const {
		return m_Position;
	}
	virtual void SetPosition(const Vector2& position) {
		m_Position.x = position.x;
		m_Position.y = position.y;

		m_TransformUpdate = true;
	}
	virtual void SetPosition(float x, float y) {
		m_Position.x = x;
		m_Position.y = y;

		m_TransformUpdate = true;
	}

	virtual const Vector2& GetSize() const {
		return m_Size;
	}
	virtual void SetSize(const Vector2& size) {
		m_Size.x = size.x;
		m_Size.y = size.y;

		m_TransformUpdate = true;
	}
	virtual void SetSize(float sx, float sy) {
		m_Size.x = sx;
		m_Size.y = sy;

		m_TransformUpdate = true;
	}

	virtual const Vector2& GetScale() const {
		return m_Scale;
	}
	virtual void SetScale(const Vector2& scale) {
		m_Scale = scale;

		m_TransformUpdate = true;
	}
	virtual void SetScale(float sx, float sy) {
		m_Scale.x = sx;
		m_Scale.y = sy;

		m_TransformUpdate = false;
	}

	virtual const Vector2& GetAnchorPoint() const {
		return m_AnchorPoint;
	}
	virtual void SetAnchorPoint(const Vector2& anchorPoint) {
		m_AnchorPoint = anchorPoint;
		CalculateAnchorPointInPoint();

		m_TransformUpdate = true;
	}
	virtual void SetAnchorPoint(float ax, float ay) {
		m_AnchorPoint.x = ax;
		m_AnchorPoint.y = ay;

		CalculateAnchorPointInPoint();

		m_TransformUpdate = true;
	}

	virtual float GetRotation() const {
		return m_Rotation;
	}
	virtual void SetRotation(float rotation) {
		m_Rotation = rotation;
		m_TransformUpdate = true;
	}

	virtual int GetZOrder() {
		return m_ZOrder;
	}
	virtual void SetZOrder(int z) {
		m_ZOrder = z;
		m_TransformUpdate = true;
	}

	virtual bool GetVisible() const {
		return m_Visible;
	}
	virtual void SetVisible(bool visible) {
		m_Visible = visible;
		m_TransformUpdate = true;
	}

	virtual const std::string& GetName() const {
		return m_Name;
	}
	virtual void SetName(const std::string& name) {
		m_Name = name;
	}

	virtual const std::string& GetTag() const {
		return m_Tag;
	}
	virtual void SetTag(const std::string& tag) {
		m_Tag = tag;
	}

	virtual D3DXMATRIX GetMatrix() {
		return m_Matrix;
	}

	virtual void Translate(Vector2 v) {
		Translate(v.x, v.y);
	}
	virtual void Translate(float x, float y) {
		m_Position.x += x;
		m_Position.y += y;

		m_TransformUpdate = true;
	}
	virtual void Scale(Vector2 sv) {
		Scale(sv.x, sv.y);
	}
	virtual void Scale(float sx, float sy) {
		m_Scale.x += sx;
		m_Scale.y += sy;

		m_TransformUpdate = true;
	}
	virtual void Rotate(float r) {
		m_Rotation += r;
		m_TransformUpdate = true;
	}

	template<class ComponentType>	ComponentType* GetComponent() {
		for (size_t i = 0; i < m_Children.size(); i++)
		{
			if (ComponentType* compo = dynamic_cast<ComponentType*>(m_Children.at(i))) return compo;
		}
		return nullptr;
	}
	template<class ComponentType>	void AddComponent(ComponentType *_component) {
		for (size_t i = 0; i < m_Children.size(); i++)
		{
			if (ComponentType* compo = dynamic_cast<ComponentType*>(m_Children.at(i))) return;
		}
		//m_Children->push_back(_component);
		AddChild(_component);
	}

public:
	void SortZOrder() {
		std::sort(m_Children.begin(), m_Children.end(), [](Object* lhs, Object* rhs) {
			return lhs->GetZOrder() < rhs->GetZOrder();
			});
	}
};