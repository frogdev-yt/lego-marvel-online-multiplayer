#include "includes.h"

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

HANDLE c_handle;
DWORD c_altmechptr;

int runClient(char* address, char* port)
{
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL,
        * ptr = NULL,
        hints;
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
    c_handle = handle;
    c_altmechptr = altMechPtr;


    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(address, port, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    printf("addrinfo received\n");

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL;ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            printf("connect failed with error: %ld\n", WSAGetLastError());
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    printf("connected!");


    // initialize delta clock
    clock_t deltaTime;
    deltaTime = clock();
    int loopIteration = 0;

    // Send + receive until the peer closes the connection
    do {

        // read game details
        //std::thread readThread(readDets, &mainX, &mainY, &mainZ, &mainVelX, &mainVelY, &mainVelZ, handle, mainPtr);
        //readThread.detach();

        // check fps every 50 or so frames
        /*
        loopIteration += 1;
        if (loopIteration == 50)
        {
            deltaTime = clock() - deltaTime;
            //printf("time since last frame: %f\n", (float)deltaTime);
            float fps = (loopIteration * 1000) / (float)deltaTime;
            printf("fps: %f\n", fps);
            deltaTime = clock();
            loopIteration = 0;
        }*/

        // 1/0.008 = 125 fps
        deltaTime = clock() - deltaTime;
        int waitTime = 0;
        if (deltaTime < 8)
        {
            waitTime = 8 - int(deltaTime);
            Sleep(waitTime);
        }
        printf("delta (anything less than 8ms is coo): %d\n", int(deltaTime));
        printf("Frame per second %d\n", 1000 / (waitTime + int(deltaTime)));
        deltaTime = clock();

        
        // last I changed was this, trying to put read and write on threads to speed up the network loop
        ReadProcessMemory(handle, (LPCVOID)(mainPtr + 0x70), &mainX, sizeof(mainX), nullptr);
        ReadProcessMemory(handle, (LPCVOID)(mainPtr + 0x74), &mainY, sizeof(mainY), nullptr);
        ReadProcessMemory(handle, (LPCVOID)(mainPtr + 0x78), &mainZ, sizeof(mainZ), nullptr);
        ReadProcessMemory(handle, (LPCVOID)(mainPtr + 0xc0), &mainVelX, sizeof(mainVelX), nullptr);
        ReadProcessMemory(handle, (LPCVOID)(mainPtr + 0xc4), &mainVelY, sizeof(mainVelY), nullptr);
        ReadProcessMemory(handle, (LPCVOID)(mainPtr + 0xc8), &mainVelZ, sizeof(mainVelZ), nullptr);
        ReadProcessMemory(handle, (LPCVOID)(mechPtr + 0x41C), &anim1, sizeof(anim1), nullptr);
        ReadProcessMemory(handle, (LPCVOID)(mechPtr + 0x430), &anim2, sizeof(anim2), nullptr);
        ReadProcessMemory(handle, (LPCVOID)(mechPtr + 0x170), &rot, sizeof(rot), nullptr);

        /*
        // checking to see that its writing
        float writetest;
        ReadProcessMemory(handle, (LPCVOID)(altPtr + 0x70), &writetest, sizeof(writetest), nullptr);
        printf("written x: %f\n", writetest);
        */
        

        // Send game details first, then recv, cause server is waiting for initial send
        senddets = {mainX,mainY,mainZ,mainVelX,mainVelY, mainVelZ, anim1, anim2, rot};
        iSendResult = send(ConnectSocket, reinterpret_cast<char*>(&senddets), sizeof(senddets), 0);
        if (iSendResult == SOCKET_ERROR) {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(ConnectSocket);
            WSACleanup();
            return 1;
        }

        //printf("Bytes Sent: %ld\n", iResult);

        // Receive game details
        iResult = recv(ConnectSocket, reinterpret_cast<char*>(&recvdets), sizeof(recvdets), 0);
        /*if (iResult > 0)
            printf("Bytes received: %d\n", iResult);
        else */if (iResult == 0)
            printf("Connection closed\n");
        else if (!(iResult > 0))
            printf("recv failed with error: %d\n", WSAGetLastError());

        // write game details
        std::thread writeThread(writeDets,recvdets,handle,altPtr,altMechPtr);
        writeThread.detach();

    } while (iResult > 0); // stops when stops receiving

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

void c_cleanup()
{
    BYTE b = 0;
    WriteProcessMemory(c_handle, (LPVOID)(c_altmechptr + 0x25D), &b, sizeof(b), nullptr);
}