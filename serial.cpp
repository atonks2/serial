/*
MIT License

Copyright (c) 2017 Alan Tonks

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "serial.h"
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

Serial::Serial() {
	PORT = "/dev/ttyUSB0"; /* A default to use if no port is specified */
	if (setBaud(4800) == 0) BAUDRATE = 4800; /* Use 4800 baud if no rate is specified */
	isOpen = false; /* Port isn't open yet */
	isCanonical = true;
	init();
}

Serial::Serial(speed_t baud, std::string port)
{
	if (setBaud(baud) == 0) BAUDRATE = baud; /* Check for valid baudrate and set the flag if it is valid */
	isOpen = false; /* Port isn't open yet */
	isCanonical = true;
	struct stat stat_buf; /* Used in checking if specified port exists */
	if (stat(port.c_str(), &stat_buf) == 0) /* Make sure the specified port exists */
		PORT = port; // port is valid
	else {
		std::cout << "Device not found." << std::endl; /* Port not found */
		exit(-1);
	}
	init();
}

Serial::Serial(speed_t baud, std::string port, bool canon)
{
	isCanonical = canon;
	if (setBaud(baud) == 0) BAUDRATE = baud;
	isOpen = false;

	// See if specified port exists. Port not open yet, so can't use
	// isatty(int fd).
	struct stat stat_buf;
	if (stat(port.c_str(), &stat_buf) == 0) PORT = port;
	else {  // Port doesn't exist
		std::cout << "Device not found." << std::endl;
		exit(-1);
	}
	init();
}

// Open and configure the port
void Serial::init()
{
	// open port for read and write, not controlling
	dev_fd = open(PORT.c_str(), O_RDWR | O_NOCTTY);
	if (dev_fd < 0) {
		perror("Failed to open device: ");
		exit(-1);
	}
	else if (dev_fd >= 0) isOpen = true;

	// Made a copy of the current configuration so it can be
	// restored in destructor.
	tcgetattr(dev_fd, &old_config);

	// define configuration vars for termios struct
	memset(&term, 0, sizeof(term));  // Clear junk from location of term to start with clean slate
	tcgetattr(dev_fd, &term);  // Per GNU C, get current config and modify the flags you need. Don't start from scratch.
	
	// BAUDRATE: Integer multiple of 2400
	// CRTSCTS: Hardware flow control
	// CS8: 8N1
	// CLOCAL: No modem control. (local device)
	// CREAD: Receive chars
	term.c_cflag |= (BAUDRATE | CS8 | CLOCAL | HUPCL | CREAD);

    // IGNPAR: Ignore parity errors
	term.c_iflag |= IGNPAR;
	// 0 for raw output
	term.c_oflag = 0;
	// Setting input mode
	if (isCanonical)  term.c_lflag |= ICANON;  // Canonical input
	else {
		//Configure non-canonical mode
		term.c_lflag &= ~(ICANON | ECHO);  // Disable canonical mode and echo
		term.c_cc[VMIN] = 1;  // Minimum number of chars to read before returning
		term.c_cc[VTIME] = 0;  // Timeout in deciseconds. 0 to disregard timing between bytes
	}
	tcflush(dev_fd, TCIFLUSH);
	applyNewConfig();
}

int Serial::setBaud(speed_t baud)
{
	if (!isOpen) return -1;
	int status = -1;
	switch (baud) {
	case 2400:
		status = cfsetspeed(&term, B2400);
		break;
	case 4800:
		status = cfsetspeed(&term, B4800);
		break;
	case 9600:
		status = cfsetspeed(&term, B9600);
		break;
	case 19200:
		status = cfsetspeed(&term, B19200);
		break;
	case 38400:
		status = cfsetspeed(&term, B38400);
		break;
	case 57600:
		status = cfsetspeed(&term, B57600);
		break;
	case 115200:
		status = cfsetspeed(&term, B115200);
		break;
	case 230400:
		status = cfsetspeed(&term, B230400);
		break;
	default:
		std::cout << "Invalid baudrate requested.\n";
		return -1;
	}
	if (status < 0) {
		perror("Failed to set requested baudrate: ");
		return -1;
	}
	else return status;
}

int Serial::applyNewConfig()
{
	if (isOpen) {
		if (tcsetattr(dev_fd, TCSANOW, &term) < 0) perror("Could not apply configuration: ");
		else return 0;
	}
	else return -1;
}

speed_t Serial::getBaud()
{
	if (isOpen) return cfgetispeed(&term);
	else return 1;
}

termios Serial::getConfig()
{
	return term;
}

char* setupRead()
{
	if (!isOpen) return -1;
	if (tcflush(dev_fd, TCIOFLUSH < 0)) {
		perror("Could not flush line: ");
		return -1;
	}
	char* buf = new char[255];
	return buf
}

int Serial::serialRead()
{
	if (!isOpen) return -1;
	int buf_size = 255; // 82 is longest NMEA Sentence
	char buf[buf_size];
	if (tcflush(dev_fd, TCIOFLUSH < 0)) perror("Could not flush line: ");
	bytes_received = read(dev_fd, buf, buf_size);
	if (bytes_received < 0) perror("Read failed: ");
	else buf[bytes_received] = '\0';  // Null terminate the string
	data.assign(buf);  // store data as a c++ string
	return bytes_received;
}

int Serial::serialRead(int bytes)
{
	if (!isOpen) return -1;
	int buf_size = 255;
	char buf[buf_size];
	if (tcflush(dev_fd, TCIOFLUSH < 0)) perror("Could not flush line: ");
	if (bytes > buf_size) bytes = buf_size;
	bytes_received = read(dev_fd, buf, bytes);
	if (bytes_received < 0) perror("Read failed: ");
	else buf[bytes_received] = '\0';
	data.assign(buf);
	return bytes_received;
}

std::string Serial::getData()
{
	if (isOpen) return data;
	else return "Open serial port first!\n";
}

Serial::~Serial()
{
	tcsetattr(dev_fd, TCSANOW, &old_config); /* Leave port how we found it. */
	close(dev_fd); /* close the port */
}
