// originally based on the Camera Movement Tutorial on Blackboard;
// takes care of all DirectX functionality required;
// kept in the procedural style of the tutorial

// Includes

#include "stdafx.h"
#include <winsock2.h>
#include <iostream>
#include <string>
#include <mutex>
#include <condition_variable>
#include <set>
#include <thread>
#include <list>
#include <map>
#include <sstream>

#include "ClientSharedData.h"
#include "CommunicateWithServerFunctor.h"
#include "Server.h"

//#pragma comment(lib,"ws2_32.lib") 	// Use this library whilst linking - contains the Winsock2 implementation.

#include <Windows.h>	// Windows library (for window functions, menus, dialog boxes, etc)
#include <d3dx9.h>		// Direct 3D library (for all Direct 3D funtions).

// Defines

#define D3D_DEBUG_INFO
// The structure of a usual vertex with a position and a diffuse colour
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE)
// The structure of a vertex with a position and texture coordinates instead of a colour
#define D3DFVF_TEXTUREVERTEX (D3DFVF_XYZ | D3DFVF_TEX1)

using namespace std;

// Structs 

// bundles everything required to render an object of a specific type (each type has its own vertex buffer)
struct ObjectRenderData
{
	LPDIRECT3DVERTEXBUFFER9 pVertexBuffer;
	int vertexCount;
};
// structure for the usual vertices
struct CUSTOMVERTEX
{
	float x, y, z;
	DWORD colour;
};
// used for the vertices making up the background
struct TEXTUREVERTEX
{
	float x, y, z;
	FLOAT u, v;
};

// Constants

// transforms degree to radian by mutliplying degree with this value
const float DEGREES_TO_RADIANS = D3DX_PI / 180.0f;
// the speed of fired projectiles
const float PROJECTILE_SPEED = 0.3f;
// if the client is not able to connect to the server within this interval, it will be shut down
const long TIMEOUT_INTERVAL = 10000;

// colours used for objects being rendered
const DWORD LIGHT_GREY = D3DCOLOR_ARGB(255, 170, 170, 170);
const DWORD DARK_GREY = D3DCOLOR_ARGB(255, 90, 90, 90);
const DWORD DARK_BLUE = D3DCOLOR_ARGB(255, 0, 0, 120);
const DWORD ARROW_COLOUR = D3DCOLOR_ARGB(255, 0, 255, 0);
const DWORD PROJECTILE_COLOUR = D3DCOLOR_ARGB(255, 255, 255, 0);

// Global variables

// every vertex is assigned an ID when created
static int ID = 0;
// holds data shared between this thread (main thread) and the worker thread responsible for communicating with the server
ClientSharedData sharedData;
// holds all objects that will be drawn in the render function
list<DrawingData> drawObjects;
// maps a specific type of drawable object to the corresponding data required to render it
map<DrawableObject, ObjectRenderData> renderData;
// tells whether the user is able to fire a projectile or not
bool isLoaded = false;

// Used to create the D3DDevice
LPDIRECT3D9 g_pD3D = NULL;
// The rendering device
LPDIRECT3DDEVICE9 g_pd3dDevice = NULL;
// The texture used as background
LPDIRECT3DTEXTURE9 g_pTexture = NULL;
// the font object used to print the user instructions onto the screen
LPD3DXFONT g_pFont = NULL;

// camera position (does not change in this application)
float g_CameraX = 0.0f; // currently not used
float g_CameraY = 0.0f; // currently not used
float g_CameraZ = -50.0f;

// store the extents of the viewport in world space (at z = 0), used to determine when an object leaves the screen
float g_ViewportSizeWidth;
float g_ViewportSizeHeight;

//-----------------------------------------------------------------------------
// Initialise Direct 3D.
// Requires a handle to the window in which the graphics will be drawn.
HRESULT SetupD3D(HWND hWnd)
{
	// Create the D3D object, return failure if this can't be done.
	if (NULL == (g_pD3D = Direct3DCreate9(D3D_SDK_VERSION))) return E_FAIL;

	// Set up the structure used to create the D3DDevice
	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

	// Create the D3DDevice
	if (FAILED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp, &g_pd3dDevice)))
	{
		return E_FAIL;
	}

	// Turn on the Z buffer
	g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, TRUE);

	// Turn off the lighting, as we're using our own vertex colours.
	g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

	return S_OK;
}

//-----------------------------------------------------------------------------
// Release (delete) all the resources used by this program.
// Only release things if they are valid (i.e. have a valid pointer).
// If not, the program will crash at this point.
VOID CleanUp()
{
	// release all vertexBuffers
	for (map<DrawableObject, ObjectRenderData>::iterator it = renderData.begin(); it != renderData.end(); ++it)
	{
		if (it->second.pVertexBuffer != NULL)
		{
			it->second.pVertexBuffer->Release();
		}
	}
	if (g_pTexture != NULL)
		g_pTexture->Release();
	if (g_pd3dDevice != NULL)
		g_pd3dDevice->Release();
	if (g_pD3D != NULL)
		g_pD3D->Release();
	if (g_pFont != NULL)
		g_pFont->Release();
}

// returns an ID for a vertex
int GetID()
{
	return ID++;
}

// resets the ID counter
void ResetID()
{
	ID = 0;
}

// Prepare the background texture
void SetupTextures()
{
	D3DXCreateTextureFromFile(g_pd3dDevice, L"Textures/Background.png", &g_pTexture);
}

// Prepare the font object
void SetupFont()
{
	D3DXFONT_DESC FontDesc = { 24, 0, 400, 0, false, DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_PITCH, L"Arial" };
	D3DXCreateFontIndirect(g_pd3dDevice, &FontDesc, &g_pFont);
}

//-----------------------------------------------------------------------------
// Set up the scene geometry.
// Sets up vertex buffers for all object types that will be present in the scene
HRESULT SetupGeometry()
{
	HRESULT SetupSpaceShip();
	HRESULT SetupArrow();
	HRESULT SetupProjectile();
	HRESULT SetupBackground();

	return(SUCCEEDED(SetupBackground()) && SUCCEEDED(SetupSpaceShip()) && SUCCEEDED(SetupArrow()) && SUCCEEDED(SetupProjectile()));
}

