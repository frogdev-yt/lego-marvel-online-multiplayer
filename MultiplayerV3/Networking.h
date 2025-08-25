#pragma once

struct byte48 {
	char data[48];
};

struct GameStateDetails {
	float x;
	float y;
	float z;
	float velx;
	float vely;
	float velz;
	int anim1;
	int anim2;
	float rot;
	byte48 randslop;
};

int runClient(char* address, char* port);
int runServer();
int s_SendLoop(SOCKET sock, HANDLE handle, DWORD mainPtr, DWORD mechPtr);
int s_RecvLoop(SOCKET sock, HANDLE handle, DWORD altPtr, DWORD altMechPtr);
int c_SendLoop(SOCKET sock, HANDLE handle, DWORD mainPtr, DWORD mechPtr);
int c_RecvLoop(SOCKET sock, HANDLE handle, DWORD altPtr, DWORD altMechPtr);