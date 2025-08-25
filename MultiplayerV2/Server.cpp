#include "includes.h"

#undef UNICODE

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_PORT "2069"

HANDLE s_handle;
DWORD s_altmechptr;


int runServer()
{
    WSADATA wsaData;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo* result = NULL;
    struct addrinfo hints;

    int iResult, iSendResult;

    // all hex addresses are dwords - unsigned 4 byte chunks
    DWORD mainPtr, mechPtr, altMechPtr, altPtr, pID, baseModule, rotPtr;
    int anim1, anim2;
    float mainX, mainY, mainZ, mainVelX, mainVelY, mainVelZ, rot;
    HANDLE handle;

    GameStateDetails senddets, recvdets;

    // initialize process read/write
    // beware of ghost instances of legomarvel which will return wrong pID
    pID = GetProcessID(L"LEGOMARVEL.exe");
    baseModule = GetModuleBaseAddress(pID, L"LEGOMARVEL.exe");
    handle = OpenProcess(PROCESS_ALL_ACCESS, NULL, pID);

    // read pointer locations, will change when game restarted but server should not run without game anyway
    // nvm these also change when switching characters you goofy sloober
    // but I'm leaving this as is for now
    ReadProcessMemory(handle, (LPCVOID)(baseModule + 0x15B0884), &mainPtr, sizeof(mainPtr), nullptr);
    ReadProcessMemory(handle, (LPCVOID)(baseModule + 0x1599B98), &mechPtr, sizeof(mechPtr), nullptr);
    ReadProcessMemory(handle, (LPCVOID)(baseModule + 0x1599B9C), &altMechPtr, sizeof(altMechPtr), nullptr);
    ReadProcessMemory(handle, (LPCVOID)(altMechPtr + 0x184), &altPtr, sizeof(altPtr), nullptr);

    // remove char2 brain or just set the no follow flag
    // OR set controlled by player flag
    // controlled by player flag at mech1+25D
    BYTE zerobyte = 1;
    WriteProcessMemory(handle, (LPVOID)(altMechPtr + 0x25D), &zerobyte, sizeof(zerobyte), nullptr);
    s_handle = handle;
    s_altmechptr = altMechPtr;


    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for the server to listen for client connections.
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
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

    freeaddrinfo(result);

    printf("listening\n");

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // No longer need server socket
    closesocket(ListenSocket);

    printf("client connected!");


    // initialize delta clock
    clock_t deltaTime;
    deltaTime = clock();
    int loopIteration = 0;

    // Receive until the peer shuts down the connection
    do {

        // read game details
        //std::thread readThread(readDets, &mainX, &mainY, &mainZ, &mainVelX, &mainVelY, &mainVelZ, handle, mainPtr);
        //readThread.detach();
        //printf("read x: %f\n", mainX);
        
        // 1/0.008 = 125 fps
        deltaTime = clock() - deltaTime;
        int waitTime = 0;
        if (deltaTime < 8)
        {
            waitTime = 8 - int(deltaTime);
            Sleep(waitTime);
        }
        printf("delta (anything less than 8ms is coo): %d\n", int(deltaTime));
        printf("Frame per second %d\n", 1000/(waitTime+int(deltaTime)));
        deltaTime = clock();

        // check fps every 50 or so frames
        /*loopIteration += 1;
        if (loopIteration == 50)
        {
            deltaTime = clock() - deltaTime;
            //printf("time since last frame: %f\n", (float)deltaTime);
            float fps = (loopIteration * 1000) / (float)deltaTime;
            printf("fps: %f\n", fps);
            deltaTime = clock();
            loopIteration = 0;
        }*/


        ReadProcessMemory(handle, (LPCVOID)(mainPtr + 0x70), &mainX, sizeof(mainX), nullptr);
        ReadProcessMemory(handle, (LPCVOID)(mainPtr + 0x74), &mainY, sizeof(mainY), nullptr);
        ReadProcessMemory(handle, (LPCVOID)(mainPtr + 0x78), &mainZ, sizeof(mainZ), nullptr);
        ReadProcessMemory(handle, (LPCVOID)(mainPtr + 0xc0), &mainVelX, sizeof(mainVelX), nullptr);
        ReadProcessMemory(handle, (LPCVOID)(mainPtr + 0xc4), &mainVelY, sizeof(mainVelY), nullptr);
        ReadProcessMemory(handle, (LPCVOID)(mainPtr + 0xc8), &mainVelZ, sizeof(mainVelZ), nullptr);
        ReadProcessMemory(handle, (LPCVOID)(mechPtr + 0x41C), &anim1, sizeof(anim1), nullptr);
        ReadProcessMemory(handle, (LPCVOID)(mechPtr + 0x430), &anim2, sizeof(anim2), nullptr);
        ReadProcessMemory(handle, (LPCVOID)(mechPtr + 0x170), &rot, sizeof(rot), nullptr);

        // Receive game details
        iResult = recv(ClientSocket, reinterpret_cast<char*>(&recvdets), sizeof(recvdets), 0);
        if (iResult > 0) {
            //printf("Bytes received: %d\n", iResult);

            // Send game details upon receiving game details
            senddets = { mainX,mainY,mainZ,mainVelX,mainVelY, mainVelZ, anim1, anim2, rot };
            iSendResult = send(ClientSocket, reinterpret_cast<char*>(&senddets), sizeof(senddets), 0);
            if (iSendResult == SOCKET_ERROR) {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(ClientSocket);
                WSACleanup();
                return 1;
            }
            //printf("Bytes sent: %d\n", iSendResult);
        }
        else if (iResult == 0)
            printf("Connection closing...\n");
        else {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
            return 1;
        }

        // write game details
        std::thread writeThread(writeDets, recvdets, handle, altPtr, altMechPtr);
        writeThread.detach();


    } while (iResult > 0); // stops when stops receiving

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

void s_cleanup()
{
    BYTE b = 0;
    WriteProcessMemory(s_handle, (LPVOID)(s_altmechptr + 0x25D), &b, sizeof(b), nullptr);
}