HRESULT SetupBackground()
{
	void SetupTextureVertexGeometry(TEXTUREVERTEX* pV, int index, float x, float y, float z, float u, float v);

	ObjectRenderData ord;
	ord.vertexCount = 6; // vertex counts are fix for each object type

	int bufferSize = ord.vertexCount * sizeof(TEXTUREVERTEX);

	// Now get Direct3D to create the vertex buffer.
	// The vertex buffer doesn't exist at this point, but the pointer to it does (g_pVertexBuffer)
	if (FAILED(g_pd3dDevice->CreateVertexBuffer(bufferSize, 0, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &ord.pVertexBuffer, NULL)))
	{
		return E_FAIL; // if the vertex buffer culd not be created.
	}

	// store the pointer to the vertex buffer and the vertex count for this object type
	renderData.insert(pair<DrawableObject, ObjectRenderData>(Background, ord));

	// Create a pointer to the first vertex in the buffer
	// Also lock it, so nothing else can touch it while the values are being inserted.
	// only lock as much space as required
	TEXTUREVERTEX* pVertices;
	if (FAILED(renderData.at(Background).pVertexBuffer->Lock(0, 0, (void**)&pVertices, 0)))
	{
		return E_FAIL;  // if the pointer to the vertex buffer could not be established.
	}

	// as the background is farther away from the camera than the other objects, the required size of the texture
	// has be calculated
	RECT workerArea;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &workerArea, 0);

	int hRes = workerArea.right - workerArea.left;
	int vRes = workerArea.bottom - workerArea.top;

	float backgroundHeight = ((abs(g_CameraZ) + 10) * tan(D3DX_PI / 8)) * 2.0f;
	float backgroundWidth = backgroundHeight * static_cast<float>(hRes) / vRes;

	// reset ID for every vertex buffer
	ResetID();

	SetupTextureVertexGeometry(pVertices, GetID(), -backgroundWidth / 2.0f, backgroundHeight / 2.0f, 10, 0, 0);
	SetupTextureVertexGeometry(pVertices, GetID(), backgroundWidth / 2.0f, backgroundHeight / 2.0f, 10, 1, 0);
	SetupTextureVertexGeometry(pVertices, GetID(), -backgroundWidth / 2.0f, -backgroundHeight / 2.0f, 10, 0, 1);

	SetupTextureVertexGeometry(pVertices, GetID(), -backgroundWidth / 2.0f, -backgroundHeight / 2.0f, 10, 0, 1);
	SetupTextureVertexGeometry(pVertices, GetID(), backgroundWidth / 2.0f, backgroundHeight / 2.0f, 10, 1, 0);
	SetupTextureVertexGeometry(pVertices, GetID(), backgroundWidth / 2.0f, -backgroundHeight / 2.0f, 10, 1, 1);

	// Unlock the vertex buffer...
	renderData.at(Background).pVertexBuffer->Unlock();

	return S_OK;
}

