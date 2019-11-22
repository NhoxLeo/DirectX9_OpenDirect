#pragma once

#define SAFE_DELETE(x) { if(x) { delete x; x = NULL; } }
#define SAFE_RELEASE(x) { if(x) { x->Release(); x = NULL; } }

#define WIN32_LEAN_AND_MEAN

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

#include "Singleton.h"
using namespace std;

typedef D3DXVECTOR2 Vector2;
typedef D3DXVECTOR3 Vector3;
typedef D3DXMATRIX Matrix;