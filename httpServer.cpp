// httpServer.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#define _CRT_SECURE_NO_WARNINGS
#pragma warning (disable: 4996)

/********************************************************************/
//select模型
/********************************************************************/
/*
#include <winsock2.h>  
#include <iostream>
#include <vector>
#include <map>
#include <string>

#define _CRT_SECURE_NO_WARNINGS
#pragma warning (disable: 4996)
#pragma comment(lib,"WS2_32.lib")  
using namespace std;

#include<WS2tcpip.h>

#define PORT       5150
#define MSGSIZE    1024

int    g_iTotalConn = 0;
SOCKET g_CliSocketArr[FD_SETSIZE];

DWORD WINAPI WorkerThread(LPVOID lpParameter);

CONST INT RECV_SIZE = 8192;

#pragma comment(lib,"libssl.lib")
#pragma comment(lib,"libcrypto.lib")

int main(int argc, _TCHAR* argv[])
{

	WSADATA     wsaData;
	SOCKET      sListen, sClient;
	SOCKADDR_IN local, client;
	int         iaddrSize = sizeof(SOCKADDR_IN);
	DWORD       dwThreadId;

	// Initialize Windows socket library
	WSAStartup(0x0202, &wsaData);

	// Create listening socket
	sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Bind
	local.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	local.sin_family = AF_INET;
	local.sin_port = htons(PORT);
	bind(sListen, (struct sockaddr *)&local, sizeof(SOCKADDR_IN));

	// Listen
	listen(sListen, 3);

	// Create worker thread
	CreateThread(NULL, 0, WorkerThread, NULL, 0, &dwThreadId);

	while (TRUE)
	{
		// Accept a connection
		sClient = accept(sListen, (struct sockaddr *)&client, &iaddrSize);
		printf("Accepted client:%s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

		// Add socket to g_CliSocketArr
		g_CliSocketArr[g_iTotalConn++] = sClient;
	}

	return 0;
}

DWORD WINAPI WorkerThread(LPVOID lpParam)
{
	int            i;
	fd_set         fdread;
	int            ret;
	struct timeval tv = { 1, 0 };
	char           szMessage[MSGSIZE];

	while (TRUE)
	{
		FD_ZERO(&fdread);
		for (i = 0; i < g_iTotalConn; i++)
		{
			FD_SET(g_CliSocketArr[i], &fdread);
		}

		// We only care read event
		ret = select(0, &fdread, NULL, NULL, &tv);

		if (ret == 0)
		{
			// Time expired
			continue;
		}

		for (i = 0; i < g_iTotalConn; i++)
		{
			if (FD_ISSET(g_CliSocketArr[i], &fdread))
			{
				// A read event happened on g_CliSocketArr
				ret = recv(g_CliSocketArr[i], szMessage, MSGSIZE, 0);
				if (ret == 0 || (ret == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET))
				{
					// Client socket closed
					printf("Client socket %d closed.\n", g_CliSocketArr[i]);
					closesocket(g_CliSocketArr[i]);
					if (i < g_iTotalConn - 1)
					{
						g_CliSocketArr[i--] = g_CliSocketArr[--g_iTotalConn];
					}
				}
				else
				{
					// We received a message from client
					szMessage[ret] = '\0';
					send(g_CliSocketArr[i], szMessage, strlen(szMessage), 0);
				}
			}
		}
	}

	return 0;
}
*/

/////////////////////////////////////////////////////////////////
//事件选择
////////////////////////////////////////////////////////////////

