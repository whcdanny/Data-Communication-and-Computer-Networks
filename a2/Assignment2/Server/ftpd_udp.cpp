// SERVER TCP PROGRAM
// revised and tidied up by
// J.W. Atwood
// 1999 June 30
// There is still some leftover trash in this code.



/* send and receive codes between client and server */
/* This is your basic WINSOCK shell */
#pragma once
#pragma comment( linker, "/defaultlib:ws2_32.lib" )
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include <winsock.h>
#include <string>
#include <iostream>
#include <windows.h>
#include <fstream>
#include <tchar.h> 
#include <stdio.h>
#include <stdlib.h>
#include <strsafe.h>
#include <vector>
#include <comdef.h> 
#include <time.h>
#pragma comment(lib, "User32.lib")

using namespace std;

//port data types
//#define REQUEST_PORT 0x7070
#define REQUEST_PORT 5001;
int port = REQUEST_PORT;

#define TRACE 1

//socket data types
SOCKET s;
SOCKADDR_IN sa;        // port 5001, server IP address
SOCKADDR_IN sa_in;     // router info, IP, port(7001)

//packet structure definition
#define STOPNWAIT 1 //Bits width For stop and wait
struct MESSAGE_FRAME {
	unsigned int header;
	unsigned int snwseq : STOPNWAIT;
	char packet_type;
	char data[80];//or error message
} msgfrm, msgrecv;

//three way handshake definition
struct THREE_WAY_HS{
	int client_number;
	int server_number;
	char packet_type;
	char direction[5];
	char file_name[2000];
} three_way_send, three_way_recv;

union {
	struct sockaddr generic;
	struct sockaddr_in ca_in;
}ca;

int calen = sizeof(ca);

//buffer data types
char clientbuffer[128];
char filedir[5];
char filename[2000];
char *buffer;
int ibufferlen;
int ibytesrecv;
int ibytessent;

//host data types
char localhost[20];

HOSTENT *hp;

//wait variables
int nsa1;
int r, infds = 1, outfds = 0;
struct timeval timeout;
const struct timeval *tp = &timeout;

fd_set readfds;

//others
HANDLE test;
DWORD dwtest;

//reference for used structures

/*  * Host structure
struct  hostent {
char    FAR * h_name;             official name of host *
char    FAR * FAR * h_aliases;    alias list *
short   h_addrtype;               host address type *
short   h_length;                 length of address *
char    FAR * FAR * h_addr_list;  list of addresses *
#define h_addr  h_addr_list[0]            address, for backward compat *
};
* Socket address structure
struct sockaddr_in {
short   sin_family;
u_short sin_port;
struct  in_addr sin_addr;
char    sin_zero[8];
}; */

//unsigned __stdcall ClientSession(void *data);
void handleGet(FILE *logfile);
void handlePut(FILE *logfile);
void handleList(FILE *logfile);

FILE * logfile;

