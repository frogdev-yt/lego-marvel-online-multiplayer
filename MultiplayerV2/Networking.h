#pragma once

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
};

int runClient(char* address, char* port);
int runServer();