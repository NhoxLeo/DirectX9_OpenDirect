#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN         // Get rid of rarely-used stuff from Windows headers
#define DIRECTINPUT_VERSION 0x800   // Tell DirectInput what version to use

// Link header files required to compile the project
#include <winsock2.h>   // Basic networking library
#include <windows.h>    // Standard Windows header
#include <commctrl.h>   // Common controls for Windows (needed for the IP address control)
#include <dinput.h>     // Include DirectInput information
#include <d3dx9.h>      // Extended functions for managing Direct3D
#include <d3d9.h>       // Basic Direct3D functionality
#include <iostream>     // Used for error reporting
#include "animation.h"  // Controls animated X models
#include "resource.h"   // Icon
#include <stdio.h>


// Constants used in the program
#define DECRYPT_STREAM_SIZE 512                             /* File decryption byte window */
#define BACKGROUND_COLOR    D3DCOLOR_XRGB( 128, 128, 255 )  /* Sky-blue background color */
#define WINSOCK_VERSION     MAKEWORD(2,2)                   /* Use Winsock version 2.2 */
#define SERVER_COMM_PORT    27192                           /* Connect to port 27192 */
#define MAX_PACKET_SIZE     1024                            /* Largest packet is 1024 bytes */
#define MAX_USERS           16                              /* Maximum of 16 players */
#define UPDATE_FREQUENCY        10                          /* Update 10 times per second */
#define IDLE_UPDATE_FREQUENCY   2                           /* When idle, update twice a second */

// Tracks in the tiny_4anim.x animation file
#define TINYTRACK_RUN           1
#define TINYTRACK_WALK          2
#define TINYTRACK_IDLE          3

// When an error occurs, this has a value
LPCSTR g_strError = NULL;

/**
 * Stores a single coordinate in the terrain vertex buffer
 *   @author Karl Gluck
 */
struct TerrainVertex
{
	FLOAT x, y, z;
	FLOAT u, v;
};

/// Vertex definition for Direct3D
#define D3DFVF_TERRAINVERTEX (D3DFVF_XYZ|D3DFVF_TEX1)


/**
 * Player data recieved from the server
 *   @author Karl Gluck
 */
struct OtherPlayer
{
	// Animation information
	LPD3DXANIMATIONCONTROLLER pController;
	LPD3DXANIMATIONSET pWalkAnimation;
	LPD3DXANIMATIONSET pIdleAnimation;
	LPD3DXANIMATIONSET pRunAnimation;
	DWORD dwCurrentTrack;

	// Internal data
	BOOL bActive;
	D3DXVECTOR3 vRenderPos;
	FLOAT fRenderYaw;

	// Data updated by server messages
	D3DXVECTOR3 vOldPos, vNewPos;
	D3DXVECTOR3 vOldVel, vNewVel;
	FLOAT fOldTime, fNewTime;
	DWORD dwState;
	FLOAT fYaw;
};

/**
 * List of message IDs that are transacted with the server
 *   @author Karl Gluck
 */
enum Message
{
	MSG_LOGON,
	MSG_LOGOFF,
	MSG_UPDATEPLAYER,
	MSG_CONFIRMLOGON,
	MSG_PLAYERLOGGEDOFF,
};

/**
 * First structure in every message
 *   @author Karl Gluck
 */
struct MessageHeader
{
	Message MsgID;
};

/**
 * Message sent to let the server know we want to log on
 *   @author Karl Gluck
 */
struct LogOnMessage
{
	MessageHeader   Header;

	LogOnMessage() { Header.MsgID = MSG_LOGON; }
};

/**
 * Client wants to disconnect from the server
 *   @author Karl Gluck
 */
struct LogOffMessage
{
	MessageHeader   Header;

	LogOffMessage() { Header.MsgID = MSG_LOGOFF; }
};

/**
 * Sent between the server and client to update player information
 *   @author Karl Gluck
 */