int main(void){

	WSADATA wsadata;

	try{
		if (WSAStartup(0x0202, &wsadata) != 0){
			cout << "Error in starting WSAStartup()\n";
		}
		else{
			buffer = "WSAStartup was successful\n";
			WriteFile(test, buffer, sizeof(buffer), &dwtest, NULL);

			/* display the wsadata structure
			cout<< endl
			<< "wsadata.wVersion "       << wsadata.wVersion       << endl
			<< "wsadata.wHighVersion "   << wsadata.wHighVersion   << endl
			<< "wsadata.szDescription "  << wsadata.szDescription  << endl
			<< "wsadata.szSystemStatus " << wsadata.szSystemStatus << endl
			<< "wsadata.iMaxSockets "    << wsadata.iMaxSockets    << endl
			<< "wsadata.iMaxUdpDg "      << wsadata.iMaxUdpDg      << endl;
			*/
		}

		/*if((s = socket(AF_INET,SOCK_DGRAM,0))==INVALID_SOCKET)
		throw "Socket failed\n";
		memset(&sa,0,sizeof(sa));
		sa.sin_family = AF_INET;
		sa.sin_port = htons(port);
		sa.sin_addr.s_addr = htonl(INADDR_ANY);
		//bind the port to the socket
		if (bind(s,(LPSOCKADDR)&sa,sizeof(sa)) == SOCKET_ERROR)
		throw "can't bind the socket";*/

		//Display info of local host
		gethostname(localhost, 20); //Get computer name: change to localhost
		cout << "ftpd_tcp starting at host: " << localhost << endl;

		if ((hp = gethostbyname(localhost)) == NULL) {
			cout << "gethostbyname() cannot get local host info?"
				<< WSAGetLastError() << endl;
			exit(1);
		}

		//Create the server socket
		if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
			throw "Socket failed\n";

		//Fill-in Server Port and Address info.
		memset(&sa, 0, sizeof(sa));
		sa.sin_family = AF_INET;
		sa.sin_port = htons(port);
		sa.sin_addr.s_addr = htonl(INADDR_ANY);

		//Bind the port to the socket
		if (bind(s, (LPSOCKADDR)&sa, sizeof(sa)) == SOCKET_ERROR)
			throw "can't bind the socket";
		else cout << "Bind was successful" << endl;

		//Timeout of one second
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		//Initiate log
		char *fn = "log.txt";
		logfile = fopen(fn, "w");

		if (TRACE) fprintf(logfile, "Sender: starting on host %s\n", localhost);
		fclose(logfile);

		do {
			char *fn = "log.txt";
			logfile = fopen(fn, "a+");

			//Generate random server number
			memset(&three_way_send, 0, sizeof(three_way_send));
			srand((unsigned)time(NULL));
			three_way_send.server_number = rand() % 255 + 1;
			three_way_send.packet_type = 't';

			cout << "\nWaiting for client number...\n" << endl;

			//First wait for client/sender
			while (true) {
				FD_ZERO(&readfds);
				FD_SET(s, &readfds);  //always check the listener

				outfds = select(s, &readfds, NULL, NULL, &timeout);//timed out, return
				int size = sizeof(sa_in);

				if (outfds > 0) {
					//receive frame
					memset(&three_way_recv, 0, sizeof(three_way_recv));
					memset(&filedir, 0, sizeof(filedir));
					memset(&filename, 0, sizeof(filename));
					ibytesrecv = recvfrom(s, (char*)&three_way_recv, sizeof(three_way_recv), 0, (struct sockaddr*)&sa_in, &size);

					if (three_way_recv.packet_type == 'm') continue;
					cout << "Client number received: " << three_way_recv.client_number << endl;

					three_way_send.client_number = three_way_recv.client_number;
					strcat(filedir, three_way_recv.direction);
					strcat(filename, three_way_recv.file_name);
					break;
				}
			}//wait loop

			

			//Send everything back to client
			cout << "Sending server number: " << three_way_send.server_number << endl;
			cout << "Sending client number back: " << three_way_send.client_number << endl;
			sendto(s, (const char*)&three_way_send, sizeof(three_way_send), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));

			cout << "\nWaiting for second response from client..." << endl;

			int waitingTime = 0;

			//Second wait for client/sender
			while (true) {
				if (waitingTime == 3) break;
				FD_ZERO(&readfds);
				FD_SET(s, &readfds);  //always check the listener

				outfds = select(s, &readfds, NULL, NULL, &timeout);//timed out, return
				int size = sizeof(sa_in);

				if (outfds > 0) {
					//receive frame
					memset(&three_way_recv, 0, sizeof(three_way_recv));
					ibytesrecv = recvfrom(s, (char*)&three_way_recv, sizeof(three_way_recv), 0, (struct sockaddr*)&sa_in, &size);
					cout << "Received server number from client: " << three_way_recv.server_number << endl;

					if (three_way_recv.packet_type == 'm') continue;
					if (three_way_recv.server_number == three_way_send.server_number) break;
				}
				else if (!outfds) {
					cout << "Resend server and client number..." << three_way_send.server_number << endl;
					ibytessent = sendto(s, (const char*)&three_way_send, sizeof(three_way_send), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
				}
				waitingTime++;
			}

			// Sleep for 7 seconds
			Sleep(3);

			if (TRACE) fprintf(logfile, "\nSender: Handshake established\n");
			cout << "Handshake established...\n" << endl;

			cout << filedir << " " << filename << endl;

			/* GET FROM SERVER - GET */
			if (strcmp(filedir, "get") == 0) {
				if (strcmp(filename, "list") == 0) handleList(logfile); // Do get list
				else handleGet(logfile); // Do get with file
			}

			/* PUT FROM CLIENT - PUT */
			else if (strcmp(filedir, "put") == 0) {
				handlePut(logfile);
			}

			/* QUIT SERVER */
			else if (strcmp(filedir, "quit") == 0) {
				cout << "Quitting..." << endl;
				Sleep(3);
				break;
			}

			Sleep(4);

			//Clean for restart
			memset(&three_way_send, 0, sizeof(three_way_send));
			memset(&three_way_recv, 0, sizeof(three_way_recv));

			fclose(logfile);
		} while (true);

	} //try loop
	//Display needed error message.

	catch (char* str) { cerr << str << WSAGetLastError() << endl; }

	//close server socket
	//closesocket(s);

	/* When done uninstall winsock.dll (WSACleanup()) and exit */
	WSACleanup();
	return 0;
}

