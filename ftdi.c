/*
 * written by Dennis Ho 2015, July
 * 
 * Utility for speed check on sync fifo
 */
/*
 * Permission is hereby granted, free of charge, to any person obtaining a copy
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
#include <stdio.h>
#include <ftd2xx.h>
#include <sys/time.h>
#define SIZE 4096
DWORD dwNumBytesToRead = 1;
DWORD dwNumBytesRead;
unsigned char byInputBuffer[SIZE]; 
DWORD dwNumBytesSent;
DWORD dwNumBytesToSend;
unsigned char byOutputBuffer[SIZE]; 
int ft232H = 0; 
DWORD dwClockDivisor = 0;
DWORD dwCount;

void readhex(int s) {
	printf("size=%d>", s);
	for (int i = 0; i < s && i < 16; i++) {
		printf("%02X:", byInputBuffer[i]);
	}
	printf("\n");
}

long getTimeStamp() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * (long) 1000000 + tv.tv_usec;
}

int main(int argc, char *argv[]) {
	DWORD bytesWritten = 0;
	FT_STATUS ftStatus = FT_OK;
	FT_HANDLE ftHandle;
	UCHAR outputData;
	UCHAR pinStatus;
	int portNumber;
	long tstart, tlast, tcurr;
	long dcount = 0;

	if (argc > 1) {
		sscanf(argv[1], "%d", &portNumber);
	} else {
		portNumber = 0;
	}

	ftStatus = FT_Open(portNumber, &ftHandle);
	if (ftStatus != FT_OK) {
		printf("FT_Open(%d) failed (error %d).\n", portNumber, (int) ftStatus);
		return 1;
	}

	printf("Selecting asynchronous SYNC FIFO mode.\n");
	ftStatus |= FT_SetBitMode(ftHandle, 0xFF, 0x00);
	ftStatus |= FT_SetBitMode(ftHandle, 0xFF, 0x40);
	if (ftStatus != FT_OK) {
		printf("FT_SyncFIFO Mode failed (error %d).\n", (int) ftStatus);
		goto exit;
	}

	ftStatus |= FT_SetLatencyTimer(ftHandle, 1);
	ftStatus |= FT_SetUSBParameters(ftHandle, SIZE, SIZE);  
	ftStatus |= FT_SetFlowControl(ftHandle, FT_FLOW_NONE, 0x00, 0x00);
	ftStatus |= FT_SetTimeouts(ftHandle, 1000, 1000);
	if (ftStatus != FT_OK) {
		printf("FT_SetBaudRate failed (error %d).\n", (int) ftStatus);
		goto exit;
	}
	tstart = getTimeStamp();
	tlast = tstart;
	printf("bytesend=%d\n", dwNumBytesSent);
	for (int i = 0; i < 65536; i++) {
		//memset(byOutputBuffer, 0, SIZE);
		for (int j = 0; j < SIZE; j++) {
			byOutputBuffer[j] = (j + i) & 0xff;
		}
		FT_Write(ftHandle, byOutputBuffer, 1, &dwNumBytesSent);
		dwNumBytesRead = 0;
		ftStatus = FT_Read(ftHandle, byInputBuffer, SIZE, &dwNumBytesRead);
		dcount += dwNumBytesRead;
		tcurr = getTimeStamp();
		if ((tcurr - tlast) > 1000000) {
			printf("read bytes=%ld k/sec\n",
					dcount / 1024 / ((tcurr - tstart) / 1000000));
			tlast = tcurr;
			if (dwNumBytesRead > 0) {
				readhex(dwNumBytesRead);
			} else {
				printf("no data\n");
			}
		}
		if (ftStatus != FT_OK) {
			printf("IO FAIL.\n");
			goto exit;
		}
	}
		exit:
		(void) FT_SetBitMode(ftHandle, 0, FT_BITMODE_RESET);
		(void) FT_Close(ftHandle);
		return 0;
	}
