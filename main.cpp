#include "orderbookmanager.h"
#include <fstream>
#include <string>

int main()
{
	std::ifstream cmdsFile("cmds.txt");

	OrderBookManager OBManager;

	if (cmdsFile.is_open())
	{
		std::string line;
		while (std::getline(cmdsFile, line)) {
			OBManager.action(line);
		}
	}

	OBManager.printOB();
	OBManager.printExceptions();
}