void handleGet(FILE *logfile) {
	FILE *fp;
	string _filepath = string(filename);
	if (fp = fopen(_filepath.c_str(), "rb")) {
		if (TRACE) fprintf(logfile, "Receiver: GET %s from server\n", filename);
		cout << "FILE EXISTS" << endl;
		cout << "Sending file " << filename << " to client." << endl;

		//Get file length
		fseek(fp, 0, SEEK_END);
		long fileSize = ftell(fp);
		int bufferSize = 80, sequence = 0, totalBytes = 0, totalPackets = 0, totalEBytes = 0;
		rewind(fp);

		cout << "FILE SIZE: " << fileSize << endl;

		//Loop through file buffer
		while (true) {
			// Clean buffer
			memset(&msgfrm, 0, sizeof(msgfrm));

			//Allocate buffer with data from file by size
			fread(msgfrm.data, 1, 80, fp);
			msgfrm.header = (fileSize < 80) ? fileSize : 80;
			msgfrm.snwseq = sequence;
			msgfrm.packet_type = 'm';

			ibytessent = sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
			totalBytes += bufferSize;
			totalPackets++;
			if (TRACE) fprintf(logfile, "Receiver: sent packet %d\n", msgfrm.snwseq);

			while (true) {
				FD_ZERO(&readfds);
				FD_SET(s, &readfds); //always check the listener

				outfds = select(s, &readfds, NULL, NULL, &timeout);//timed out, return
				int size = sizeof(sa_in);

				if (outfds > 0) {
					//receive frame
					memset(&msgrecv, 0, sizeof(msgrecv));
					ibytesrecv = recvfrom(s, (char*)&msgrecv, sizeof(msgrecv), 0, (struct sockaddr*)&sa_in, &size);
				
					if (msgrecv.packet_type == 't') continue;

					if (strcmp(msgrecv.data, "ACK") == 0) {
						sequence = (sequence == 1) ? 0 : 1;
						totalEBytes += msgfrm.header;
						if (TRACE) fprintf(logfile, "Receiver: received %s for packet %d\n", msgrecv.data, msgrecv.snwseq);
						break; //Server received data
					}
				}
				else if (!outfds) {
					totalBytes += msgfrm.header;
					totalPackets++;
					ibytessent = sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
					if (TRACE) fprintf(logfile, "Receiver: packet %d dropped, resent\n", msgfrm.snwseq);
				}
			}

			//End of file
			if (feof(fp)) {
				int numTries = 0;
				memset(&msgfrm, 0, sizeof(msgfrm));
				strcpy(msgfrm.data, "ENDOFFILE");
				msgfrm.packet_type = 'm';

				while (true) {
					if (numTries == 10) break;
					//Inform user that end of file is reached.
					sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
					numTries++;
				}

				cout << "\nEnd-of-File reached." << endl;
				fclose(fp);
				break;
			}

			fileSize -= bufferSize;
		}

		cout << "Total effective bytes sent: " << totalEBytes << endl;
		cout << "Total bytes sent : " << totalBytes << endl;
		cout << "Total packets sent: " << totalPackets << endl;

		if (TRACE) {
			fprintf(logfile, "Sender: file transfer completed\n");
			fprintf(logfile, "Sender: effective bytes sent: %d\n", totalEBytes);
			fprintf(logfile, "Sender: number of packets sent %d\n", totalPackets);
			fprintf(logfile, "Sender: number of bytes sent %d\n", totalBytes);
		}
	}
	else {
		int numTries = 0;
		memset(&msgfrm, 0, sizeof(msgfrm));
		strcpy(msgfrm.data, "NOFILEEXISTS");
		msgfrm.packet_type = 'm';

		while (true) {
			if (numTries == 10) break;
			//Inform user that end of file is reached.
			sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
			numTries++;
		}

		//File doesn't exists
		cout << "\nFILE DOESN'T EXIST" << endl;
	}
}

