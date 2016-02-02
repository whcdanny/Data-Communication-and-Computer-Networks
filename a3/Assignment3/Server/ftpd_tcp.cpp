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
#define MAX_BUFFER_SIZE 256

//socket data types
SOCKET s;
SOCKADDR_IN sa;        // port 5001, server IP address
SOCKADDR_IN sa_in;     // router info, IP, port(7001)

//packet structure definition
#define STOPNWAIT 1 //Bits width For stop and wait
struct MESSAGE_FRAME {
	unsigned int header;
	unsigned int snwseq;
	int sequence;
	char packet_type; //t:three_way; m:message;
	char data[MAX_BUFFER_SIZE];//or error message
} msgfrm, msgrecv;

//three way handshake definition
struct THREE_WAY_HS{
	int client_number;
	int server_number;
	int window_size;
	char packet_type; //t:three_way; m:message;
	char direction[10];
	char file_name[1000];
	char file_rename[1000];
} three_way_send, three_way_recv;

union {
	struct sockaddr generic;
	struct sockaddr_in ca_in;
}ca;

int calen = sizeof(ca);

//buffer data types
char clientbuffer[128];
char filedir[10];
char filename[2000];
char filerename[2000];
char *buffer;
int windowSize = 0;
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

boolean fileExists(char namefile[]);