HRESULT SetupSpaceShip()
{
	void SetupVertexGeometry(CUSTOMVERTEX* pV, int index, float x, float y, float z, DWORD c);

	ObjectRenderData ord;
	ord.vertexCount = 414;

	int bufferSize = ord.vertexCount * sizeof(CUSTOMVERTEX);

	if (FAILED(g_pd3dDevice->CreateVertexBuffer(bufferSize, 0, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &ord.pVertexBuffer, NULL)))
	{
		return E_FAIL;
	}

	renderData.insert(pair<DrawableObject, ObjectRenderData>(SpaceShip, ord));

	CUSTOMVERTEX* pVertices;
	if (FAILED(renderData.at(SpaceShip).pVertexBuffer->Lock(0, 0, (void**)&pVertices, 0)))
	{
		return E_FAIL;
	}

	ResetID();

	// Front part, front
	SetupVertexGeometry(pVertices, GetID(), 3.0f, 0.75f, -0.75f, DARK_BLUE);
	SetupVertexGeometry(pVertices, GetID(), 4.5f, 0.0f, 0.75f, DARK_BLUE);
	SetupVertexGeometry(pVertices, GetID(), 4.5f, 0.0f, -0.75f, DARK_BLUE);

	SetupVertexGeometry(pVertices, GetID(), 4.5f, 0.0f, 0.75f, DARK_BLUE);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, 0.75f, -0.75f, DARK_BLUE);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, 0.75f, 0.75f, DARK_BLUE);

	SetupVertexGeometry(pVertices, GetID(), 4.5f, 0.0f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 4.5f, -0.38f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 4.5f, -0.38f, -0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), 4.5f, -0.38f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 4.5f, 0.0f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 4.5f, 0.0f, 0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), 4.5f, -0.38f, -0.75, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.7f, -0.75f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.7f, -0.75f, -0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), 3.7f, -0.75f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 4.5f, -0.38f, -0.75, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 4.5f, -0.38f, 0.75, LIGHT_GREY);

	// Front part, left side
	SetupVertexGeometry(pVertices, GetID(), 3.0f, 0.75f, -0.75f, DARK_BLUE);
	SetupVertexGeometry(pVertices, GetID(), 4.5f, 0.0f, -0.75f, DARK_BLUE);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, 0.0f, -0.75f, DARK_BLUE);

	SetupVertexGeometry(pVertices, GetID(), 3.0f, 0.0f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 4.5f, 0.0f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 4.5f, -0.38f, -0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), 3.0f, 0.0f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 4.5f, -0.38f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.7f, -0.75f, -0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), 3.0f, 0.0f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.7f, -0.75f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.75f, -0.75f, LIGHT_GREY);

	// Front part, right side
	SetupVertexGeometry(pVertices, GetID(), 3.0f, 0.75f, 0.75f, DARK_BLUE);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, 0.0f, 0.75f, DARK_BLUE);
	SetupVertexGeometry(pVertices, GetID(), 4.5f, 0.0f, 0.75f, DARK_BLUE);

	SetupVertexGeometry(pVertices, GetID(), 3.0f, 0.0f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 4.5f, -0.38f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 4.5f, 0.0f, 0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), 3.0f, 0.0f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.7f, -0.75f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 4.5f, -0.38f, 0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), 3.0f, 0.0f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.75f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.7f, -0.75f, 0.75f, LIGHT_GREY);

	// Main part, left side
	SetupVertexGeometry(pVertices, GetID(), 3.0f, 0.75f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, 0.19f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.75f, -0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), 3.0f, 0.19f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.19f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.75f, -0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), 3.0f, 0.19f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.19f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 1.44f, 0.19f, -0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.19f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 1.44f, -0.19f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 1.44f, 0.19f, -0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.19f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.75f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -3.15f, -0.19f, -0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.75f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -3.15f, -0.75f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -3.15f, -0.19f, -0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), -3.15f, -0.75f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, -0.19f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -3.15f, -0.19f, -0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), -1.23f, 0.19f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -1.23f, -0.19f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.19f, -0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), -1.23f, -0.19f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, -0.19f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.19f, -0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), 2.2f, -0.75f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 1.4f, -1.15f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -2.4f, -0.75f, -0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), 1.4f, -1.15f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -0.85f, -1.15f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -2.4f, -0.75f, -0.75f, LIGHT_GREY);

	// Main part, right side
	SetupVertexGeometry(pVertices, GetID(), 3.0f, 0.75f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.75f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, 0.19f, 0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), 3.0f, 0.19f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.75f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.19f, 0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), 3.0f, 0.19f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 1.44f, 0.19f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.19f, 0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.19f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 1.44f, 0.19f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 1.44f, -0.19f, 0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.19f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -3.15f, -0.19f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.75f, 0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.75f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -3.15f, -0.19f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -3.15f, -0.75f, 0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), -3.15f, -0.75f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -3.15f, -0.19f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, -0.19f, 0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), -1.23f, 0.19f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.19f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -1.23f, -0.19f, 0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), -1.23f, -0.19f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.19f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, -0.19f, 0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), 2.2f, -0.75f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -2.4f, -0.75f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 1.4f, -1.15f, 0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), 1.4f, -1.15f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -2.4f, -0.75f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -0.85f, -1.15f, 0.75f, LIGHT_GREY);

	// Main part, bottom
	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.75f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 2.2f, -0.75f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.75f, -0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), 2.2f, -0.75f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 2.2f, -0.75f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.75f, -0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), 2.2f, -0.75f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 1.4f, -1.15f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 2.2f, -0.75f, -0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), 1.4f, -1.15f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 1.4f, -1.15f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 2.2f, -0.75f, -0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), 1.4f, -1.15f, 0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -0.85f, -1.15f, 0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 1.4f, -1.15f, -0.75f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -0.85f, -1.15f, 0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -0.85f, -1.15f, -0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 1.4f, -1.15f, -0.75f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -0.85f, -1.15f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -2.4f, -0.75f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -0.85f, -1.15f, -0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), -2.4f, -0.75f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -2.4f, -0.75f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -0.85f, -1.15f, -0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), -2.4f, -0.75f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -3.15f, -0.75f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -2.4f, -0.75f, -0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), -3.15f, -0.75f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -3.15f, -0.75f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -2.4f, -0.75f, -0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), -3.15f, -0.75f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, -0.19f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -3.15f, -0.75f, -0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), -4.5f, -0.19f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, -0.19f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -3.15f, -0.75f, -0.75f, LIGHT_GREY);

	// Main part, top

	SetupVertexGeometry(pVertices, GetID(), 3.0f, 0.75f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 1.25f, 0.75f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, 0.75f, 0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), 1.25f, 0.75f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 1.25f, 0.75f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, 0.75f, 0.75f, LIGHT_GREY); // 50

	SetupVertexGeometry(pVertices, GetID(), -4.1f, 0.75f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.75f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.1f, 0.75f, 0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.75f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.75f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.1f, 0.75f, 0.75f, LIGHT_GREY);

	// Main Part, back

	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.75f, -0.75, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.56f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.75f, 0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.56f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.56f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.75f, 0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.56f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.0f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.56f, -0.68f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.0f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.0f, -0.68f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.56f, -0.68f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.56f, -0.08f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.0f, -0.08f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.56f, 0.08f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.0f, -0.08f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.0f, 0.08f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.56f, 0.08f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.56f, 0.68f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.0f, 0.68f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.56f, 0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.0f, 0.68f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.0f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.56f, 0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.0f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, -0.19f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.0f, 0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), -4.5f, -0.19f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, -0.19f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.0f, 0.75f, LIGHT_GREY);

	// top part
	SetupVertexGeometry(pVertices, GetID(), -0.54f, 1.15f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 1.25f, 0.75f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -0.54f, 1.15f, -0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), 1.25f, 0.75f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 1.25f, 0.75f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -0.54f, 1.15f, -0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), -0.54f, 1.15f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 1.25f, 0.75f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.1f, 1.15f, -0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), 1.25f, 0.75f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.1f, 0.75f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.1f, 1.15f, -0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), -0.54f, 1.15f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.1f, 1.15f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), 1.25f, 0.75f, 0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), 1.25f, 0.75f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.1f, 1.15f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.1f, 0.75f, 0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), -4.1f, 1.15f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.1f, 0.75f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.1f, 1.15f, 0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), -4.1f, 0.75f, -0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.1f, 0.75f, 0.75f, LIGHT_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.1f, 1.15f, 0.75f, LIGHT_GREY);

	SetupVertexGeometry(pVertices, GetID(), -0.54f, 1.15f, 0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -0.54f, 1.15f, -0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.1f, 1.15f, 0.75f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -0.54f, 1.15f, -0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.1f, 1.15f, -0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.1f, 1.15f, 0.75f, DARK_GREY);

	// thrusters, left one
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.56f, -0.68f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.56f, -0.68f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.56f, -0.08f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.56f, -0.68f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.56f, -0.08f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.56f, -0.08f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.0f, -0.68f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.0f, -0.08f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.0f, -0.68f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.0f, -0.68f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.0f, -0.08f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.0f, -0.08f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.56f, -0.68f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.0f, -0.68f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.56f, -0.68f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.0f, -0.68f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.0f, -0.68f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.56f, -0.68f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.56f, -0.08f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.56f, -0.08f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.0f, -0.08f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.0f, -0.08f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.56f, -0.08f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.0f, -0.08f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.56f, -0.68f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.0f, -0.68f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.56f, -0.08f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.0f, -0.68f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.0f, -0.08f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.56f, -0.08f, DARK_GREY);

	// thrusters, right one
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.56f, 0.08f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.56f, 0.08f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.56f, 0.68f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.56f, 0.08f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.56f, 0.68f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.56f, 0.68f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.0f, 0.08f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.0f, 0.68f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.0f, 0.08f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.0f, 0.08f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.0f, 0.68f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.0f, 0.68f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.56f, 0.08f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.0f, 0.08f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.56f, 0.08f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.0f, 0.08f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.0f, 0.08f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.56f, 0.08f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.56f, 0.68f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.56f, 0.68f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.0f, 0.68f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -4.5f, 0.0f, 0.68f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.56f, 0.68f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.0f, 0.68f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.56f, 0.08f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.0f, 0.08f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.56f, 0.68f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.0f, 0.08f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.0f, 0.68f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -5.0f, 0.56f, 0.68f, DARK_GREY);

	// guns, left barrel
	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.75f, -0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.98f, -0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.75f, -0.75f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.98f, -0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.98f, -0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.75f, -0.75f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.75f, -0.52f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.7f, -0.75f, -0.52f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.98f, -0.52f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.98f, -0.52f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.7f, -0.75f, -0.52f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.7f, -0.98f, -0.52f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.98f, -0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.98f, -0.52f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.98f, -0.75f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.98f, -0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.98f, -0.52f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.98f, -0.52f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.75f, -0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.7f, -0.75f, -0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.75f, -0.52f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), 3.7f, -0.75f, -0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.7f, -0.75f, -0.52f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.75f, -0.52f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.75f, -0.52f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.98f, -0.52f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.75f, -0.75f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.98f, -0.52f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.98f, -0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.75f, -0.75f, DARK_GREY);

	// guns, middle part
	SetupVertexGeometry(pVertices, GetID(), 3.7f, -0.75f, 0.52f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.7f, -0.98f, 0.52f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.7f, -0.75f, -0.52f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), 3.7f, -0.98f, 0.52f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.7f, -0.98f, -0.52f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.7f, -0.75f, -0.52f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), 3.7f, -0.98f, 0.52f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.98f, 0.52f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.7f, -0.98f, -0.52f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.98f, 0.52f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.98f, -0.52f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.7f, -0.98f, -0.52f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.75f, -0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.98f, -0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.75f, 0.75f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.98f, -0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.98f, 0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.75f, 0.75f, DARK_GREY);

	// guns, right barrel
	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.75f, 0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.75f, 0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.98f, 0.75f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.98f, 0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.75f, 0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.98f, 0.75f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.75f, 0.52f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.98f, 0.52f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.7f, -0.75f, 0.52f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.98f, 0.52f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.7f, -0.98f, 0.52f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.7f, -0.75f, 0.52f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.98f, 0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.98f, 0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.98f, 0.52f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.98f, 0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.0f, -0.98f, 0.52f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.98f, 0.52f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.75f, 0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.75f, 0.52f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.7f, -0.75f, 0.75f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), 3.7f, -0.75f, 0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.75f, 0.52f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 3.7f, -0.75f, 0.52f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.75f, 0.52f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.75f, 0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.98f, 0.52f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.98f, 0.52f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.75f, 0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 5.0f, -0.98f, 0.75f, DARK_GREY);

	// left wing, wing
	SetupVertexGeometry(pVertices, GetID(), 1.44f, 0.19f, -0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -0.85f, 0.19f, -3.05f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -1.23f, 0.19f, -0.75f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -0.85f, 0.19f, -3.05f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -2.38f, 0.19f, -3.05f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -1.23f, 0.19f, -0.75f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), 1.44f, -0.19f, -0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -1.23f, -0.19f, -0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -0.85f, -0.19f, -3.05f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -0.85f, -0.19f, -3.05f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -1.23f, -0.19f, -0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -2.38f, -0.19f, -3.05f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), 1.44f, 0.19f, -0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 1.44f, -0.19f, -0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -0.85f, 0.19f, -3.05f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), 1.44f, -0.19f, -0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -0.85f, -0.19f, -3.05f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -0.85f, 0.19f, -3.05f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -0.85f, 0.19f, -3.05f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -0.85f, -0.19f, -3.05f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -2.38f, 0.19f, -3.05f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -0.85f, -0.19f, -3.05f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -2.38f, -0.19f, -3.05f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -2.38f, 0.19f, -3.05f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -2.38f, 0.19f, -3.05f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -2.38f, -0.19f, -3.05f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -1.23f, 0.19f, -0.75f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -2.38f, -0.19f, -3.05f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -1.23f, -0.19f, -0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -1.23f, 0.19f, -0.75f, DARK_GREY);

	// right wing, wing
	SetupVertexGeometry(pVertices, GetID(), 1.44f, 0.19f, 0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -1.23f, 0.19f, 0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -0.85f, 0.19f, 3.05f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -0.85f, 0.19f, 3.05f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -1.23f, 0.19f, 0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -2.38f, 0.19f, 3.05f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), 1.44f, -0.19f, 0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -0.85f, -0.19f, 3.05f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -1.23f, -0.19f, 0.75f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -0.85f, -0.19f, 3.05f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -2.38f, -0.19f, 3.05f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -1.23f, -0.19f, 0.75f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), 1.44f, 0.19f, 0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -0.85f, 0.19f, 3.05f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), 1.44f, -0.19f, 0.75f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), 1.44f, -0.19f, 0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -0.85f, 0.19f, 3.05f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -0.85f, -0.19f, 3.05f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -0.85f, 0.19f, 3.05f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -2.38f, 0.19f, 3.05f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -0.85f, -0.19f, 3.05f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -0.85f, -0.19f, 3.05f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -2.38f, 0.19f, 3.05f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -2.38f, -0.19f, 3.05f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -2.38f, 0.19f, 3.05f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -1.23f, 0.19f, 0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -2.38f, -0.19f, 3.05f, DARK_GREY);

	SetupVertexGeometry(pVertices, GetID(), -2.38f, -0.19f, 3.05f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -1.23f, 0.19f, 0.75f, DARK_GREY);
	SetupVertexGeometry(pVertices, GetID(), -1.23f, -0.19f, 0.75f, DARK_GREY);

	renderData.at(SpaceShip).pVertexBuffer->Unlock();

	return S_OK;
}

