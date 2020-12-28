#include "orderbookmanager.h"
#include <fstream>
#include <string>

int main()
{
	std::ifstream cmdsFile("cmds.txt");

	OrderBookManager OBManager;
	int lineNo = 0;

	if (cmdsFile.is_open())
	{
		std::string line;
		while (std::getline(cmdsFile, line)) {
			OBManager.action(line);
			if (++lineNo % 10 == 0)
			{
				OBManager.printOB();
				OBManager.printExceptions();
			}
		}
	}

	OBManager.printOB();
	OBManager.printExceptions();
}