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
    // pID = GetProcessID(L"LEGOMARVEL.exe");
    HWND hwnd = FindWindowA(NULL, "LEGO® MARVEL Super Heroes");
    GetWindowThreadProcessId(hwnd, &pID);

    baseModule = GetModuleBaseAddress(pID, L"LEGOMARVEL.exe");
    handle = OpenProcess(PROCESS_ALL_ACCESS, NULL, pID);

    printf("pID: %d\n", pID);

    // read pointer locations, will change when game restarted but server should not run without game anyway
    // nvm these also change when switching characters you goofy sloober
    // but I'm leaving this as is for now
    ReadProcessMemory(handle, (LPCVOID)(baseModule + 0x15B0884), &mainPtr, sizeof(mainPtr), nullptr);
    ReadProcessMemory(handle, (LPCVOID)(baseModule + 0x1599B98), &mechPtr, sizeof(mechPtr), nullptr);
    ReadProcessMemory(handle, (LPCVOID)(baseModule + 0x1599B9C), &altMechPtr, sizeof(altMechPtr), nullptr);
    ReadProcessMemory(handle, (LPCVOID)(altMechPtr + 0x184), &altPtr, sizeof(altPtr), nullptr);

    printf("mechPtr: %X\n", mechPtr);

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

    // set socket to nodelay mode
    BOOL opt = TRUE;
    setsockopt(ClientSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&opt, sizeof(opt));

    printf("client connected!");

    // detach read/send thread - should stop running if recv thread stops running
    std::thread sendThread(s_SendLoop, ClientSocket, handle, mainPtr, mechPtr);
    sendThread.detach();

    // run receive/write thread in main loop - should return error if send thread stops running
    int test = s_RecvLoop(ClientSocket,handle,altPtr,altMechPtr);
    if (test != 0)
    {
        return(test);
    }

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

int s_SendLoop(SOCKET sock, HANDLE handle, DWORD mainPtr, DWORD mechPtr) // read and send at 125 Hz
{
    int iSendResult;
    DWORD context;

    float x, y, z, velx, vely, velz, rot;
    int anim1, anim2;
    byte48 randomcontextslop;

    auto nextsend = std::chrono::high_resolution_clock::now();

    do {
        // 1/0.008 = 125 Hz
        auto idealFrameTime = std::chrono::milliseconds(8);
        auto currenttime = std::chrono::high_resolution_clock::now();
        nextsend = currenttime + idealFrameTime;

        ReadProcessMemory(handle, (LPCVOID)(mainPtr + 0x70), &x, sizeof(x), nullptr);
        ReadProcessMemory(handle, (LPCVOID)(mainPtr + 0x74), &y, sizeof(y), nullptr);
        ReadProcessMemory(handle, (LPCVOID)(mainPtr + 0x78), &z, sizeof(z), nullptr);
        ReadProcessMemory(handle, (LPCVOID)(mainPtr + 0xc0), &velx, sizeof(velx), nullptr);
        ReadProcessMemory(handle, (LPCVOID)(mainPtr + 0xc4), &vely, sizeof(vely), nullptr);
        ReadProcessMemory(handle, (LPCVOID)(mainPtr + 0xc8), &velz, sizeof(velz), nullptr);
        ReadProcessMemory(handle, (LPCVOID)(mechPtr + 0x41C), &anim1, sizeof(anim1), nullptr);
        ReadProcessMemory(handle, (LPCVOID)(mechPtr + 0x428), &context, sizeof(context), nullptr);
        ReadProcessMemory(handle, (LPCVOID)(mechPtr + 0x430), &anim2, sizeof(anim2), nullptr);
        ReadProcessMemory(handle, (LPCVOID)(mechPtr + 0x170), &rot, sizeof(rot), nullptr);
        ReadProcessMemory(handle, (LPCVOID)(context + 4), &randomcontextslop, sizeof(randomcontextslop), nullptr);


        // checking if my reading of the randomcontextslop bytes was correcty
        /*
        for (int i = 0;i < 48;i++)
        {
            printf("randomslop %d = %X\n",i,randomcontextslop[i]);
        }
        */



        GameStateDetails senddets = { x,y,z,velx,vely,velz,anim1,anim2,rot,randomcontextslop };
        iSendResult = send(sock, reinterpret_cast<char*>(&senddets), sizeof(senddets), 0);

        if (iSendResult == SOCKET_ERROR) {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(sock);
            WSACleanup();
            return 1;
        }

        // wait until 8 ms has passed
        while (currenttime < nextsend) {
            currenttime = std::chrono::high_resolution_clock::now();
        }

    } while (iSendResult != SOCKET_ERROR);
} 

int s_RecvLoop(SOCKET sock, HANDLE handle, DWORD altPtr, DWORD altMechPtr) // recv and write as fast as possible
{

    int iResult;
    GameStateDetails recvdets;

    // initialize delta clock
    clock_t deltaTime;
    deltaTime = clock();

    do {

        iResult = recv(sock, reinterpret_cast<char*>(&recvdets), sizeof(recvdets), 0);
        if (iResult > 0)
        {
            writeDets(recvdets, handle, altPtr, altMechPtr);
            deltaTime = clock() - deltaTime;
            printf("Recv delta (<8): %d\n", (int)deltaTime);
            deltaTime = clock();
        } 
        else if (iResult == 0)
        {
            printf("Connection closing...\n");
        }
        else
        {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(sock);
            WSACleanup();
            return 1;
        }

    } while (iResult != 0);

}

void s_cleanup()
{
    BYTE b = 0;
    WriteProcessMemory(s_handle, (LPVOID)(s_altmechptr + 0x25D), &b, sizeof(b), nullptr);
}