struct UpdatePlayerMessage
{
	MessageHeader   Header;
	DWORD           dwPlayerID;
	FLOAT           fVelocity[3];       // Expressed in meters / second
	FLOAT           fPosition[3];
	DWORD           dwState;
	FLOAT           fYaw;

	UpdatePlayerMessage() { Header.MsgID = MSG_UPDATEPLAYER; }
};

/**
 * Sent by the server to tell the client that it has successfully logged on
 *   @author Karl Gluck
 */
struct ConfirmLogOnMessage
{
	MessageHeader   Header;

	ConfirmLogOnMessage() { Header.MsgID = MSG_CONFIRMLOGON; }
};

/**
 * Another player has logged off
 *   @author Karl Gluck
 */
struct PlayerLoggedOffMessage
{
	MessageHeader   Header;
	DWORD           dwPlayerID;

	PlayerLoggedOffMessage() { Header.MsgID = MSG_PLAYERLOGGEDOFF; }
};

/**
 * Loads basic mesh resource files.  This class could have much more complex functionality.  For
 * example, this class was originally created to allow for encrypted meshes to be loaded.
 *   @author Karl Gluck
 */
class BasicAllocateHierarchy : public AllocateHierarchy
{
public:

	/**
	 * Constructor to simply pass the number of blended matrices to the standard
	 * allocate hierarchy class
	 *   @param dwMaxBlendedMatrices Maximum number of hardware-supported matrix blends
	 */
	BasicAllocateHierarchy(DWORD dwMaxBlendedMatrices) :
		AllocateHierarchy(dwMaxBlendedMatrices)
	{
	}

private:

	/**
	 * Loads a texture resource from its file name.  This method is a simple implementation
	 * of the load texture command (more complex versions may decrypt resources, get textures
	 * from a cache, etc)
	 *   @param strFileName Source file name to load
	 *   @param ppTexture Destination for resultant texture interface
	 *   @return Result code
	 */
	STDMETHOD(LoadTexture)(THIS_ LPDIRECT3DDEVICE9 pDevice, LPCSTR strFileName,
		LPDIRECT3DTEXTURE9 * ppTexture)
	{
		return D3DXCreateTextureFromFile(pDevice, strFileName, ppTexture);
	}

	/**
	 * Loads a mesh hierarchy from an X file
	 *   @param pDevice Source device object to create the structure with
	 *   @param strFileName Name of the file to load
	 *   @param ppFrameHierarchy Target variable for frame hierarchy
	 *   @param ppAnimController Returns a pointer to the animation controller
	 *   @return Result code
	 */
	STDMETHOD(LoadMeshHierarchyFromX)(THIS_ LPDIRECT3DDEVICE9 pDevice, LPCSTR strFileName,
		LPD3DXFRAME* ppFrameHierarchy,
		LPD3DXANIMATIONCONTROLLER* ppAnimController)
	{
		return D3DXLoadMeshHierarchyFromX(strFileName, 0, pDevice, this, NULL, ppFrameHierarchy, ppAnimController);
	}
};

/**
 * Stores the information for the player client-side
 *   @author Karl Gluck
 */
struct Player
{
	AnimatedMesh mesh;
	LPD3DXANIMATIONCONTROLLER pController;
	LPD3DXANIMATIONSET pWalkAnimation;
	LPD3DXANIMATIONSET pIdleAnimation;
	LPD3DXANIMATIONSET pRunAnimation;
	D3DXVECTOR3 vPosition;
	FLOAT fCurrentPlayerYaw;
	FLOAT fTargetPlayerYaw;
	FLOAT fCameraZoom;
	FLOAT fCurrentCameraYaw;
	FLOAT fTargetCameraYaw;
	FLOAT fCurrentCameraHeight;
	FLOAT fTargetCameraHeight;
	FLOAT fVelocity;
	D3DXMATRIXA16 matPosition;
	FLOAT fOriginYaw;
	DWORD dwState;
	DWORD dwCurrentTrack;
};