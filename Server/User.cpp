#include "User.h"

using namespace Server;

User::User()
{
}

User::~User()
{
}

HRESULT User::Create(DWORD dwId, WORD wPort)
{
	return E_NOTIMPL;
}

VOID User::Destroy()
{
	return VOID();
}

DWORD User::GetId()
{
	return 0;
}

HRESULT User::Connect(const SOCKADDR_IN * pAddress, LPTHREAD_START_ROUTINE lpProcAddress)
{
	return E_NOTIMPL;
}

VOID User::Disconnect()
{
	return VOID();
}

BOOL User::IsConnected()
{
	return 0;
}

VOID User::ThreadFinished()
{
	return VOID();
}

EndWaitResult User::WaitForPackets(DWORD dwTimeout)
{
	return EndWaitResult();
}

int User::SendPacket(const CHAR * pBuffer, int length)
{
	return 0;
}

int User::RecvPacket(char * pBuffer, int length)
{
	return 0;
}