HRESULT SetupProjectile()
{
	void SetupVertexGeometry(CUSTOMVERTEX* pV, int index, float x, float y, float z, DWORD c);

	ObjectRenderData ord;
	ord.vertexCount = 324;

	int vertices = ord.vertexCount;
	int bufferSize = ord.vertexCount * sizeof(CUSTOMVERTEX);

	if (FAILED(g_pd3dDevice->CreateVertexBuffer(bufferSize, 0, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &ord.pVertexBuffer, NULL)))
	{
		return E_FAIL;
	}

	renderData.insert(pair<DrawableObject, ObjectRenderData>(Projectile, ord));

	CUSTOMVERTEX* pVertices;
	if (FAILED(renderData.at(Projectile).pVertexBuffer->Lock(0, 0, (void**)&pVertices, 0)))
	{
		return E_FAIL;
	}

	// create a sphere and fill the buffer with the vertices
	ResetID();

	// should be divisors of 360 respectively 180
	int verticalSlices = 9;
	int horizontalSlices = 5;

	// original radius of the sphere
	float r = 1.0f;
	// radius of the current ring
	float rn = 0.0f;
	// the y-coordinate for all points on the current ring
	float yCoord = 0.0f;
	// radius of the previous ring
	float rnOld = 0.0f;
	// the y-coordinate for all points on the previous ring
	float yCoordOld = 0.0f;

	int angleMod = 360 / verticalSlices;

	// outer loop to determine y-coordinate and and new radius for rings around the sphere
	for (int alpha = 180; alpha >= 0; alpha -= 180 / horizontalSlices)
	{
		rn = r * sin(alpha * DEGREES_TO_RADIANS);
		yCoord = r * cos(alpha * DEGREES_TO_RADIANS);

		// inner loop to compute x and z coordinates for points lying on a ring around the sphere at the current y-coordinate
		for (int beta = 0; beta > -360; beta -= angleMod)
		{
			// create 2 triangles that form a rectangular plate
			SetupVertexGeometry(pVertices, GetID(), rnOld * sin(beta * DEGREES_TO_RADIANS), yCoordOld, rnOld * cos(beta * DEGREES_TO_RADIANS), PROJECTILE_COLOUR);
			SetupVertexGeometry(pVertices, GetID(), rn * sin(beta * DEGREES_TO_RADIANS), yCoord, rn * cos(beta * DEGREES_TO_RADIANS), PROJECTILE_COLOUR);
			SetupVertexGeometry(pVertices, GetID(), rnOld * sin((beta - angleMod) * DEGREES_TO_RADIANS), yCoordOld, rnOld * cos((beta - angleMod) * DEGREES_TO_RADIANS), PROJECTILE_COLOUR);

			SetupVertexGeometry(pVertices, GetID(), rn * sin(beta * DEGREES_TO_RADIANS), yCoord, rn * cos(beta * DEGREES_TO_RADIANS), PROJECTILE_COLOUR);
			SetupVertexGeometry(pVertices, GetID(), rn * sin((beta - angleMod) * DEGREES_TO_RADIANS), yCoord, rn * cos((beta - angleMod) * DEGREES_TO_RADIANS), PROJECTILE_COLOUR);
			SetupVertexGeometry(pVertices, GetID(), rnOld * sin((beta - angleMod) * DEGREES_TO_RADIANS), yCoordOld, rnOld * cos((beta - angleMod) * DEGREES_TO_RADIANS), PROJECTILE_COLOUR);
		}
		rnOld = rn;
		yCoordOld = yCoord;
	}

	renderData.at(Projectile).pVertexBuffer->Unlock();

	return S_OK;
}