void handlePut(FILE *logfile) {
	if (TRACE) fprintf(logfile, "Receiver: PUT %s to server\n", filename);
	cout << "Retrieving file " << filename << " from client, waiting...\n" << endl;

	string _filepath = string(filename); // Local client folder
	FILE *fp = fopen(_filepath.c_str(), "wb+"); // Create file
	int sequence = 0, totalBytes = 0, totalPackets = 0;

	while (true) {
		
		FD_ZERO(&readfds);
		FD_SET(s, &readfds); //always check the listener

		outfds = select(s, &readfds, NULL, NULL, &timeout);//timed out, return
		int size = sizeof(sa_in);

		if (outfds > 0) {
			//receive frame
			memset(&msgrecv, 0, sizeof(msgrecv));
			recvfrom(s, (char*)&msgrecv, sizeof(msgrecv), 0, (struct sockaddr*)&sa_in, &size);

			if (msgrecv.packet_type == 't') continue;

			if (strcmp(msgrecv.data, "ENDOFFILE") == 0) {
				fclose(fp);
				break;

			}
			else if (strcmp(msgrecv.data, "NOFILEEXISTS") == 0) {
				fclose(fp);
				if (remove(_filepath.c_str()) == 0) cout << "FILE DOESN'T EXIST" << endl;

				break;
			}

			//cout << msgrecv.data;

			//What happens when ACK is lost
			if (msgrecv.snwseq == sequence) {
				if (TRACE) fprintf(logfile, "Receiver: received packet %d\n", msgrecv.snwseq);

				//Write buffer into current file
				fwrite(msgrecv.data, 1, msgrecv.header, fp);
				sequence = (sequence == 1) ? 0 : 1;
				totalBytes += msgrecv.header;
				totalPackets++;

				//Clean buffer
				memset(&msgfrm, 0, sizeof(msgfrm));

				//Set and send ACK to client
				strcat(msgfrm.data, "ACK");
				msgfrm.snwseq = sequence;
				
				sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
				if (TRACE) fprintf(logfile, "Receiver: sent %s for packet %d\n", msgfrm.data, msgrecv.snwseq);
			}
			else {
				//Resend ACK if unwanted packet is received
				sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
				if (TRACE) fprintf(logfile, "Receiver: resent %s for packet %d\n", msgfrm.data, msgrecv.snwseq);
			}
		}
	}

	cout << "\nEnd-of-file reached." << endl;
	cout << "Total bytes received: " << totalBytes << endl;
	cout << "Total packets received: " << totalPackets << endl;
	if (TRACE) {
		fprintf(logfile, "Receiver: file transfer completed\n");
		fprintf(logfile, "Receiver: number of packets received %d\n", totalPackets);
		fprintf(logfile, "Receiver: number of bytes received %d\n", totalBytes);
	}
}

void handleList(FILE *logfile) {
	cout << "Client requested to display all the files." << endl;
	int sequence = 0;

	WIN32_FIND_DATA searchFile;
	memset(&searchFile, 0, sizeof(WIN32_FIND_DATA));
	
	LPCSTR filepath = "G:\\comp445\\Assignment2\\Server\\*";
	HANDLE handle = FindFirstFile(filepath, &searchFile);

	while (handle != INVALID_HANDLE_VALUE) {
		//Look for end of string name
		int bufferLength = 0;
		for (int fileLen = 0; fileLen < 260; fileLen++)
		{
			if (searchFile.cFileName[fileLen] == '\0') {
				bufferLength = fileLen + 1;
				break;
			}
		}

		_bstr_t b(searchFile.cFileName);
		const char* c = b;
		cout << c << endl;

		memset(&msgfrm, 0, sizeof(msgfrm));
		strcat(msgfrm.data, c);
		msgfrm.snwseq = sequence;
		msgfrm.packet_type = 'm';
		sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));

		while (true) {
			FD_ZERO(&readfds);
			FD_SET(s, &readfds);  //always check the listener

			outfds = select(s, &readfds, NULL, NULL, &timeout);//timed out, return
			int size = sizeof(sa_in);

			if (outfds > 0) {
				//receive frame
				memset(&msgrecv, 0, sizeof(msgrecv));
				ibytesrecv = recvfrom(s, (char*)&msgrecv, sizeof(msgrecv), 0, (struct sockaddr*)&sa_in, &size);

				if (msgrecv.packet_type == 't') continue;

				if (strcmp(msgrecv.data, "ACK") == 0) {
					sequence = (sequence == 1) ? 0 : 1;
					break;
				}
			}
			else if (!outfds) {
				sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
			}
		}

		if (!FindNextFile(handle, &searchFile)) {
			while (true) {
				int numTries = 0;
				memset(&msgfrm, 0, sizeof(msgfrm));
				strcpy(msgfrm.data, "ENDOFLIST");
				msgfrm.packet_type = 'm';

				while (true) {
					if (numTries == 10) break;
					// Inform user that end of file is reached.
					sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
					numTries++;
				}

				cout << "\nEnd-of-List reached." << endl;
				break;
			}
			break;
		}
	}




}
