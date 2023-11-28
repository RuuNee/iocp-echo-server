#include "pch.h"
#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

int main()
{
    Sleep(1000);
    WSADATA wsaData;

    ASSERT_CRASH(WSAStartup(MAKEWORD(2, 2), &wsaData) == 0, "WSAStartup() error!");

    SOCKET hSocket = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED); 
    ASSERT_CRASH(hSocket != INVALID_SOCKET, "socket() error");


    SOCKADDR_IN recvAddr;
    memset(&recvAddr, 0, sizeof(recvAddr));
    recvAddr.sin_family = AF_INET;
    recvAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    recvAddr.sin_port = htons(3598);

    ASSERT_CRASH(connect(hSocket, (SOCKADDR*)&recvAddr, sizeof(recvAddr)) != SOCKET_ERROR, "connect() error!");

    WSAEVENT event = WSACreateEvent();

    WSAOVERLAPPED overlapped;
    memset(&overlapped, 0, sizeof(overlapped));

    overlapped.hEvent = event;

    WSABUF dataBuf;
    char message[1024] = { 0, };
    int sendBytes = 0;
    int recvBytes = 0;
    int flags = 0;

    while (true)
    {
        flags = 0;
        printf("전송할 데이터(종료 : exit)\n");

        scanf("%s", message);

        if (!strcmp(message, "exit")) break;

        dataBuf.len = strlen(message);
        dataBuf.buf = message;

        if (WSASend(hSocket, &dataBuf, 1, (LPDWORD)&sendBytes, 0, &overlapped, NULL) == SOCKET_ERROR)
            ASSERT_CRASH(WSAGetLastError() == WSA_IO_PENDING, "WSASend() error");

        WSAWaitForMultipleEvents(1, &event, TRUE, WSA_INFINITE, FALSE);

        WSAGetOverlappedResult(hSocket, &overlapped, (LPDWORD)&sendBytes, FALSE, NULL);

        printf("전송된바이트수: %d \n", sendBytes);

        if (WSARecv(hSocket, &dataBuf, 1, (LPDWORD)&recvBytes, (LPDWORD)&flags, &overlapped, NULL) == SOCKET_ERROR)
            ASSERT_CRASH(WSAGetLastError() != WSA_IO_PENDING, "WSASend() error");

        printf("Recv[%s]\n", dataBuf.buf);
    }

    closesocket(hSocket);

    WSACleanup();

    return 0;
}
