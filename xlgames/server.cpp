/*
* visual studio 2022
* IOCP 에코 서버 
*/

#pragma once
#include "pch.h"
#include "ThreadManager.h"

#include <iostream>
using namespace std;

typedef enum
{
	IO_READ,
	IO_WRITE
}IO_OPRATION;

struct IO_DATA
{
	OVERLAPPED overlappped_;
	WSABUF wsaBuf_;
	IO_OPRATION ioType_;

	int totalBytes_;
	int currentBytes_;
	char buffer_[SOCKET_BUFSIZE];
};

struct SOCKET_DATA
{
	SOCKET socket_;
	SOCKADDR_IN addrInfo_;
	char ipAddress_[16];
	IO_DATA ioData_;
};

void AcceptThread(LPVOID context);
void WorkerThread(LPVOID context);

SOCKET listen_socket;

#define WORKER_THREAD_NUMBER (5)

int main()
{
	printf("Server Starting...\n");

	WSADATA wsa;
	ASSERT_CRASH(WSAStartup(MAKEWORD(2, 2), &wsa) == 0, "WSAStartup() error!");

	HANDLE iocp;
	//IOCP 생성
	iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, WORKER_THREAD_NUMBER);

	ASSERT_CRASH(iocp, "input output completion port is null");

	//Worker 스레드 시작
	ThreadManager* _ThreadManager = new ThreadManager();

	for (int i = 0; i < WORKER_THREAD_NUMBER; i++)
	{
		_ThreadManager->Launch([=]()
			{
				while (true)
				{
					WorkerThread(iocp);
				}
			});
	}

	listen_socket = WSASocket(AF_INET, SOCK_STREAM, NULL, NULL, NULL, WSA_FLAG_OVERLAPPED);

	ASSERT_CRASH(listen_socket != INVALID_SOCKET, "listen socket is null");

	SOCKADDR_IN serverAddr;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(PORT);
	serverAddr.sin_family = AF_INET;

	try
	{
		int socketError = ::bind(listen_socket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
		ASSERT_CRASH(socketError != SOCKET_ERROR, "bind error!");
		
		socketError = ::listen(listen_socket, 5);
		ASSERT_CRASH(socketError != SOCKET_ERROR, "listen error!");

		printf("server is listening...\n");
	}
	catch (char* msg)
	{
		printf("%s", msg);
	}

	//Accept 스레드 시작
	AcceptThread(iocp);

	::closesocket(listen_socket);
	WSACleanup();
	printf("End network base\n");
	return 0;
}

void CloseClient(SOCKET_DATA* socket)
{
	if (!socket)
		return;

	::closesocket(socket->socket_);

	delete socket;
}

void AcceptThread(LPVOID iocpHandler)
{
	HANDLE iocp = (HANDLE)iocpHandler;

	while (true)
	{
		SOCKET acceptSocket = INVALID_SOCKET;
		SOCKADDR_IN recvAddr;
		static int addrLen = sizeof(recvAddr);
		acceptSocket = WSAAccept(listen_socket, (struct sockaddr*)&recvAddr, &addrLen, NULL, 0);

		if (acceptSocket == SOCKET_ERROR)
		{
			printf("Accept fail!\n");
			continue;
		}

		getpeername(acceptSocket, (struct sockaddr*)&recvAddr, &addrLen);
		char clientAddr[64];
		inet_ntop(AF_INET, &(recvAddr.sin_addr), clientAddr, _countof(clientAddr));

		SOCKET_DATA* session = new SOCKET_DATA;

		if (session == NULL)
		{
			printf("accept alloc fail!\n");
			break;
		}

		ZeroMemory(session, sizeof(SOCKET_DATA));
		session->socket_ = acceptSocket;
		strcpy_s(session->ipAddress_, clientAddr);

		session->ioData_.ioType_ = IO_READ;
		session->ioData_.totalBytes_ = sizeof(session->ioData_.buffer_);
		session->ioData_.currentBytes_ = 0;
		session->ioData_.wsaBuf_.buf = session->ioData_.buffer_;
		session->ioData_.wsaBuf_.len = sizeof(session->ioData_.buffer_);

		iocp = CreateIoCompletionPort((HANDLE)acceptSocket, iocp, (ULONG_PTR)session, NULL);

		if (!iocp)
		{
			::closesocket(acceptSocket);
			return;
		}

		session->socket_ = acceptSocket;
		IO_DATA ioData = session->ioData_;
		DWORD flags = 0;
		DWORD recvBytes;
		DWORD errorCode = WSARecv(session->socket_, &(ioData.wsaBuf_), 1, &recvBytes, &flags, &(ioData.overlappped_), NULL);

		if (errorCode == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			CloseClient(session);
		}
	}
	return;
}

void WorkerThread(LPVOID iocpHandler)
{
	HANDLE iocp = (HANDLE)iocpHandler;

	while (true)
	{
		IO_DATA* ioData = NULL;
		SOCKET_DATA* session = NULL;
		DWORD trafficSize;
		BOOL succes = GetQueuedCompletionStatus(iocp, &trafficSize,
			OUT reinterpret_cast<PULONG_PTR>(&session), (LPOVERLAPPED*)&ioData, INFINITE);

		if (!succes)
		{
			printf("queue data getting fail\n");
			continue;
		}

		if (session == NULL)
		{
			printf("socket data broken!\n");
			return;
		}

		if (trafficSize == 0)
		{
			CloseClient(session);
			continue;
		}
		ioData->currentBytes_ += trafficSize;
		DWORD flags = 0;

		switch (ioData->ioType_)
		{
		case IO_WRITE:
			ioData->wsaBuf_.buf[trafficSize] = '\0';
			printf("@ send message : %s\n", ioData->wsaBuf_.buf);
			break;

		case IO_READ:
		{
			ioData->ioType_ = IO_WRITE;
			ioData->wsaBuf_.len = trafficSize;
			flags = 0;
			DWORD sendBytes;
			DWORD errorCode = WSASend(session->socket_, &(ioData->wsaBuf_), 1, &sendBytes, flags, &(ioData->overlappped_), NULL);

			if (errorCode == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
			{
				CloseClient(session);
				printf("error code! = %d\n", errorCode);
			}

			ioData->wsaBuf_.buf[trafficSize] = '\0';
			printf("@recv client message : % s\n", ioData->wsaBuf_.buf);

			ioData->wsaBuf_.len = SOCKET_BUFSIZE;

			ZeroMemory(ioData->buffer_, sizeof(ioData->buffer_));
			{
				ioData->ioType_ = IO_READ;
				DWORD recvBytes;
				DWORD errorCode = WSARecv(session->socket_, &(ioData->wsaBuf_), 1, &recvBytes, &flags, &(ioData->overlappped_), NULL);

				if (errorCode == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
				{
					CloseClient(session);
				}
			}
		}
		break;
		}
	}
	return;
}