HRESULT SetupArrow()
{
	void SetupVertexGeometry(CUSTOMVERTEX* pV, int index, float x, float y, float z, DWORD c);

	ObjectRenderData ord;
	ord.vertexCount = 9;

	int bufferSize = ord.vertexCount * sizeof(CUSTOMVERTEX);

	if (FAILED(g_pd3dDevice->CreateVertexBuffer(bufferSize, 0, D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &ord.pVertexBuffer, NULL)))
	{
		return E_FAIL;
	}

	renderData.insert(pair<DrawableObject, ObjectRenderData>(Arrow, ord));

	CUSTOMVERTEX* pVertices;
	if (FAILED(renderData.at(Arrow).pVertexBuffer->Lock(0, 0, (void**)&pVertices, 0)))
	{
		return E_FAIL;
	}

	ResetID();

	SetupVertexGeometry(pVertices, GetID(), 0.0f, 4.0f, 0.0f, ARROW_COLOUR);
	SetupVertexGeometry(pVertices, GetID(), 0.25f, 3.0f, 0.0f, ARROW_COLOUR);
	SetupVertexGeometry(pVertices, GetID(), -0.25f, 3.0f, 0.0f, ARROW_COLOUR);

	SetupVertexGeometry(pVertices, GetID(), -0.25f, 3.0f, 0.0f, ARROW_COLOUR);
	SetupVertexGeometry(pVertices, GetID(), 0.25f, 3.0f, 0.0f, ARROW_COLOUR);
	SetupVertexGeometry(pVertices, GetID(), -0.25f, 0.0f, 0.0f, ARROW_COLOUR);

	SetupVertexGeometry(pVertices, GetID(), -0.25f, 0.0f, 0.0f, ARROW_COLOUR);
	SetupVertexGeometry(pVertices, GetID(), 0.25f, 3.0f, 0.0f, ARROW_COLOUR);
	SetupVertexGeometry(pVertices, GetID(), 0.25f, 0.0f, 0.0f, ARROW_COLOUR);

	renderData.at(Arrow).pVertexBuffer->Unlock();

	return S_OK;
}

// Set up vertex data
void SetupVertexGeometry(CUSTOMVERTEX* pV, int index, float x, float y, float z, DWORD c)
{
	pV[index].x = x;
	pV[index].y = y;
	pV[index].z = z;
	pV[index].colour = c;
}
void SetupTextureVertexGeometry(TEXTUREVERTEX* pV, int index, float x, float y, float z, float u, float v)
{
	pV[index].x = x;
	pV[index].y = y;
	pV[index].z = z;
	pV[index].u = u;
	pV[index].v = v;
}


//-----------------------------------------------------------------------------
// Set up the view - the view and projection matrices.
void SetupViewMatrices()
{
	// Set up the view matrix.
	// This defines which way the 'camera' will look at, and which way is up.
	D3DXVECTOR3 vCamera(g_CameraX, g_CameraY, g_CameraZ);
	//D3DXVECTOR3 vLookat(0.0f, 0.0f, g_CameraZ + 10.0f);
	D3DXVECTOR3 vLookat(0.0f, 0.0f, 0.0f);

	D3DXVECTOR3 vUpVector(0.0f, 1.0f, 0.0f);
	D3DXMATRIX  matView;
	D3DXMatrixLookAtLH(&matView, &vCamera, &vLookat, &vUpVector);
	g_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);

	RECT workerArea;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &workerArea, 0);

	int hRes = workerArea.right - workerArea.left;
	int vRes = workerArea.bottom - workerArea.top;

	// This transforms 2D geometry into a 3D space.
	D3DXMATRIX matProj;
	D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, static_cast<float>(hRes) / vRes, 1.0f, 100.0f);
	g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);
}

//-----------------------------------------------------------------------------
// Render the scene.

