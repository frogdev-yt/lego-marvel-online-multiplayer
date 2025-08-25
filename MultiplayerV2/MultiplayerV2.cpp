#include "includes.h"
#include <string>

int isServer;



int main()
{

	// wish I could initialize the process read/write stuff here and then just do the actual read/write on server and client .cpp

	// loop forever
	// ask for server or client
	// run server or client script accordingly
	// server or client script does both updating of game state and networking on its own
	// if it crashes and returns 0, it just takes you back to asking if you want to host server or client

	while (true)
	{
		std::cout << "Server or client (1=Server, 0=Client)?: ";
		std::cin >> isServer;

		if (isServer == 1)
		{
			runServer();
			s_cleanup();
		}
		else
		{
			char addr[32];
			char port[6];

			std::cout << "Server address?: ";
			std::cin >> addr;
			std::cout << "Port?: ";
			std::cin >> port;
			runClient(addr,port);
			c_cleanup();
		}
	}
	

}