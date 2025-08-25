#define WIN32_LEAN_AND_MEAN

#include "includes.h"
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "Client.h"


// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


//#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "2069"

DetailStruct csend = { 0,0,0,0,0,0 };

void clientsend(DetailStruct dets)
{
    csend = dets;
}

int runClient(const char* serverName)
{
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL,
        * ptr = NULL,
        hints;
    int iResult, iSendResult;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // was AF_UNSPEC
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    std::string port = "";
    std::cout << "What port you want ahah hah: ";
    std::cin >> port;

    // Resolve the server address and port
    do {
        iResult = getaddrinfo(serverName, port.c_str(), &hints, &result);
        if (iResult != 0) {
            printf("getaddrinfo failed with error: %d\n", iResult);
            printf("trying again in 1 second\n");
            Sleep(1000);
        }
    } while (iResult != 0);

    printf("got addr info\n");

    // Attempt to connect to an address until one succeeds

    // old reconnect spam loop
    do {
        printf("trying to connect\n");
        for (ptr = result; ptr != NULL;ptr = ptr->ai_next) {

            // Create a SOCKET for connecting to server
            ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
                ptr->ai_protocol);
            if (ConnectSocket == INVALID_SOCKET) {
                printf("socket failed with error: %ld\n", WSAGetLastError());
                WSACleanup();
                return 1;
            }

            printf("created connect socket\n");

            // Connect to server.
            iResult = connect( ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
            if (iResult == SOCKET_ERROR) {
                printf("socket returned connect error %ld, trying again\n", WSAGetLastError());
                closesocket(ConnectSocket);
                ConnectSocket = INVALID_SOCKET;
                //Sleep(2000);
                continue;
            }
            printf("connectsocket value = %p",ConnectSocket);
            break;
        }
    } while (ConnectSocket == INVALID_SOCKET);

    freeaddrinfo(result);

    // this means it made it out of the connect loop above without a valid socket
    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    // We will not be sending an initial buffer
    // Send & Receive until the peer closes the connection
    do {

        iSendResult = send(ConnectSocket, reinterpret_cast<char*>(&csend), sizeof(csend), 0);
        if (iSendResult == SOCKET_ERROR) {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(ConnectSocket);
            WSACleanup();
            return 1;
        }

        //printf("Bytes Sent: %ld\n", iResult);
        
        DetailStruct crecv;
        iResult = recv(ConnectSocket, reinterpret_cast<char*>(&crecv), sizeof(crecv), 0);
        if (iResult > 0)
        {
            //printf("Bytes received: %d\n", iResult);
            setAltDets(crecv);
        }
        else if (iResult == 0)
            printf("Connection closed\n");
        else
            printf("recv failed with error: %d\n", WSAGetLastError());

    } while (iResult > 0);


    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}