/*
#include <winsock2.h>
#include <stdio.h>

#define PORT    5150
#define MSGSIZE 1024

#pragma comment(lib, "ws2_32.lib")

int      g_iTotalConn = 0;
SOCKET   g_CliSocketArr[MAXIMUM_WAIT_OBJECTS];
WSAEVENT g_CliEventArr[MAXIMUM_WAIT_OBJECTS];

DWORD WINAPI WorkerThread(LPVOID);
void Cleanup(int index);

int main()
{
	WSADATA     wsaData;
	SOCKET      sListen, sClient;
	SOCKADDR_IN local, client;
	DWORD       dwThreadId;
	int         iaddrSize = sizeof(SOCKADDR_IN);

	// Initialize Windows Socket library
	WSAStartup(0x0202, &wsaData);

	// Create listening socket
	sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Bind
	local.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	local.sin_family = AF_INET;
	local.sin_port = htons(PORT);
	bind(sListen, (struct sockaddr *)&local, sizeof(SOCKADDR_IN));

	// Listen
	listen(sListen, 3);

	// Create worker thread
	CreateThread(NULL, 0, WorkerThread, NULL, 0, &dwThreadId);

	while (TRUE)
	{
		// Accept a connection
		sClient = accept(sListen, (struct sockaddr *)&client, &iaddrSize);
		printf("Accepted client:%s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

		// Associate socket with network event
		g_CliSocketArr[g_iTotalConn] = sClient;
		g_CliEventArr[g_iTotalConn] = WSACreateEvent();
		WSAEventSelect(g_CliSocketArr[g_iTotalConn],
			g_CliEventArr[g_iTotalConn],
			FD_READ | FD_CLOSE);
		g_iTotalConn++;
	}
}

DWORD WINAPI WorkerThread(LPVOID lpParam)
{
	int              ret, index;
	WSANETWORKEVENTS NetworkEvents;
	char             szMessage[MSGSIZE];

	while (TRUE)
	{
		ret = WSAWaitForMultipleEvents(g_iTotalConn, g_CliEventArr, FALSE, 1000, FALSE);
		if (ret == WSA_WAIT_FAILED || ret == WSA_WAIT_TIMEOUT)
		{
			continue;
		}

		index = ret - WSA_WAIT_EVENT_0;
		WSAEnumNetworkEvents(g_CliSocketArr[index], g_CliEventArr[index], &NetworkEvents);

		if (NetworkEvents.lNetworkEvents & FD_READ)
		{
			// Receive message from client
			ret = recv(g_CliSocketArr[index], szMessage, MSGSIZE, 0);
			if (ret == 0 || (ret == SOCKET_ERROR && WSAGetLastError() == WSAECONNRESET))
			{
				Cleanup(index);
			}
			else
			{
				szMessage[ret] = '\0';
				send(g_CliSocketArr[index], szMessage, strlen(szMessage), 0);
			}
		}

		if (NetworkEvents.lNetworkEvents & FD_CLOSE)
		{
			Cleanup(index);
		}
	}
	return 0;
}

void Cleanup(int index)
{
	closesocket(g_CliSocketArr[index]);
	WSACloseEvent(g_CliEventArr[index]);

	if (index < g_iTotalConn - 1)
	{
		g_CliSocketArr[index] = g_CliSocketArr[g_iTotalConn - 1];
		g_CliEventArr[index] = g_CliEventArr[g_iTotalConn - 1];
	}

	g_iTotalConn--;
}
*/

