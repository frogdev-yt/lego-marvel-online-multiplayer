#define NOMINMAX
#include "includes.h"
#include "MultiplayerTest.h"
#include <limits>  // For std::numeric_limits

float altX,altY,altZ,altVelX,altVelY,altVelZ,mainX,mainY,mainZ,mainVelX,mainVelY,mainVelZ = 0;
int isServer;
bool loop;
HANDLE handle;

// all hex addresses are DWORDS - 4 byte unsigned data chunks
DWORD pID, baseModule, altMechPtr, altPtr, mainPtr;


int main()
{

	pID = GetProcessID(L"LEGOMARVEL.exe");

	baseModule = GetModuleBaseAddress(pID, L"LEGOMARVEL.exe");

	std::cout << pID << std::endl;

	handle = OpenProcess(PROCESS_ALL_ACCESS, NULL, pID);

	// creates a new thread to run the server and starts it
	// threads share global variables
	// pick server or client here, then run accordingly

	while (true) {
		std::cout << "Do you want to be the server? (1 = Yes, 0 = No): ";
		std::cin >> isServer;

		// Check for input failure (e.g., user enters letters)
		if (std::cin.fail()) {
			std::cin.clear(); // Clear error flags
			std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Discard bad input
			std::cout << "Invalid input. Please enter 1 or 0.\n";
			continue;
		}

		// Check that input is 0 or 1
		if (isServer == 0 || isServer == 1) {
			break;
		}

		std::cout << "Invalid choice. Please enter 1 for server or 0 for client.\n";
	}

	if (isServer == 1)
	{
		std::thread serverThread(runServer);
		serverThread.detach();
		printf("slopper\n");
	} 
	else 
	{
		std::string address = "";
		std::cout << "Enter server address: ";
		std::cin >> address;
		const char* caddr = address.c_str();
		std::thread clientThread(runClient,caddr);
		clientThread.detach();
		printf("cslopper\n");
	}

	// moved to readwriteloop
	while (true)
	{
		// read static offset addresses
		ReadProcessMemory(handle, (LPCVOID)(baseModule + 0x15B0884), &mainPtr, sizeof(mainPtr), nullptr);
		ReadProcessMemory(handle, (LPCVOID)(baseModule + 0x1599B9C), &altMechPtr, sizeof(altMechPtr), nullptr);

		// read main char xyz and vel to send to other client
		ReadProcessMemory(handle, (LPCVOID)(mainPtr + 0x70), &mainX, sizeof(mainX), nullptr);
		ReadProcessMemory(handle, (LPCVOID)(mainPtr + 0x74), &mainY, sizeof(mainY), nullptr);
		ReadProcessMemory(handle, (LPCVOID)(mainPtr + 0x78), &mainZ, sizeof(mainZ), nullptr);
		ReadProcessMemory(handle, (LPCVOID)(mainPtr + 0xc0), &mainVelX, sizeof(mainVelX), nullptr);
		ReadProcessMemory(handle, (LPCVOID)(mainPtr + 0xc4), &mainVelY, sizeof(mainVelY), nullptr);
		ReadProcessMemory(handle, (LPCVOID)(mainPtr + 0xc8), &mainVelZ, sizeof(mainVelZ), nullptr);

		if (isServer == 1)
		{
			serversend(DetailStruct{ mainX, mainY, mainZ, mainVelX, mainVelY, mainVelZ });
		}
		else
		{
			clientsend(DetailStruct{ mainX, mainY, mainZ, mainVelX, mainVelY, mainVelZ });
		}

		// write alt char xyz with values sent from other client
		ReadProcessMemory(handle, (LPCVOID)(altMechPtr + 0x184), &altPtr, sizeof(altPtr), nullptr);
		WriteProcessMemory(handle, (LPVOID)(altPtr + 0x70), &altX, sizeof(altX), nullptr);
		WriteProcessMemory(handle, (LPVOID)(altPtr + 0x74), &altY, sizeof(altY), nullptr);
		WriteProcessMemory(handle, (LPVOID)(altPtr + 0x78), &altZ, sizeof(altZ), nullptr);
	}

}

void toggleMainLoop()
{
	loop = true;
}

void setAltDets(DetailStruct dets)
{
	altX = dets.x;
	altY = dets.y;
	altZ = dets.z;
	
	// write vel straight away
	// realistically should do this for x as well, right now it's writing the memory every frame when it doesn't have to
	// only needs to write the x when it gets a new x value
	// only needs to read it when it's going to send a new value
	// but will add a slight delay to reading if it needs to ask for a new value every time, so reading can happen every frame
	// or maybe not idk
	WriteProcessMemory(handle, (LPVOID)(altPtr + 0xc0), &dets.velx, sizeof(dets.velx), nullptr);
	WriteProcessMemory(handle, (LPVOID)(altPtr + 0xc4), &dets.vely, sizeof(dets.vely), nullptr);
	WriteProcessMemory(handle, (LPVOID)(altPtr + 0xc8), &dets.velz, sizeof(dets.velz), nullptr);
}
