#include <iostream>
#include <string>
#include "serial.h"

speed_t baud = 4800;
std::string port = "/dev/ttyUSB0";
Serial gps(baud, port);

void readGPS()
{
    for(int i = 0; i < 4; ++i) {
        std::cout << "Sentence: " << gps.serialRead();
    }
}

void printConfig()
{
    termios cur_config = gps.getConfig();
    std::cout << "c_cflag: " << std::hex << cur_config.c_cflag << std::endl;
    std::cout << "c_iflag: " << std::hex << cur_config.c_iflag << std::endl;
    std::cout << "c_oflag: " << std::hex << cur_config.c_oflag << std::endl;
    std::cout << "c_lflag: " << std::hex << cur_config.c_lflag << std::endl;
}

int main()
{
    printConfig();
    std::cout << "Baud 1: " << gps.getBaud() << std::endl;
    std::cout << "Applying change: " << gps.applyNewConfig() << std::endl;

    baud = 9600;
    std::cout << "Setting Baud to " << baud << ": " << gps.setBaud(baud) << std::endl;
    std::cout << "Applying change: " << gps.applyNewConfig() << std::endl;
    std::cout << "Baud 2: " << gps.getBaud() << std::endl;
	
    baud = 4800;
    std::cout << "Setting Baud to " << baud << ": " << gps.setBaud(baud) << std::endl;
    std::cout << "Applying change: " << gps.applyNewConfig() << std::endl;
    readGPS();

    baud = 4800;
    std::cout << "Restoring baud to " << baud << ": "  << gps.setBaud(baud) << std::endl;
    std::cout << "Applying change: " << gps.applyNewConfig() << std::endl;
    std::cout << "Baud 2: " << gps.getBaud() << std::endl;
    readGPS();

    return 0;
}