/////////////////////////////////////////////////////////////////////
//事件通知的方式实现重叠io
////////////////////////////////////////////////////////////////////
/*
#include <winsock2.h>
#include <stdio.h>

#define PORT    5150
#define MSGSIZE 1024

#pragma comment(lib, "ws2_32.lib")

typedef struct
{
	WSAOVERLAPPED overlap;
	WSABUF        Buffer;
	char          szMessage[MSGSIZE];
	DWORD         NumberOfBytesRecvd;
	DWORD         Flags;
}PER_IO_OPERATION_DATA, *LPPER_IO_OPERATION_DATA;

int                     g_iTotalConn = 0;
SOCKET                  g_CliSocketArr[MAXIMUM_WAIT_OBJECTS];
WSAEVENT                g_CliEventArr[MAXIMUM_WAIT_OBJECTS];
LPPER_IO_OPERATION_DATA g_pPerIODataArr[MAXIMUM_WAIT_OBJECTS];

DWORD WINAPI WorkerThread(LPVOID);
void Cleanup(int);

int main()
{
	WSADATA     wsaData;
	SOCKET      sListen, sClient;
	SOCKADDR_IN local, client;
	DWORD       dwThreadId;
	int         iaddrSize = sizeof(SOCKADDR_IN);

	// Initialize Windows Socket library
	WSAStartup(0x0202, &wsaData);

	// Create listening socket
	sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Bind
	local.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	local.sin_family = AF_INET;
	local.sin_port = htons(PORT);
	bind(sListen, (struct sockaddr *)&local, sizeof(SOCKADDR_IN));

	// Listen
	listen(sListen, 3);

	// Create worker thread
	CreateThread(NULL, 0, WorkerThread, NULL, 0, &dwThreadId);

	while (TRUE)
	{
		// Accept a connection
		sClient = accept(sListen, (struct sockaddr *)&client, &iaddrSize);
		printf("Accepted client:%s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

		g_CliSocketArr[g_iTotalConn] = sClient;

		// Allocate a PER_IO_OPERATION_DATA structure
		g_pPerIODataArr[g_iTotalConn] = (LPPER_IO_OPERATION_DATA)HeapAlloc(
			GetProcessHeap(),
			HEAP_ZERO_MEMORY,
			sizeof(PER_IO_OPERATION_DATA));
		g_pPerIODataArr[g_iTotalConn]->Buffer.len = MSGSIZE;
		g_pPerIODataArr[g_iTotalConn]->Buffer.buf = g_pPerIODataArr[g_iTotalConn]->szMessage;
		g_CliEventArr[g_iTotalConn] = g_pPerIODataArr[g_iTotalConn]->overlap.hEvent = WSACreateEvent();

		// Launch an asynchronous operation
		WSARecv(
			g_CliSocketArr[g_iTotalConn],
			&g_pPerIODataArr[g_iTotalConn]->Buffer,
			1,
			&g_pPerIODataArr[g_iTotalConn]->NumberOfBytesRecvd,
			&g_pPerIODataArr[g_iTotalConn]->Flags,
			&g_pPerIODataArr[g_iTotalConn]->overlap,
			NULL);

		g_iTotalConn++;
	}

	closesocket(sListen);
	WSACleanup();
	return 0;
}

DWORD WINAPI WorkerThread(LPVOID lpParam)
{
	int   ret, index;
	DWORD cbTransferred;

	while (TRUE)
	{
		ret = WSAWaitForMultipleEvents(g_iTotalConn, g_CliEventArr, FALSE, 1000, FALSE);
		if (ret == WSA_WAIT_FAILED || ret == WSA_WAIT_TIMEOUT)
		{
			continue;
		}

		index = ret - WSA_WAIT_EVENT_0;
		WSAResetEvent(g_CliEventArr[index]);

		WSAGetOverlappedResult(
			g_CliSocketArr[index],
			&g_pPerIODataArr[index]->overlap,
			&cbTransferred,
			TRUE,
			&g_pPerIODataArr[g_iTotalConn]->Flags);

		if (cbTransferred == 0)
		{
			// The connection was closed by client
			Cleanup(index);
		}
		else
		{
			// g_pPerIODataArr[index]->szMessage contains the received data
			g_pPerIODataArr[index]->szMessage[cbTransferred] = '\0';
			send(g_CliSocketArr[index], g_pPerIODataArr[index]->szMessage,
				cbTransferred, 0);

			// Launch another asynchronous operation
			WSARecv(
				g_CliSocketArr[index],
				&g_pPerIODataArr[index]->Buffer,
				1,
				&g_pPerIODataArr[index]->NumberOfBytesRecvd,
				&g_pPerIODataArr[index]->Flags,
				&g_pPerIODataArr[index]->overlap,
				NULL);
		}
	}

	return 0;
}

void Cleanup(int index)
{
	closesocket(g_CliSocketArr[index]);
	WSACloseEvent(g_CliEventArr[index]);
	HeapFree(GetProcessHeap(), 0, g_pPerIODataArr[index]);

	if (index < g_iTotalConn - 1)
	{
		g_CliSocketArr[index] = g_CliSocketArr[g_iTotalConn - 1];
		g_CliEventArr[index] = g_CliEventArr[g_iTotalConn - 1];
		g_pPerIODataArr[index] = g_pPerIODataArr[g_iTotalConn - 1];
	}

	g_pPerIODataArr[--g_iTotalConn] = NULL;
}
*/
/////////////////////////////////////////////////////////////////////
//完成例程的方式实现重叠io
////////////////////////////////////////////////////////////////////
/*
#include <WINSOCK2.H>
#include <stdio.h>

#define PORT    5150
#define MSGSIZE 1024

#pragma comment(lib, "ws2_32.lib")

typedef struct
{
	WSAOVERLAPPED overlap;
	WSABUF        Buffer;
	char          szMessage[MSGSIZE];
	DWORD         NumberOfBytesRecvd;
	DWORD         Flags;
	SOCKET        sClient;
}PER_IO_OPERATION_DATA, *LPPER_IO_OPERATION_DATA;

DWORD WINAPI WorkerThread(LPVOID);
void CALLBACK CompletionROUTINE(DWORD, DWORD, LPWSAOVERLAPPED, DWORD);

SOCKET g_sNewClientConnection;
BOOL   g_bNewConnectionArrived = FALSE;

int main()
{
	WSADATA     wsaData;
	SOCKET      sListen;
	SOCKADDR_IN local, client;
	DWORD       dwThreadId;
	int         iaddrSize = sizeof(SOCKADDR_IN);

	// Initialize Windows Socket library
	WSAStartup(0x0202, &wsaData);

	// Create listening socket
	sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Bind
	local.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	local.sin_family = AF_INET;
	local.sin_port = htons(PORT);
	bind(sListen, (struct sockaddr *)&local, sizeof(SOCKADDR_IN));

	// Listen
	listen(sListen, 3);

	// Create worker thread
	CreateThread(NULL, 0, WorkerThread, NULL, 0, &dwThreadId);

	while (TRUE)
	{
		// Accept a connection
		g_sNewClientConnection = accept(sListen, (struct sockaddr *)&client, &iaddrSize);
		g_bNewConnectionArrived = TRUE;
		printf("Accepted client:%s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
	}
}

DWORD WINAPI WorkerThread(LPVOID lpParam)
{
	LPPER_IO_OPERATION_DATA lpPerIOData = NULL;

	while (TRUE)
	{
		if (g_bNewConnectionArrived)
		{
			// Launch an asynchronous operation for new arrived connection
			lpPerIOData = (LPPER_IO_OPERATION_DATA)HeapAlloc(
				GetProcessHeap(),
				HEAP_ZERO_MEMORY,
				sizeof(PER_IO_OPERATION_DATA));
			lpPerIOData->Buffer.len = MSGSIZE;
			lpPerIOData->Buffer.buf = lpPerIOData->szMessage;
			lpPerIOData->sClient = g_sNewClientConnection;

			WSARecv(lpPerIOData->sClient,
				&lpPerIOData->Buffer,
				1,
				&lpPerIOData->NumberOfBytesRecvd,
				&lpPerIOData->Flags,
				&lpPerIOData->overlap,
				CompletionROUTINE);

			g_bNewConnectionArrived = FALSE;
		}

		SleepEx(1000, TRUE);
	}
	return 0;
}

void CALLBACK CompletionROUTINE(DWORD dwError,
	DWORD cbTransferred,
	LPWSAOVERLAPPED lpOverlapped,
	DWORD dwFlags)
{
	LPPER_IO_OPERATION_DATA lpPerIOData = (LPPER_IO_OPERATION_DATA)lpOverlapped;

	if (dwError != 0 || cbTransferred == 0)
	{
		// Connection was closed by client
		closesocket(lpPerIOData->sClient);
		HeapFree(GetProcessHeap(), 0, lpPerIOData);
	}
	else
	{
		lpPerIOData->szMessage[cbTransferred] = '\0';
		send(lpPerIOData->sClient, lpPerIOData->szMessage, cbTransferred, 0);

		// Launch another asynchronous operation
		memset(&lpPerIOData->overlap, 0, sizeof(WSAOVERLAPPED));
		lpPerIOData->Buffer.len = MSGSIZE;
		lpPerIOData->Buffer.buf = lpPerIOData->szMessage;

		WSARecv(lpPerIOData->sClient,
			&lpPerIOData->Buffer,
			1,
			&lpPerIOData->NumberOfBytesRecvd,
			&lpPerIOData->Flags,
			&lpPerIOData->overlap,
			CompletionROUTINE);
	}
}
*/

