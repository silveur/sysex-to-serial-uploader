/**
@file SeaHeader.h
@author  Silvere Letellier
@author silvere.letellier@gmail.com
@version 1.0
@copyright Copyright (c) 2014 ROLI Ltd.
@section LICENSE
BSD 2
Copyright (c) <YEAR>, <OWNER>
All rights reserved.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

unsigned int pause_between_block = 100;
char output_device[64];
char input_file[64];
unsigned int baud_rate = 115200;
int file_descriptor;
int verbose =0;
struct termios m_oldtio;

int upload_file(char* input, char* output, unsigned int pause);
int open_serial(char* serial_port, unsigned int baud);
int close_serial(int file_descriptor);

int main(int argc,char *argv[])
{
	int i;
	for(i=1;i<argc;i++)
	{
		if (!strcmp(argv[i], "-f"))
		{
			strcpy(input_file, argv[i+1]);
			printf("Input file: %s\n", input_file);
			i++;
		}
		else if (!strcmp(argv[i], "-o"))
		{
			strcpy(output_device, argv[i+1]);
			printf("Output device: %s\n", output_device);
			i++;
		}
		else if (!strcmp(argv[i], "-p"))
		{
			pause_between_block = atoi(argv[i+1]);
			printf("Pause between blocks: %u\n", pause_between_block);
			i++;
		}
		else if (!strcmp(argv[i], "-b"))
		{
			baud_rate = atoi(argv[i+1]);
			printf("Baud rate: %u\n", baud_rate);
			i++;
		}
		else if (!strcmp(argv[i], "-v"))
		{
			verbose = 1;
			printf("Verbose mode on\n");
		}
		else
		{
			printf("-f: Absolute path file input\n-o: Serial output device\n-p: Pause between sysex blocks(100ms)\n-b: Baud rate(115200)\n-v: Verbose mode\n");
			exit(EXIT_FAILURE);
		}
	}
	file_descriptor = open_serial(output_device, baud_rate);
	if (file_descriptor > -1)
	{
		upload_file(input_file, output_device, pause_between_block);
		if (close_serial(file_descriptor) > -1)
			exit(EXIT_SUCCESS);
		else
		{
			printf("Error while closing the serial line...\n");
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		printf("Aborting...\n");
		exit(EXIT_FAILURE);
	}
}

int upload_file(char* input, char* output, unsigned int pause)
{
	FILE* sysex_file = fopen(input, "rb");
	unsigned char *buffer;
	unsigned long fileLen;
	unsigned i;
	if (sysex_file == NULL)
	{
		fprintf(stderr, "Can't open input file: %s\n", input_file);
		exit(EXIT_FAILURE);
	}
	
	fseek(sysex_file, 0, SEEK_END);
	fileLen=ftell(sysex_file);
	printf("File size: %lu \n", fileLen);
	fseek(sysex_file, 0, SEEK_SET);
	buffer=(unsigned char *)malloc(fileLen);
	if (!buffer)
	{
		fprintf(stderr, "Memory error!");
		fclose(sysex_file);
		exit(EXIT_FAILURE);;
	}
	
	fread(buffer, fileLen, 1, sysex_file);
	fclose(sysex_file);
	
	unsigned char* byte_to_upload = NULL;
	printf("Uploading... Please wait.\n");
	for(i=0;i<fileLen;i++)
	{
		byte_to_upload = &buffer[i];
		if (verbose)
			printf("0x%02x,", *byte_to_upload);
		long size = write(file_descriptor, byte_to_upload, 1);
		if (*byte_to_upload == 0xf7)
			usleep(pause * 1000);
		if (!size)
		{
			printf("Transfer error\n");
			free(buffer);
			exit(EXIT_FAILURE);
		}
	}
	free(buffer);
	printf("Uploaded %lu bytes. Exiting...\n", fileLen);
	return EXIT_SUCCESS;
}

int open_serial(char* serial_port, unsigned int baud)
{
	struct termios tio;
	file_descriptor = open(serial_port, O_RDWR | O_NOCTTY | O_NDELAY);
	if (file_descriptor == -1)
	{
		perror(serial_port);
		return -1;
	}
	if (tcgetattr(file_descriptor, &tio) < 0)
	{
		perror(serial_port);
		return -1;
	}
	m_oldtio = tio;
	fcntl(file_descriptor, F_SETFL, 0);
	
	/* Configure port */
	tio.c_cflag = baud | CS8 | CLOCAL | CREAD;
	tio.c_iflag = IGNPAR;
	tio.c_oflag = 0;
	tio.c_lflag = 0;
	tio.c_cc[VMIN] = 1;
	tio.c_cc[VTIME] = 0;
	cfsetispeed(&tio, baud);
	cfsetospeed(&tio, baud);
	
	tcflush(file_descriptor, TCIFLUSH);
	if (tcsetattr(file_descriptor, TCSANOW, &tio) < 0)
	{
		perror(serial_port);
		return -1;
	}
	if (file_descriptor < 0)
	{
		perror(serial_port);
		return -1;
	}
	else
		printf("------- Serial port opened ok --------\n");
	
	return file_descriptor;
}

int close_serial(int file_descriptor)
{
	printf("------- Closing serial port ----------\n");
	tcsetattr(file_descriptor, TCSANOW, &m_oldtio);
	return close(file_descriptor);
}

