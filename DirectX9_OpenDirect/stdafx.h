#pragma once

#define SAFE_DELETE(x) { if(x) { delete x; x = NULL; } }
#define SAFE_RELEASE(x) { if(x) { x->Release(); x = NULL; } }

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <stdio.h>
#include <time.h>

#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <functional>

#include "Singleton.h"
#include "Application.h"
#include "TypeDefined.h"


//#include "Renderer.h"
//#include "Object.h"
//#include "Camera.h"
//#include "Director.h"
//#include "Scene.h"
//#include "Collider.h"
//#include "Input.h"
//#include "Texture.h"
//#include "ResourceManager.h"
//#include "Label.h"
//#include "Sprite.h"
//#include "AnimationGroup.h"
//#include "AnimationSprite.h"
//#include "ProgressBar.h"
//#include "CircleCollider.h"
//#include "BoxCollider.h"