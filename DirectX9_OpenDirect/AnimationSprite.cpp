#include "Precompiled.h"
#include "AnimationSprite.h"

#include "Sprite.h"

AnimationSprite::AnimationSprite()
	: m_CurrentFrame(0)
	, m_Delay(0)
	, m_FrameCounter(0) {}

AnimationSprite::AnimationSprite(std::wstring filePath, int rows, int collums,float speed)
{
	//mainrender->LoadTexture(_charPath);
	//animationFrameRects.clear();
	/*frameWidth = (mainrender->GetRECT()->right - mainrender->GetRECT()->left) / _collums;
	frameHeight = (mainrender->GetRECT()->bottom - mainrender->GetRECT()->top) / _rows;

	for (int i = 0; i < _rows; i++)
	{
		for (int j = 0; j < _collums; j++)
		{
			RECT *rc = new RECT();
			rc->left = frameWidth * j;
			rc->top = frameHeight * i;
			rc->right = frameWidth * (j + 1);
			rc->bottom = frameHeight * (i + 1);
			animationFrameRects.emplace_back(rc);
		}
	}
	mainrender->SetRECT(*animationFrameRects[0]);
	frameCount = _rows * _collums;*/

	auto spriteSheet = Sprite::Create(filePath);
	InitializeWithAnimation(speed);
	int frameWidth = (spriteSheet->GetSourceRect().right - spriteSheet->GetSourceRect().left) / collums;
	int frameHeight = (spriteSheet->GetSourceRect().bottom - spriteSheet->GetSourceRect().top) / rows;
	
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < collums; j++)
		{
			RECT *rc = new RECT();
			rc->left = frameWidth * j;
			rc->top = frameHeight * i;
			rc->right = frameWidth * (j + 1);
			rc->bottom = frameHeight * (i + 1);
			//animationFrameRects.emplace_back(rc);

			auto spriteFrame = Sprite::Create(filePath);
			spriteFrame->SetAnchorPoint(0.25f, 0.6f);
			spriteFrame->SetParent(this);
			spriteFrame->SetSourceRect(*rc);
			m_SpriteList.push_back(spriteFrame);
		}
	}
}

void AnimationSprite::AddFrame(std::wstring fileName) {
	auto spriteFrame = Sprite::Create(fileName);
	spriteFrame->SetParent(this);
	m_SpriteList.push_back(spriteFrame);
}

AnimationSprite::~AnimationSprite() {}

bool AnimationSprite::InitializeWithAnimation(int delay) {
	m_Delay = delay;

	return true;
}

void AnimationSprite::Update(float deltaTime) {
	Object::Update(deltaTime);

	m_FrameCounter++;
	if (m_FrameCounter >= m_Delay) {
		m_FrameCounter = 0;
		m_CurrentFrame++;
	}

	if (m_CurrentFrame > (m_SpriteList.size() - 1))
		m_CurrentFrame = 0;

	m_SpriteList.at(m_CurrentFrame)->Update(deltaTime);
}

void AnimationSprite::Render() {
	Object::Render();

	m_SpriteList.at(m_CurrentFrame)->Render();
}