/****************************************************************/
//io完成端口
/**/
#include <WINSOCK2.H>
#include <stdio.h>

#define PORT    5150
#define MSGSIZE 1024

#pragma comment(lib, "ws2_32.lib")

typedef enum
{
	RECV_POSTED
}OPERATION_TYPE;

typedef struct
{
	WSAOVERLAPPED  overlap;
	WSABUF         Buffer;
	char           szMessage[MSGSIZE];
	DWORD          NumberOfBytesRecvd;
	DWORD          Flags;
	OPERATION_TYPE OperationType;
}PER_IO_OPERATION_DATA, *LPPER_IO_OPERATION_DATA;

DWORD WINAPI WorkerThread(LPVOID);

int main()
{
	WSADATA                 wsaData;
	SOCKET                  sListen, sClient;
	SOCKADDR_IN             local, client;
	DWORD                   i, dwThreadId;
	int                     iaddrSize = sizeof(SOCKADDR_IN);
	HANDLE                  CompletionPort = INVALID_HANDLE_VALUE;
	SYSTEM_INFO             systeminfo;
	LPPER_IO_OPERATION_DATA lpPerIOData = NULL;

	// Initialize Windows Socket library
	WSAStartup(0x0202, &wsaData);

	// Create completion port
	CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

	// Create worker thread
	GetSystemInfo(&systeminfo);
	for (i = 0; i < systeminfo.dwNumberOfProcessors; i++)
	{
		CreateThread(NULL, 0, WorkerThread, CompletionPort, 0, &dwThreadId);
	}

	// Create listening socket
	sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Bind
	local.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	local.sin_family = AF_INET;
	local.sin_port = htons(PORT);
	bind(sListen, (struct sockaddr *)&local, sizeof(SOCKADDR_IN));

	// Listen
	listen(sListen, 3);

	while (TRUE)
	{
		// Accept a connection
		sClient = accept(sListen, (struct sockaddr *)&client, &iaddrSize);
		printf("Accepted client:%s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

		// Associate the newly arrived client socket with completion port
		CreateIoCompletionPort((HANDLE)sClient, CompletionPort, (DWORD)sClient, 0);

		// Launch an asynchronous operation for new arrived connection
		lpPerIOData = (LPPER_IO_OPERATION_DATA)HeapAlloc(
			GetProcessHeap(),
			HEAP_ZERO_MEMORY,
			sizeof(PER_IO_OPERATION_DATA));
		lpPerIOData->Buffer.len = MSGSIZE;
		lpPerIOData->Buffer.buf = lpPerIOData->szMessage;
		lpPerIOData->OperationType = RECV_POSTED;
		WSARecv(sClient,
			&lpPerIOData->Buffer,
			1,
			&lpPerIOData->NumberOfBytesRecvd,
			&lpPerIOData->Flags,
			&lpPerIOData->overlap,
			NULL);
	}

	PostQueuedCompletionStatus(CompletionPort, 0xFFFFFFFF, 0, NULL);
	CloseHandle(CompletionPort);
	closesocket(sListen);
	WSACleanup();
	return 0;
}

DWORD WINAPI WorkerThread(LPVOID CompletionPortID)
{
	HANDLE                  CompletionPort = (HANDLE)CompletionPortID;
	DWORD                   dwBytesTransferred;
	SOCKET                  sClient;
	LPPER_IO_OPERATION_DATA lpPerIOData = NULL;

	while (TRUE)
	{
		GetQueuedCompletionStatus(
			CompletionPort,
			&dwBytesTransferred,
			(PULONG_PTR)&sClient,
			(LPOVERLAPPED *)&lpPerIOData,
			INFINITE);
		if (dwBytesTransferred == 0xFFFFFFFF)
		{
			return 0;
		}

		if (lpPerIOData->OperationType == RECV_POSTED)
		{
			if (dwBytesTransferred == 0)
			{
				// Connection was closed by client
				closesocket(sClient);
				HeapFree(GetProcessHeap(), 0, lpPerIOData);
			}
			else
			{
				lpPerIOData->szMessage[dwBytesTransferred] = '\0';
				send(sClient, lpPerIOData->szMessage, dwBytesTransferred, 0);

				// Launch another asynchronous operation for sClient
				memset(lpPerIOData, 0, sizeof(PER_IO_OPERATION_DATA));
				lpPerIOData->Buffer.len = MSGSIZE;
				lpPerIOData->Buffer.buf = lpPerIOData->szMessage;
				lpPerIOData->OperationType = RECV_POSTED;
				WSARecv(sClient,
					&lpPerIOData->Buffer,
					1,
					&lpPerIOData->NumberOfBytesRecvd,
					&lpPerIOData->Flags,
					&lpPerIOData->overlap,
					NULL);
			}
		}
	}
	return 0;
}