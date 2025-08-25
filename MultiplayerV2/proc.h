#pragma once

#include "includes.h"

DWORD GetProcessID(const wchar_t* procName);

DWORD GetModuleBaseAddress(DWORD procID, const wchar_t* modName);

// void readDets(float* x, float* y, float* z, float* velx, float* vely, float* velz, HANDLE h, DWORD ptr);
void writeDets(GameStateDetails dets, HANDLE h, DWORD ptr, DWORD mechptr);

void s_cleanup();
void c_cleanup();