void Render()
{
	// Clear the backbuffer to a black color
	g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

	// Begin the scene
	if (SUCCEEDED(g_pd3dDevice->BeginScene()))
	{
		void RenderSphereTriangles();
		void RenderSpaceShip();
		void RenderArrow();
		void RenderProjectile();
		void RenderBackground();
		void loadProjectile();

		// Define the viewpoint.
		SetupViewMatrices();

		// add new objects to draw if there are any
		{
			lock_guard<std::mutex> newDrawObjectsLock(sharedData.newDrawObjectsMutex);
			if (!sharedData.newDrawObjects.empty())
			{
				while (!sharedData.newDrawObjects.empty())
				{
					DrawingData data = sharedData.newDrawObjects.front();

					if (data.moveX > 0)
					{
						// object is moving to the right -> start drawing on left side of screen 
						data.posX = -g_ViewportSizeWidth / 2.0f - data.sizeX / 2.0f;
					}
					else
					{
						// object is moving to the left -> start drawing on right side of screen 
						data.posX = g_ViewportSizeWidth / 2.0f + data.sizeX / 2.0f;
					}
					if (data.origin == -1)
					{
						data.origin = 1; // mark objects as coming from another client
					}

					data.wasSent = false; // reset flag

					drawObjects.push_back(data);
					sharedData.newDrawObjects.pop();
				}
			}
		}

		// remove objects to draw if some have been destroyed (execute destroy commands from server)
		{
			lock_guard<std::mutex> newDestroyObjectsLock(sharedData.newDestroyObjectsMutex);
			if (!sharedData.newDestroyObjects.empty())
			{
				while (!sharedData.newDestroyObjects.empty())
				{
					list<DrawingData>::iterator it = find(drawObjects.begin(), drawObjects.end(), sharedData.newDestroyObjects.front());
					if (it != drawObjects.end())
					{
						drawObjects.erase(it);
					}
					sharedData.newDestroyObjects.pop();
				}
			}
		}

		// there are active objects that should be drawn
		if (!drawObjects.empty())
		{
			list<DrawingData>::iterator it = drawObjects.begin();
			while (it != drawObjects.end())
			{
				// Update the object's coordinates.
				(*it).posX += (*it).moveX;
				(*it).posY += (*it).moveY;
				(*it).posZ += (*it).moveZ;

				D3DXMATRIX TranslationMatrix;
				D3DXMATRIX RotationMatrix;
				D3DXMATRIX TransformMatrix;

				// create rotation matrix for the current object based on the drawing data
				D3DXMatrixRotationYawPitchRoll(&RotationMatrix, (*it).rotY * DEGREES_TO_RADIANS, (*it).rotX * DEGREES_TO_RADIANS, (*it).rotZ * DEGREES_TO_RADIANS);

				// create translation matrix for the current object
				D3DXMatrixTranslation(&TranslationMatrix, (*it).posX, (*it).posY, (*it).posZ);

				// multiply rotation and translation and apply the transformation
				D3DXMatrixMultiply(&TransformMatrix, &RotationMatrix, &TranslationMatrix);
				g_pd3dDevice->SetTransform(D3DTS_WORLD, &TransformMatrix);

				// use the appropriate render function
				switch ((*it).object)
				{
				case SpaceShip:
					RenderSpaceShip();
					break;
				case Projectile:
					RenderProjectile();
					break;
				case Arrow:
					RenderArrow();
					break;
				case Background:
					RenderBackground();
					break;
				}

				// determine whether disappeared from the screen
				// +1 is a small offset to make sure that the object really is no longer visible
				if ((((*it).moveX > 0) && ((*it).posX > g_ViewportSizeWidth / 2.0f + (*it).sizeX / 2.0f + 1)) || // disappeared at right-hand end of screen
					(((*it).moveX < 0) && ((*it).posX < -g_ViewportSizeWidth / 2.0f - (*it).sizeX / 2.0f - 1)) || // disappeared at left-hand end of screen
					(((*it).moveY > 0) && ((*it).posY > g_ViewportSizeHeight / 2.0f + (*it).sizeY / 2.0f + 1)) || // disappeared at top end end of screen
					(((*it).moveY < 0) && ((*it).posY < -g_ViewportSizeHeight / 2.0f - (*it).sizeY / 2.0f - 1)))  // disappeared at bottom end of screen
				{
					// reload the projectile if the old projectile just went off-screen
					if ((*it).object == Projectile && (*it).origin == -1)
					{
						loadProjectile();
					}

					// remove element from drawing list
					// by using it++ no iterator becomes invalidated and loop can continue
					drawObjects.erase(it++);
				}
				else if ((((*it).moveX > 0) && ((*it).posX > g_ViewportSizeWidth / 2.0f - (*it).sizeX / 2.0f) && (!(*it).wasSent)) || // arrived at right-hand end of screen
					(((*it).moveX < 0) && ((*it).posX < -g_ViewportSizeWidth / 2.0f + (*it).sizeX / 2.0f) && (!(*it).wasSent))) // arrived at left-hand end of screen
				{

					// did the object just reach the end of the screen for the first time (note: wasSent flag)
					// notify server but keep drawing the object until it completely disappears from the screen

					// add a copy of the object's drawing data to the sendObjects
					{
						lock_guard<std::mutex> newSendObjectsLock(sharedData.newSendObjectsMutex);
						sharedData.newSendObjects.push(*it);
					}

					(*it).wasSent = true; // mark the object as having been sent to the next client
					++it; // next object
				}
				else
				{
					// move on to the next element, regular drawing cycle
					++it;
				}
			}
		}

		// draw the user instructions
		void SetText();
		SetText();

		// determine whether a projectile has hit a space ship
		void determineCollisions();
		determineCollisions();

		// End the scene.
		g_pd3dDevice->EndScene();

	}

	// Present the backbuffer to the display.
	g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
}

// provide some user instructions in the lower right-hand corner of the screen
void SetText()
{
	RECT workerArea;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &workerArea, 0);

	int hRes = workerArea.right - workerArea.left;
	int vRes = workerArea.bottom - workerArea.top;

	RECT FontPosition;
	FontPosition.left = 0;
	FontPosition.top = vRes - vRes / 8;
	FontPosition.right = hRes - 20;
	FontPosition.bottom = vRes;

	g_pFont->DrawText(NULL, L"Use the left and right arrow keys to aim.\nFire a projectile by pressing the space bar.", -1, &FontPosition, DT_RIGHT, 0xffffffff);
}

void RenderBackground()
{
	// Render the contents of the vertex buffer. Move this to drawing loop?
	g_pd3dDevice->SetStreamSource(0, renderData.at(Background).pVertexBuffer, 0, sizeof(TEXTUREVERTEX));
	g_pd3dDevice->SetFVF(D3DFVF_TEXTUREVERTEX);

	// Select the texture, and initialise the texture stage state...
	g_pd3dDevice->SetTexture(0, g_pTexture);
	g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);

	g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, renderData.at(Background).vertexCount / 3);

	g_pd3dDevice->SetTexture(0, NULL);
}

void RenderSpaceShip()
{
	g_pd3dDevice->SetStreamSource(0, renderData.at(SpaceShip).pVertexBuffer, 0, sizeof(CUSTOMVERTEX));
	g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);

	g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, renderData.at(SpaceShip).vertexCount / 3);
}

void RenderProjectile()
{
	g_pd3dDevice->SetStreamSource(0, renderData.at(Projectile).pVertexBuffer, 0, sizeof(CUSTOMVERTEX));
	g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);

	g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, renderData.at(Projectile).vertexCount / 3);
}

void RenderArrow()
{
	g_pd3dDevice->SetStreamSource(0, renderData.at(Arrow).pVertexBuffer, 0, sizeof(CUSTOMVERTEX));
	g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);

	g_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, renderData.at(Arrow).vertexCount / 3);
}

//----------------------------------------------------------
// Additional functions (game functionality)