unsigned __stdcall ClientSession(void *data);
void handleGet(FILE *logfile);
void handlePut(FILE *logfile);
void handleList(FILE *logfile);
void handleRename(FILE *logfile);

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
		//Get computer name: change to localhost
		gethostname(localhost, 20); 
		cout << "ftpd_tcp starting at host: " << localhost << endl;

		if ((hp = gethostbyname(localhost)) == NULL) {
			cout << "gethostbyname() cannot get local host info?"
				<< WSAGetLastError() << endl;
			exit(1);
		}

		//Create the server socket
		if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
			throw "Socket failed\n";

		//Create Server Port and Address info.
		memset(&sa, 0, sizeof(sa));
		sa.sin_family = AF_INET;
		sa.sin_port = htons(port);
		sa.sin_addr.s_addr = htonl(INADDR_ANY);
		
		//Bind the port to the socket
		if (bind(s, (LPSOCKADDR)&sa, sizeof(sa)) == SOCKET_ERROR) {
			throw "can't bind the socket";
			system("pause");}
		else cout << "Bind was successful" << endl;

		//Timeout of one second
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		//Initiate log.txt
		char *fn = "log.txt";
		logfile = fopen(fn, "w");

		if (TRACE) fprintf(logfile, "Sender: starting on host %s\n", localhost);
		fclose(logfile);

		do {
			char *fn = "log.txt";
			logfile = fopen(fn, "a+");

			//Create random server number
			memset(&three_way_send, 0, sizeof(three_way_send));
			srand((unsigned)time(NULL));
			three_way_send.server_number = rand() % 255 + 1;
			three_way_send.packet_type = 't';

			cout << "\nWaiting for client number...\n" << endl;

			//First wait for client/sender
			while (true) {
				FD_ZERO(&readfds);
				FD_SET(s, &readfds); //always check the listener

				outfds = select(s, &readfds, NULL, NULL, &timeout);//timed out, return
				int size = sizeof(sa_in);

				if (outfds > 0) {
					//receive frame
					memset(&three_way_recv, 0, sizeof(three_way_recv));
					memset(&filedir, 0, sizeof(filedir));
					memset(&filename, 0, sizeof(filename));
					ibytesrecv = recvfrom(s, (char*)&three_way_recv, sizeof(three_way_recv), 0, (struct sockaddr*)&sa_in, &size);

					if (three_way_recv.packet_type == 't') {
						cout << "Client number received: " << three_way_recv.client_number << endl;

						three_way_send.client_number = three_way_recv.client_number;
						strcpy(filedir, three_way_recv.direction);
						strcpy(filename, three_way_recv.file_name);
						strcpy(filerename, three_way_recv.file_rename);
						windowSize = three_way_recv.window_size;
						break;
					}
					else if (three_way_recv.packet_type == 'm' || three_way_recv.packet_type == 'r') {
						continue;
					}
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
				FD_SET(s, &readfds); //always check the listener

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
			//Sleep for 7 seconds
			Sleep(3);

			if (TRACE) fprintf(logfile, "\nSender: Handshake established\n");
			cout << "Handshake established...\n" << endl;

			cout << filedir << " " << filename << endl;

			/* GET FROM SERVER - GET */
			if (strcmp(filedir, "get") == 0) {
				if (strcmp(filename, "list") == 0) handleList(logfile); //Do get list
				else handleGet(logfile); //Do get with file
			}
			/* PUT FROM CLIENT - PUT */
			else if (strcmp(filedir, "put") == 0) {
				handlePut(logfile);
			}
			/* RENAME FROM CLIENT - RENAME */
			else if (strcmp(filedir, "rename") == 0) {
				handleRename(logfile);
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
	cout << "Waiting for confirmation from user..." << endl;
	//First Wait for confirmation from user and send file
	while(true) {
		FD_ZERO(&readfds);
		FD_SET(s, &readfds);

		outfds = select(s, &readfds, NULL, NULL, &timeout);//timed out, return
		int size = sizeof(sa_in);

		if (outfds > 0) {
			memset(&msgrecv, 0, sizeof(msgrecv));
			recvfrom(s, (char*)&msgrecv, sizeof(msgrecv), 0, (struct sockaddr*)&sa_in, &size);

			if(strcmp(msgrecv.data, "SENDFILE") == 0) {
				cout << "Initiate sending file to user..." << endl;
				break;
			}
			else if(strcmp(msgrecv.data,"n") == 0 || strcmp(msgrecv.data,"N") == 0) {
				cout << "File transfer cancelled..." << endl;
				return;
			}
		}
	}

	if (TRACE) fprintf(logfile, "Sender: GET %s from server\n", filename);
	cout << "Sending file " << filename << " to client, waiting...\n" << endl;

	FILE *fp;
	string _filepath = string(filename);

	if (fp = fopen(_filepath.c_str(), "rb")) {
		//Packet info
		int totalBytes = 0,
			totalPackets = 0,
			totalEffBytes = 0,
			currentBase = 0,
			waitingSeq = 0,
			bufferSize = 0;

		//Get the file length and the total frames
		fseek(fp, 0, SEEK_END);
		long fileSize = ftell(fp);
		rewind(fp);
		int totalFrames = (int)ceil((float)fileSize / MAX_BUFFER_SIZE);

		while (true) {
			//Send packets in window size limit
			for (int i = 0; i < windowSize && (currentBase + i) < totalFrames; i++) {
				memset(&msgfrm, 0, sizeof(msgfrm));
				bufferSize = fileSize - ((currentBase + i) * MAX_BUFFER_SIZE);
				msgfrm.header = (bufferSize < MAX_BUFFER_SIZE) ? bufferSize : MAX_BUFFER_SIZE;
				//Gets larger
				msgfrm.sequence = currentBase + i; 
				msgfrm.packet_type = 'm';
				fread(msgfrm.data, 1, MAX_BUFFER_SIZE, fp);
				totalBytes += MAX_BUFFER_SIZE;
				totalPackets++;

				sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
			}
			printf("Packets %d - %d sent.\n", currentBase, currentBase+windowSize-1);
			if (TRACE) fprintf(logfile, "Receiver: Packets %d - %d sent.\n", currentBase, currentBase+windowSize-1);

			//Wait for ACKS of 4 sent packets
			bool isNak = false;
			int seqItr = 0;
			while(true) {
				FD_ZERO(&readfds);
				FD_SET(s, &readfds);

				outfds = select(s, &readfds, NULL, NULL, &timeout);//timed out, return
				int size = sizeof(sa_in);

				if (outfds > 0) {
					memset(&msgrecv, 0, sizeof(msgrecv));
					recvfrom(s, (char*)&msgrecv, sizeof(msgrecv), 0, (struct sockaddr*)&sa_in, &size);

					//Correct packet sent
					cout << "MSG: " << msgrecv.data << " SEQ: " << msgrecv.sequence << " TYPE: " << msgrecv.packet_type << endl;
					if (msgrecv.packet_type == 'm') {
						if (strcmp(msgrecv.data, "ACK") == 0) {
							//Comparison to currentBase and ignore delayed packets
							if (msgrecv.sequence > currentBase) {
								//Assign base for next packet
								currentBase = msgrecv.sequence; 
								cout << "ACK: " << currentBase << endl;
							}
							totalEffBytes += MAX_BUFFER_SIZE;
							if (TRACE) fprintf(logfile, "Receiver: Packet ACK'd - %d\n", msgrecv.sequence); 
						}
						//Dropped or delayed packet
						else if (strcmp(msgrecv.data, "NAK") == 0) {
							isNak = true;
							currentBase = msgrecv.sequence;
							//Recalculate file pointer position
							fseek(fp, currentBase * MAX_BUFFER_SIZE, SEEK_SET); 
							if (TRACE) fprintf(logfile, "Receiver: Packet NAK'd - %d\n", msgrecv.sequence); 
							break;
						}

						seqItr++;
					} 
				}

				if(seqItr == windowSize) break;
			}

			// End of file
			if (currentBase == totalFrames) {
				memset(&msgfrm, 0, sizeof(msgfrm));
				msgfrm.packet_type = 'm';
				strcat(msgfrm.data, "ENDOFFILE");

				//End of file reached
				for (int k = 0; k < windowSize; k++)
					sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));

				cout << "End of file reached." << endl;
				fclose(fp);
				break;
			}
		}

		cout << "Total effective bytes sent: " << totalEffBytes << endl;
		cout << "Total bytes sent : " << totalBytes << endl;
		cout << "Total packets sent: " << totalPackets << endl;

		if (TRACE) {
			fprintf(logfile, "Sender: file transfer completed\n");
			fprintf(logfile, "Sender: effective bytes sent: %d\n", totalEffBytes);
			fprintf(logfile, "Sender: number of packets sent %d\n", totalPackets);
			fprintf(logfile, "Sender: number of bytes sent %d\n", totalBytes);
		}
	}
	else {
		memset(&msgfrm, 0, sizeof(msgfrm));
		strcpy(msgfrm.data, "NOFILEEXISTS");
		msgfrm.packet_type = 'm';

		for (int numTries = 0; numTries < 10; numTries++) {
			//Talk to user that end of file is reached.
			sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
		}

		//File doesn't exists		
		cout << "\nFILE DOESN'T EXIST" << endl;
	}
}

void handlePut(FILE *logfile) {
	//First checking if the file exists
	if(fileExists(filename)) {
		memset(&msgfrm, 0, sizeof(msgfrm));
		strcpy(msgfrm.data, "RENAMEFILE");
		msgfrm.packet_type = 'r';

		cout << "File already exists." << endl;
		sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));

		while(true) {
			FD_ZERO(&readfds);
			FD_SET(s, &readfds);

			outfds = select(s, &readfds, NULL, NULL, &timeout); //timed out, return
			int size = sizeof(sa_in);

			if (outfds > 0) {
				memset(&msgrecv, 0, sizeof(msgrecv));
				recvfrom(s, (char*)&msgrecv, sizeof(msgrecv), 0, (struct sockaddr*)&sa_in, &size);

				if (msgrecv.packet_type == 't') continue;
				if (msgrecv.packet_type == 'r') {
					if(strcmp(msgrecv.data,"n") == 0 || strcmp(msgrecv.data,"N") == 0) {
						cout << "User has cancelled file transfer." << endl;
						return;
					} 
					else if(fileExists(msgrecv.data)) {
						if (TRACE) fprintf(logfile, "Receiver: Filename exists\n", msgrecv.sequence);
						cout << "File already exists." << endl;
						sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
					} 
					else {
						cout << "File good" << endl;
						break;
					}
				}
			} 
		}

		//Replace filename being used to sender choice
		memset(&filename, 0, sizeof(filename));
		strcpy(filename, msgrecv.data);
	}

	//File name is right then start transfer
	cout << "Initialise file transfer..." << endl;
	memset(&msgfrm, 0, sizeof(msgfrm));
	strcpy(msgfrm.data, "SENDFILE");
	msgfrm.packet_type = 'r';
	sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));

	if (TRACE) fprintf(logfile, "Receiver: PUT %s to server\n", filename);
	cout << "Retrieving file " << filename << " from client, waiting...\n" << endl;
	
	//The local client folder
	string _filepath = string(filename); 
	//Create a file
	FILE *fp = fopen(_filepath.c_str(), "wb+"); 

	//Packet info
	int totalBytes = 0, 
		totalPackets = 0, 
		waitingSeq = 0;

	//Take incoming packets
	while (true) {
		FD_ZERO(&readfds);
		FD_SET(s, &readfds);

		outfds = select(s, &readfds, NULL, NULL, &timeout); //timed out, return
		int size = sizeof(sa_in);

		if (outfds > 0) {
			memset(&msgrecv, 0, sizeof(msgrecv));
			recvfrom(s, (char*)&msgrecv, sizeof(msgrecv), 0, (struct sockaddr*)&sa_in, &size);

			if (msgrecv.packet_type == 't') continue;

			if (msgrecv.packet_type == 'r') {
				//Talk user start sending the file
				cout << "Packet most likely not received. " << endl;
				memset(&msgfrm, 0, sizeof(msgfrm));
				strcpy(msgfrm.data, "SENDFILE");
				msgfrm.packet_type = 'r';
				sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
			}
			else if (msgrecv.packet_type == 'm') {
				//End of the file reach
				if (strcmp(msgrecv.data, "ENDOFFILE") == 0) {
					fclose(fp);
					break;
				}
				else if (strcmp(msgrecv.data, "NOFILEEXISTS") == 0) {
					if (remove(_filepath.c_str()) == 0) cout << "FILE DOESN'T EXIST" << endl;
					break;
				}

				//It is right then update sending frame
				if (msgrecv.sequence == waitingSeq) {
					if (TRACE) fprintf(logfile, "Receiver: Sequence %d accepted\n", msgrecv.sequence);
					cout << "Sequence " << msgrecv.sequence << " written to file." << endl;
					//Write file
					fwrite(msgrecv.data, 1, msgrecv.header, fp);
					memset(&msgfrm, 0, sizeof(msgfrm));
					strcpy(msgfrm.data, "ACK");
					msgfrm.packet_type = 'm';
					msgfrm.sequence = msgrecv.sequence;
					waitingSeq++; //Next sequence
					totalPackets++;
					totalBytes += msgrecv.header;
				} 
				else {
					if (TRACE) fprintf(logfile, "Receiver: Sequence %d rejected\n", msgrecv.sequence);
					//It is not wanted packet
					memset(&msgfrm, 0, sizeof(msgfrm));
					strcpy(msgfrm.data, "NAK");
					msgfrm.packet_type = 'm';
					//Let the sequence is waited sequence
					msgfrm.sequence = waitingSeq; 
				}

				//Send ACK or NAK packets
				sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
			}
		}
		else { 
			//Send SENDFILE again if nothing is recieved
			//sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
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
	//Change to school directory or add Downloads folder
	
	// = "C:\\Users\\haochen\\Documents\\Visual Studio 2012\\Projects\\Assignment3\\Server\\*";
	LPCSTR filepath = "G:\\comp445\\Assignment3\\Server\\*";
	HANDLE handle = FindFirstFile(filepath, &searchFile);

	while (handle != INVALID_HANDLE_VALUE) {
		//Look for the string name
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

				if(msgrecv.packet_type == 't') continue;

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
			while(true) {
				int numTries = 0;
				memset(&msgfrm, 0, sizeof(msgfrm));
				strcpy(msgfrm.data, "ENDOFLIST");
				msgfrm.packet_type = 'm';

				while (true) {
					if (numTries == 10) break;
					//Talk to user that end of file is reached.
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

void handleRename(FILE *logfile) {
	if (TRACE) fprintf(logfile, "Renaming file %s to %s.", filename, filerename);

	memset(&msgfrm, 0, sizeof(msgfrm));
	msgfrm.packet_type = 'm';

	if(rename(filename, filerename) == 0) {
		cout << "File " << filename << " successfully renamed to " << filerename << "." << endl;
		strcpy(msgfrm.data, "FILERENAMED");
	} else {
		cout << "File renaming encountered an error." << endl;
		strcpy(msgfrm.data, "FILEERROR");
	}

	int numTries = 0;
	while (true) {
		if (numTries == 10) break;
		//Talk to user that end of file is reached.
		sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
		numTries++;
	}
}

boolean fileExists(char namefile[]) {
	if(FILE *fe = fopen(string(namefile).c_str(), "r")) {
		fclose(fe);
		return true;
	} else {
		return false;
	}
}