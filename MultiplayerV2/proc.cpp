#include "proc.h"

DWORD GetProcessID(const wchar_t* procName)
{
    DWORD procID = 0;
    HANDLE hSnap = (CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));

    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 procEntry;
        procEntry.dwSize = sizeof(procEntry);

        if (Process32First(hSnap, &procEntry)) {
            do {
                if (!_wcsicmp(procEntry.szExeFile, procName)) {
                    procID = procEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnap, &procEntry));
        }
    }

    CloseHandle(hSnap);
    return procID;
}

DWORD GetModuleBaseAddress(DWORD procId, const wchar_t* modName)
{
    DWORD modBaseAddr = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);

    if (hSnap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32 modEntry;
        modEntry.dwSize = sizeof(modEntry);

        if (Module32First(hSnap, &modEntry)) {
            do {
                if (!_wcsicmp(modEntry.szModule, modName)) {
                    modBaseAddr = (DWORD)modEntry.modBaseAddr;
                    break;
                }


            } while (Module32Next(hSnap, &modEntry));
        }
    }

    CloseHandle(hSnap);
    return modBaseAddr;
}

// could change these to take pointers instead of accessing global vars but client and server script use like all the same vars - we'll see
/*void readDets(float* x, float* y, float* z, float* velx, float* vely, float* velz, HANDLE h, DWORD ptr)
{
    ReadProcessMemory(h, (LPCVOID)(ptr + 0x70), &x, sizeof(x), nullptr);
    ReadProcessMemory(h, (LPCVOID)(ptr + 0x74), &y, sizeof(y), nullptr);
    ReadProcessMemory(h, (LPCVOID)(ptr + 0x78), &z, sizeof(z), nullptr);
    ReadProcessMemory(h, (LPCVOID)(ptr + 0xc0), &velx, sizeof(velx), nullptr);
    ReadProcessMemory(h, (LPCVOID)(ptr + 0xc4), &vely, sizeof(vely), nullptr);
    ReadProcessMemory(h, (LPCVOID)(ptr + 0xc8), &velz, sizeof(velz), nullptr);
}*/

void writeDets(GameStateDetails dets, HANDLE h, DWORD ptr, DWORD mechptr)
{
    //printf("xyz: %f, %f, %f\n", dets.x, dets.y, dets.z);
    WriteProcessMemory(h, (LPVOID)(ptr + 0x70), &dets.x, sizeof(dets.x), nullptr);
    WriteProcessMemory(h, (LPVOID)(ptr + 0x74), &dets.y, sizeof(dets.y), nullptr);
    WriteProcessMemory(h, (LPVOID)(ptr + 0x78), &dets.z, sizeof(dets.z), nullptr);
    WriteProcessMemory(h, (LPVOID)(ptr + 0xc0), &dets.velx, sizeof(dets.velx), nullptr);
    WriteProcessMemory(h, (LPVOID)(ptr + 0xc4), &dets.vely, sizeof(dets.vely), nullptr);
    WriteProcessMemory(h, (LPVOID)(ptr + 0xc8), &dets.velz, sizeof(dets.velz), nullptr);
    WriteProcessMemory(h, (LPVOID)(mechptr + 0x170), &dets.rot, sizeof(dets.rot), nullptr);
    WriteProcessMemory(h, (LPVOID)(mechptr + 0x41c), &dets.anim1, sizeof(dets.anim1), nullptr);
    WriteProcessMemory(h, (LPVOID)(mechptr + 0x430), &dets.anim2, sizeof(dets.anim2), nullptr);
}