// check if projectiles have collided with space ships (check all projectiles with all spaceships)
void determineCollisions()
{
	bool newProjectileRequired = false;
	bool collisionDetected = false;

	// use a set to prevent the same ship from being inserted multiple times (i.e. when two projectiles hit it in the same frame)
	set<DrawingData> spaceShipsToErase;

	// use computeBoundingBox instead???

	for (list<DrawingData>::iterator it = drawObjects.begin(); it != drawObjects.end(); ++it)
	{
		collisionDetected = false;

		if ((*it).object == Projectile)
		{
			// determine "bounding box" for each projectile

			D3DXVECTOR3 projectileMin;
			projectileMin.x = (*it).posX - (*it).sizeX / 2.0f;
			projectileMin.y = (*it).posY - (*it).sizeY / 2.0f;
			projectileMin.z = (*it).posZ - (*it).sizeZ / 2.0f;

			D3DXVECTOR3 projectileMax;
			projectileMax.x = (*it).posX + (*it).sizeX / 2.0f;
			projectileMax.y = (*it).posY + (*it).sizeY / 2.0f;
			projectileMax.z = (*it).posZ + (*it).sizeZ / 2.0f;


			list<DrawingData>::iterator it2 = drawObjects.begin();
			while ((it2 != drawObjects.end()) && !collisionDetected)
			{
				if ((*it2).object == SpaceShip)
				{
					// determine "bounding box" of each space ship
					// slightly deviate from the precise values as the bounding box does not properly represent the shape of a spaceship

					D3DXVECTOR3 shipMin;
					shipMin.x = (*it2).posX - (*it2).sizeX / 2.0f + 0.5f;
					shipMin.y = (*it2).posY - (*it2).sizeY / 2.0f + 0.5f;
					shipMin.z = (*it2).posZ - (*it2).sizeZ / 2.0f + 0.5f;

					D3DXVECTOR3 shipMax;
					shipMax.x = (*it2).posX + (*it2).sizeX / 2.0f - 0.5f;
					shipMax.y = (*it2).posY + (*it2).sizeY / 2.0f - 0.5f;
					shipMax.z = (*it2).posZ + (*it2).sizeZ / 2.0f - 0.5f;

					// check for collision
					if (projectileMin.x < shipMax.x && projectileMax.x > shipMin.x &&
						projectileMin.y < shipMax.y && projectileMax.y > shipMin.y &&
						projectileMin.z < shipMax.z && projectileMax.z > shipMin.z)
					{
						// Collision detected

						// erasing it immediately may cause some problems with the iterators -> mark it for later destruction
						spaceShipsToErase.insert(*it2);
					}
				}
				++it2;
			}
		}
	}

	// erase the ships that have been marked for destruction
	if (!spaceShipsToErase.empty())
	{
		for (set<DrawingData>::iterator it3 = spaceShipsToErase.begin(); it3 != spaceShipsToErase.end(); ++it3)
		{
			{
				// add the space ship to the objects that will be sent to the server to further process their destruction
				lock_guard<std::mutex> newDestroySendObjectsLock(sharedData.newDestroySendObjectsMutex);
				sharedData.newDestroySendObjects.push((*it3));
			}

			// in case there is the same ship being drawn twice (case: one client, ship has started disappearing on one side)
			// -> erase both (all) ships that are identical to the previously determined ship to be destroyed
			drawObjects.erase(remove(drawObjects.begin(), drawObjects.end(), (*it3)), drawObjects.end());
		}
	}

	// if the projectile belonging to this client has been destroyed, create a new one
	if (newProjectileRequired)
	{
		void loadProjectile();
		loadProjectile();
	}
}

// create drawing data for objects that belong to the level and cannot move from it
void placeLevelObjects()
{
	// create drawing data for the background and add it to the drawing objects

	DrawingData background;
	background.id = -1;
	background.origin = -1;
	background.object = Background;
	background.posX = 0;
	background.posY = 0;
	background.posZ = 0;
	background.moveX = 0;
	background.moveY = 0;
	background.moveZ = 0;
	background.rotX = 0;
	background.rotY = 0;
	background.rotZ = 0;
	background.sizeX = 30;
	background.sizeY = 30;
	background.sizeZ = 0;
	background.wasSent = false;

	drawObjects.push_back(background);

	// create drawing data for the arrow and add it to the drawing objects
	DrawingData arrow;
	arrow.id = -1;
	arrow.origin = -1;
	arrow.object = Arrow;
	arrow.posX = 0;
	arrow.posY = -g_ViewportSizeHeight / 2.0f + 3.0f; // fix position at the bottom of the viewport
	arrow.posZ = 0;
	arrow.moveX = 0;
	arrow.moveY = 0;
	arrow.moveZ = 0;
	arrow.rotX = 0;
	arrow.rotY = 0;
	arrow.rotZ = 0; // initially no rotation
	arrow.sizeX = 6;
	arrow.sizeY = 8;
	arrow.sizeZ = 4;
	arrow.wasSent = false;

	drawObjects.push_back(arrow);
}

// create new drawing data for a projectile and add it to the lists of objects going to be drawed
// called on startup and everytime the old projectile gets destroyed by collision or leaving the screen
void loadProjectile()
{
	DrawingData projectile;

	projectile.id = -1;
	projectile.origin = -1;
	projectile.object = Projectile;
	projectile.posX = 0;
	projectile.posY = -g_ViewportSizeHeight / 2.0f + 3.0f; // starting position same as arrow
	projectile.posZ = 0;
	projectile.moveX = 0;
	projectile.moveY = 0;
	projectile.moveZ = 0;
	projectile.rotX = 0;
	projectile.rotY = 0;

	// set rotation according to rotation of the aiming arrow
	bool found = false;
	list<DrawingData>::iterator it = drawObjects.begin();
	while (it != drawObjects.end() && !found)
	{
		if ((*it).object == Arrow)
		{
			projectile.rotZ = (*it).rotZ;
			found = true;
		}
		++it;
	}

	projectile.sizeX = 2;
	projectile.sizeY = 2;
	projectile.sizeZ = 2;
	projectile.wasSent = false;

	drawObjects.push_back(projectile);
	isLoaded = true;
}

// rotate the arrow helping the user to aim and the projectile accordingly
void rotateArrow(float angle)
{
	// find the projectile belonging to this client
	for (list<DrawingData>::iterator it = drawObjects.begin(); it != drawObjects.end(); ++it)
	{
		if ((*it).object == Arrow)
		{
			if (((*it).rotZ + angle >= -80.0f) && ((*it).rotZ + angle <= 80.0f))
			{
				(*it).rotZ += angle;
			}
		}
		else if ((isLoaded) && ((*it).object == Projectile) && ((*it).origin == -1))
		{
			if (((*it).rotZ + angle >= -80.0f) && ((*it).rotZ + angle <= 80.0f))
			{
				// as long as the projectile is loaded rotate the projectile in accordance to the arrow
				(*it).rotZ += angle;
			}
		}
	}
}

