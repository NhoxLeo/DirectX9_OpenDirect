#pragma once

#define SAFE_DELETE(x) { if(x) { delete x; x = NULL; } }
#define SAFE_RELEASE(x) { if(x) { x->Release(); x = NULL; } }
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN         // Get rid of rarely-used stuff from Windows headers
#define DIRECTINPUT_VERSION 0x800   // Tell DirectInput what version to use

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib,"d3dx9.lib")

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
#include <iostream>

#include "Singleton.h"
#include "Application.h"
#include "TypeDefined.h"


#include "Renderer.h"
#include "Object.h"
#include "Camera.h"
#include "Director.h"
#include "Texture.h"
#include "ResourceManager.h"
#include "Scene.h"
#include "Sprite.h"
#include "AnimationSprite.h"
#include "Collider.h"
#include "Input.h"
#include "CircleCollider.h"
#include "BoxCollider.h"
#include "CollisionManager.h"
#include "Rigidbody.h"
#include "PhysicsManager.h"
#include "resources.h"
#include "NetworkEntity.h"
//#include "Networker.h"
//#include "Label.h"
//#include "AnimationGroup.h"
//#include "ProgressBar.h"

using namespace std;
