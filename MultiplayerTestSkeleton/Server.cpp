#include "includes.h"
#include "Server.h"
#pragma comment(lib, "Ws2_32.lib")
#define DEFAULT_PORT "2069"
#define DEFAULT_BUFLEN 512

DetailStruct ssend = { 0,0,0,0,0,0 };

void serversend(DetailStruct dets)
{
	ssend = dets;
}

int runServer()
{
	WSADATA wsaData;
	int iResult, iSendResult;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	struct addrinfo* result = NULL;
	struct addrinfo hints;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE; // means will be used in a bind


	// Resolve the local address and port to be used by the server
	
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket

	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	printf("made it to listen\n");

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	printf("started listening\n");


	// Accept a client socket
	sockaddr caddr;
	ClientSocket = accept(ListenSocket, &caddr, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		printf("accept failed: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	printf("accepted socket\n");
	//printf(caddr.sa_data);

	// No longer need server socket - if we want to keep listening for more connections, will have to change this in future
	closesocket(ListenSocket);

	printf("made it to do loop\n");

	// Receive until the peer shuts down the connection
	do {
		DetailStruct srecv;
		iResult = recv(ClientSocket, reinterpret_cast<char*>(&srecv), sizeof(srecv), 0);

		if (iResult > 0) {
			printf("Bytes received: %d\n", iResult);
			setAltDets(srecv); // should set the x y z of the alt char to be that of the received

			// Send XYZ of our player back
			iSendResult = send(ClientSocket, reinterpret_cast<char*>(&ssend), sizeof(ssend), 0);

			if (iSendResult == SOCKET_ERROR) {
				printf("send failed: %d\n", WSAGetLastError());
				closesocket(ClientSocket);
				WSACleanup();
				return 1;
			}
			printf("Bytes sent: %d\n", iSendResult);
		}
		else if (iResult == 0)
			printf("Connection closing...\n");
		else {
			printf("recv failed: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			return 1;
		}

	} while (iResult > 0);

	// shutdown the connection since we're done
	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(ClientSocket);
	WSACleanup();

	return 0;
}