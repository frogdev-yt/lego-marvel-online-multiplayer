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

struct Float3 {
    float x, y, z;
};

// ts slow ahh hell
void writeDets(GameStateDetails dets, HANDLE h, DWORD ptr, DWORD mechptr)
{
    DWORD context,anim3ptr;

    Float3 xyz = { dets.x, dets.y, dets.z };
    Float3 velxyz = { dets.velx, dets.vely, dets.velz };
    WriteProcessMemory(h, (LPVOID)(ptr + 0x70), &xyz, sizeof(xyz), nullptr);
    WriteProcessMemory(h, (LPVOID)(ptr + 0xc0), &velxyz, sizeof(velxyz), nullptr);
    WriteProcessMemory(h, (LPVOID)(mechptr + 0x170), &dets.rot, sizeof(dets.rot), nullptr);
    //WriteProcessMemory(h, (LPVOID)(mechptr + 0x41c), &dets.anim1, sizeof(dets.anim1), nullptr);
    WriteProcessMemory(h, (LPVOID)(mechptr + 0x430), &dets.anim2, sizeof(dets.anim2), nullptr);

    ReadProcessMemory(h, (LPCVOID)(mechptr + 0x428), &context, sizeof(context), nullptr);
    //ReadProcessMemory(h, (LPCVOID)(context + 74), &anim3ptr, sizeof(anim3ptr), nullptr);

    WriteProcessMemory(h, (LPVOID)(context + 4), &dets.randslop, sizeof(dets.randslop), nullptr);
    // WriteProcessMemory(h, (LPVOID)(anim3ptr + 8), &dets.anim1, sizeof(dets.anim1), nullptr);
}