void fireProjectile()
{
	DrawingData* projectile = nullptr;
	DrawingData* arrow = nullptr;

	// find the projectile and arrow belonging to this client
	for (list<DrawingData>::iterator it = drawObjects.begin(); it != drawObjects.end(); ++it)
	{
		if (((*it).origin == -1) && ((*it).object == Projectile))
		{
			projectile = &(*it);
		}
		else if ((*it).object == Arrow)
		{
			arrow = &(*it);
		}
	}

	// determine the direction the aiming vector is pointing to
	D3DXVECTOR3 direction(cos((arrow->rotZ + 90) * DEGREES_TO_RADIANS), sin((arrow->rotZ + 90) * DEGREES_TO_RADIANS), 0);
	// normalise the direction
	D3DXVECTOR3 normalizedDirection;
	D3DXVec3Normalize(&normalizedDirection, &direction);
	// calculate the movement of the projectile in each frame
	D3DXVECTOR3 movement = normalizedDirection * PROJECTILE_SPEED;

	projectile->moveX = movement.x;
	projectile->moveY = movement.y;
	projectile->moveZ = movement.z;

	isLoaded = false;
}

//-----------------------------------------------------------------------------
// The window's message handling function.

LRESULT WINAPI MsgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
	{
		CleanUp();
		PostQuitMessage(0);
		return 0;
	}
	// Respond to a keyboard event; game controls
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_LEFT:
			rotateArrow(2.0f);
			return 0;
			break;
		case VK_RIGHT:
			rotateArrow(-2.0f);
			return 0;
			break;
		case VK_SPACE:
			if (isLoaded)
			{
				fireProjectile();
			}
			return 0;
			break;
		}
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

//-----------------------------------------------------------------------------
// WinMain() - The application's entry point.
// This sort of procedure is mostly standard, and could be used in most
// DirectX applications.

bool g_isClosing = false;
BOOL ConsoleCtrlHandler(DWORD eventType)
{
	if (eventType == CTRL_CLOSE_EVENT)
	{
		g_isClosing = true;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
	// create a console in addition to the window and bind the standard streams to it
	// console will be used to provide some instructions on startup and to read in the ip address of a server to connect to
	FILE *stream;
	AllocConsole();
	freopen_s(&stream, "CONIN$", "r+t", stdin);
	freopen_s(&stream, "CONOUT$", "w+t", stdout);
	freopen_s(&stream, "CONOUT$", "w+t", stderr);

	cout << "Choose 'server' or 'client' by typing it down \n";
	string option;
	do
	{
		cout << "Choose 'server' or 'client' by typing 'server' or 'client' down \n";
		cin >> option;
	} while (option != "server" && option != "client");

	if (option == "server")
	{
		BOOL ConsoleCtrlHandler(DWORD eventType);

		// create a server object
		Server server;
		// start the server and check if startup succeeded
		if (server.start() == 0)
		{
			// register a new console handler to be able to determine when the user closes the console
			SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleCtrlHandler, TRUE);
			// as long as the user doesn't close the console, keep the server running (listening to new connections)
			while (!g_isClosing)
			{
				server.run();
			}
		}

		// the server object's destructor will be called automatically
		return 0;
	}
	else if (option == "client")
	{
		// validate user input; currently only ip4 addresses are accepted
		string ipAddress;
		bool isValidAddress;
		do
		{
			cout << "\nPlease enter ip address of the server you want to connect to.\nOnly IP4 addresses are supported right now.\nType in 'localhost' if you want to connect to a server is running\non this machine\n";
			cin >> ipAddress;

			if (ipAddress == "localhost")
			{
				isValidAddress = true;
			}
			else
			{
				istringstream iss(ipAddress);
				int firstNumber = 0, secondNumber = 0, thirdNumber = 0, fourthNumber = 0;
				char firstChar = ' ', secondChar = ' ', thirdChar = ' ';
				iss >> firstNumber >> firstChar >> secondNumber >> secondChar >> thirdNumber >> thirdChar >> fourthNumber;

				isValidAddress = (firstNumber >= 0) && (firstNumber < 256) && (firstChar == '.') && (secondNumber >= 0) && (secondNumber < 256) &&
					(secondChar == '.') && (thirdNumber >= 0) && (thirdNumber < 256) && (thirdChar == '.') && (fourthNumber >= 0) && (fourthNumber < 256);
			}
		} while (!isValidAddress);
		cout << "\nConnecting to server...";

		// Register the window class
		WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
						 GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
						 L"Networking Assignment", NULL };
		RegisterClassEx(&wc);
		// determine extents of the work area of the screen (identical to full screen size but minus taskbar)
		RECT workArea;
		SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
		int windowX = workArea.left;
		int windowY = workArea.top;
		int windowWidth = workArea.right - workArea.left;
		int windowHeight = workArea.bottom - workArea.top;
		int hRes = workArea.right - workArea.left;
		int vRes = workArea.bottom - workArea.top;
		// Create the application's window (set to size of work area)
		HWND hWnd = CreateWindow(L"Networking Assignment", L"Networking Assignment",
			WS_OVERLAPPEDWINDOW, 0, 0, hRes, vRes,
			GetDesktopWindow(), NULL, wc.hInstance, NULL);
		// Initialize Direct3D
		if (SUCCEEDED(SetupD3D(hWnd)))
		{
			// determine size of the viewport in world space (used in some other functions)
			g_ViewportSizeHeight = (abs(g_CameraZ) * tan(D3DX_PI / 8)) * 2.0f;
			g_ViewportSizeWidth = g_ViewportSizeHeight * static_cast<float>(hRes) / vRes;

			// Create the scene geometry
			if (SUCCEEDED(SetupGeometry()))
			{
				// Show the window (make sure it's maximized -> fullscreen)
				ShowWindow(hWnd, SW_MAXIMIZE);
				UpdateWindow(hWnd);

				// initialize the shared data structure
				sharedData.doRun = true;
				sharedData.isWorkerThreadRunning = true;
				sharedData.connectionEstablished = false;

				// create a worker thread that will create a connection to the server and communicate with it
				thread workerThread = thread(CommunicateWithServerFunctor(), &sharedData, ipAddress.c_str());

				// setup some additional stuff (while worker thread is connecting)
				SetupFont();
				SetupTextures();
				placeLevelObjects();
				loadProjectile();

				// Enter the message loop
				MSG msg;
				ZeroMemory(&msg, sizeof(msg));

				// wait for notification from worker thread that connection to server has been established
				// if connection cannot be established within a specified time interval, close the client
				std::unique_lock<std::mutex> connectionEstablishedLock(sharedData.connectionEstablishedMutex);
				if (sharedData.connectionEstablishedCondition.wait_for(connectionEstablishedLock, chrono::milliseconds(TIMEOUT_INTERVAL), [] {return sharedData.connectionEstablished; }))
				{
					// console no longer needed
					FreeConsole();

					while (msg.message != WM_QUIT && sharedData.isWorkerThreadRunning)
					{
						if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
						{
							TranslateMessage(&msg);
							DispatchMessage(&msg);
						}
						else
						{
							Render();
						}
					}

					// quit message received, terminate worker thread
					sharedData.doRun = false;
				}
				// wait for the worker thread to finish
				workerThread.join();


			}
		}
		cout << "\nClient terminated";
		UnregisterClass(L"Networking Assignment", wc.hInstance);

		return 0;
	}
}
