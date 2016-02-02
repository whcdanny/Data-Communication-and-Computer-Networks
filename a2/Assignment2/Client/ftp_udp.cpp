// CLIENT TCP PROGRAM
// Revised and tidied up by
// J.W. Atwood
// 1999 June 30
//


char* getmessage(char *);

/* send and receive codes between client and server */
/* This is your basic WINSOCK shell */
#pragma comment( linker, "/defaultlib:ws2_32.lib" )
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include <winsock.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <string>
#include <windows.h>
#include <fstream>
#include <ctype.h>
#include <locale>
#include <time.h>

using namespace std;

//user defined port number
#define REQUEST_PORT 5000;
int port = REQUEST_PORT;

//socket data types
SOCKET s;
SOCKADDR_IN sa;         // filled by bind
SOCKADDR_IN sa_in;      // fill with server info, IP, port

//wait variables
int nsa1;
int r, infds = 1, outfds = 0;
struct timeval timeout;
const struct timeval *tp = &timeout;

fd_set readfds;

#define TRACE 1
#define UTIMER 300000;
#define STIMER 0;
struct timeval timeouts;
//timeouts.tv_sec=STIMER;
//timeouts.tv_usec=UTIMER;

//packet structure definition
#define STOPNWAIT 1 //Bits width For stop and wait
struct MESSAGE_FRAME {
	unsigned int header;
	unsigned int snwseq:STOPNWAIT;
	char packet_type;
	char data[80];//or error message
} msgfrm, msgrecv, mfr;

//three way handshake definition
struct THREE_WAY_HS{
	int client_number;
	int server_number;
	char packet_type;
	char direction[5];
	char file_name[2000];
} three_way_send, three_way_recv, three_way;

//buffer data types
char szbuffer[128];
char *buffer;

int ibufferlen = 0;
int ibytessent;
int ibytesrecv = 0;

//host data types
HOSTENT *hp;
HOSTENT *rp;

char remotehost[11];

//other
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

boolean isFileValid(char namefile[]);
boolean isDirectionValid(char directiontype[]);
boolean isStringEmpty(char stringChars[]);
string removeWhiteSpaces(char paramText[]);
string lowerCaseString(char stringChars[]);
//void initiateFileHandler(FILE *logfile);
void handleGet(FILE *logfile);
void handlePut(FILE *logfile);
void handleList(FILE *logfile);

FILE * logfile;

int main(void){

	WSADATA wsadata;

	try {

		if (WSAStartup(0x0202, &wsadata) != 0){
			cout << "Error in starting WSAStartup()" << endl;
		}
		else {
			buffer = "WSAStartup was successful\n";
			WriteFile(test, buffer, sizeof(buffer), &dwtest, NULL);
		}

		if((s = socket(AF_INET,SOCK_DGRAM,0))==INVALID_SOCKET) 
				  throw "Socket failed\n";

		memset(&sa,0,sizeof(sa));
		sa.sin_family = AF_INET;
		sa.sin_port = htons(port);
		sa.sin_addr.s_addr = htonl(INADDR_ANY); //host to network

		//Bind the port to the socket
		if (bind(s,(LPSOCKADDR)&sa,sizeof(sa)) == SOCKET_ERROR)
				throw "can't bind the socket";

		//Ask for name of remote server
		cout << "Type name of ftp server: " << flush;
		cin >> remotehost;

		//If user types quit in any case end session
		string quitCommand = lowerCaseString(remotehost);
		if (quitCommand.compare("quit") == 0) {
			cout << "Quitting..." << endl;
			closesocket(s);
			Sleep(3000);
			exit(0);
		}

		if ((rp = gethostbyname(remotehost)) == NULL)
			throw "remote gethostbyname failed\n";

		memset(&sa_in,0,sizeof(sa_in));
		memcpy(&sa_in.sin_addr, rp->h_addr, rp->h_length);
		sa_in.sin_family = rp->h_addrtype;   
		sa_in.sin_port = htons(7000);

		//Timeout of one second
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		//Initiate log
		char *fn = "log.txt";
		logfile = fopen(fn, "w");

		if (TRACE) fprintf(logfile, "Sender: starting on host %s\n", remotehost);
		fclose(logfile);

		do {
			char *fn = "log.txt";
			logfile = fopen(fn, "a+");

			memset(&three_way_send, 0, sizeof(three_way_send));

			//Ask for direction type: get, put, list
			cout << "\nType direction of transfer \n(get, put, quit): " << flush;
			cin >> three_way_send.direction;

			// Validate namefile
			while (!isDirectionValid(three_way_send.direction) || string(three_way_send.direction).empty()) {
				cout << "Available directions: \n" <<
					"\t get  - get file from server\n" <<
					"\t put  - put file to server\n" <<
					"\t quit - quit session\n" << endl;
				cout << "Type direction of transfer: " << flush;
				cin >> three_way_send.direction;
			}

			if (strcmp(three_way_send.direction, "quit") != 0) {
				//Ask for file to be put to/get from server
				cout << "\nType name of file to be transferred \n(type 'list' to display files from server): " << flush;
				cin >> three_way_send.file_name;

				//Validate namefile
				while (!isFileValid(three_way_send.file_name) || string(three_way_send.file_name).empty()) {
					cout << "\nFile names should not contain: \\/:*?\"<>| or any blank spaces." << endl;
					cout << "Type name of file to be transferred: " << flush;
					cin >> three_way_send.file_name;
				}
			}

			cout << "Initiating handshake..." << endl;

			srand((unsigned)time(NULL));


			//rendom number for client number
			three_way_send.client_number = rand() % 255 + 1;
			three_way_send.packet_type = 't';

			cout << "Sending random number to server: " << three_way_send.client_number << endl;
			ibytessent = sendto(s, (const char*)&three_way_send, sizeof(three_way_send), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));

			/* Initiate three way handshake */
			//First wait for server/receiver 
			while (true) {

				FD_ZERO(&readfds);
				FD_SET(s, &readfds);  //always check the listener

				outfds = select(s, &readfds, NULL, NULL, &timeout); //timed out, return
				int size = sizeof(sa_in);

				if (outfds > 0) {
					//receive frame
					
					memset(&three_way_recv, 0, sizeof(three_way_recv));
					ibytesrecv = recvfrom(s, (char*)&three_way_recv, sizeof(three_way_recv), 0, (struct sockaddr*)&sa_in, &size);
					
					if(three_way_recv.packet_type == 'm') continue;
					cout << "\nServer random number received: " << three_way_recv.server_number << endl;
					cout << "Client random number included: " << three_way_recv.client_number << endl;

					if (three_way_recv.client_number == three_way_send.client_number) {
						ibytessent = sendto(s, (const char*)&three_way_recv, sizeof(three_way_recv), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
						break;
					}
				}
				else if (!outfds) {
					cout << "Resending client random number to server..." << endl;
					ibytessent = sendto(s, (const char*)&three_way_send, sizeof(three_way_send), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
				}
			}

			cout << "\nWaiting for final response from server..." << endl;
			int waitingTime = 0;

			// Second wait for server/receiver
			while (true) {
				if (waitingTime == 3) break;

				
				FD_ZERO(&readfds);
				FD_SET(s, &readfds); //always check the listener

				//Send ACK2 to server/receiver
				cout << "Resending number back to server..." << endl;
				sendto(s, (const char*)&three_way_recv, sizeof(three_way_recv), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));

				waitingTime++;
			}
			// Sleep for 7 seconds
			Sleep(3);
			
			
			/* End three way handshake */
			if (TRACE) fprintf(logfile, "\nSender: Handshake established\n");
			cout << "Handshake established...\n" << endl;

			/* GET FROM SERVER - GET */
			if (strcmp(three_way_send.direction, "get") == 0) {
				if (strcmp(three_way_send.file_name, "list") == 0) handleList(logfile); //Do get list
				else handleGet(logfile); // Do get with file
			}
			/* PUT TO SERVER - PUT */
			else if (strcmp(three_way_send.direction, "put") == 0) {
				handlePut(logfile);
			}
			/* LIST FILES TO CLIENT - LIST */
			else if (strcmp(three_way_send.direction, "list") == 0) {
				handleList(logfile);

			}
			/* QUIT CLIENT */
			else if (strcmp(three_way_send.direction, "quit") == 0) {
				cout << "Quitting..." << endl;
				Sleep(3);
				break;
			}

			fclose(logfile);
		} while (true);
		
	} 

	//Display any needed error response.

	catch (char *str) { cerr << str << ":" << dec << WSAGetLastError() << endl; }

	//close the client socket

	/* When done uninstall winsock.dll (WSACleanup()) and exit */
	WSACleanup();

	return 0;
}

void handleGet(FILE *logfile) {
	if (TRACE) fprintf(logfile, "Sender: GET %s from server\n", three_way_send.file_name);
	cout << "Retrieving file " << three_way_send.file_name << " from " << remotehost << ", waiting...\n" << endl;

	string _filepath = string(three_way_send.file_name); // Local client folder
	FILE *fp = fopen(_filepath.c_str(), "wb+"); // Create file
	int sequence = 0, totalBytes = 0, totalPackets = 0;

	while (true) {
		
		FD_ZERO(&readfds);
		FD_SET(s, &readfds);  //always check the listener

		outfds = select(s, &readfds, NULL, NULL, &timeout); //timed out, return
		int size = sizeof(sa_in);

		if (outfds > 0) {
			//receive frame
			memset(&msgrecv, 0, sizeof(msgrecv));
			recvfrom(s, (char*)&msgrecv, sizeof(msgrecv), 0, (struct sockaddr*)&sa_in, &size);
			
			if(msgrecv.packet_type == 't') continue;

			if (strcmp(msgrecv.data, "ENDOFFILE") == 0) {
				fclose(fp);
				break;

			} else if(strcmp(msgrecv.data, "NOFILEEXISTS") == 0) {
				fclose(fp);
				if (remove(_filepath.c_str()) == 0) cout << "FILE DOESN'T EXIST" << endl;

				break;
			}

			//cout << msgrecv.data;

			//What happens when ACK is lost
			if (msgrecv.snwseq == sequence) {
				if (TRACE) fprintf(logfile, "Sender: received packet %d\n", msgrecv.snwseq);

				// Write buffer into current file
				fwrite(msgrecv.data, 1, msgrecv.header, fp);
				sequence = (sequence == 1) ? 0 : 1;
				totalBytes += msgrecv.header;
				totalPackets++;

				// Clean buffer
				memset(&msgfrm, 0, sizeof(msgfrm));

				//Set and send ACK to server
				strcat(msgfrm.data, "ACK");
				msgfrm.snwseq = sequence;
				
				sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
				if (TRACE) fprintf(logfile, "Sender: sent %s for packet %d\n", msgfrm.data, msgrecv.snwseq);
			}
			else {
				//Resend ACK if unwanted packet is received
				
				sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
				if (TRACE) fprintf(logfile, "Sender: resent %s for packet %d\n", msgfrm.data, msgrecv.snwseq);
			}
		}
	}

	cout << "\nEnd-of-file reached." << endl;
	cout << "Total bytes received: " << totalBytes << endl;
	cout << "Total packets received: " << totalPackets << endl;
	if (TRACE) {
		fprintf(logfile, "Sender: file transfer completed\n");
		fprintf(logfile, "Sender: number of packets received %d\n", totalPackets);
		fprintf(logfile, "Sender: number of bytes received %d\n", totalBytes);
	}
}

void handlePut(FILE *logfile) {
	FILE *fp;
	string _filepath = string(three_way_send.file_name);
	if (fp = fopen(_filepath.c_str(), "rb")) {
		if (TRACE) fprintf(logfile, "Sender: PUT %s to server\n", three_way_send.file_name);
		cout << "FILE EXISTS" << endl;
		cout << "Sending file " << three_way_send.file_name << " to client." << endl;

		// Get file length
		fseek(fp, 0, SEEK_END);
		long fileSize = ftell(fp);
		int bufferSize = 80, sequence = 0, totalBytes = 0, totalPackets = 0, totalEBytes = 0;
		rewind(fp);

		cout << "FILE SIZE: " << fileSize << endl;

		// Loop through file buffer
		while (true) {
			// Clean buffer
			memset(&msgfrm, 0, sizeof(msgfrm));

			// Allocate buffer with data from file by size
			fread(msgfrm.data, 1, 80, fp);
			msgfrm.header = (fileSize < 80) ? fileSize : 80;
			msgfrm.snwseq = sequence;
			msgfrm.packet_type = 'm';

			ibytessent = sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
			totalBytes += bufferSize;
			totalPackets++;
			if (TRACE) fprintf(logfile, "Sender: sent packet %d\n", msgfrm.snwseq);

			while (true) {
				FD_ZERO(&readfds);
				FD_SET(s, &readfds); //always check the listener

				outfds = select(s, &readfds, NULL, NULL, &timeout);//timed out, return
				int size = sizeof(sa_in);

				if (outfds > 0) {
					//receive frame
					memset(&msgrecv, 0, sizeof(msgrecv));
					recvfrom(s, (char*)&msgrecv, sizeof(msgrecv), 0, (struct sockaddr*)&sa_in, &size);

					if(msgrecv.packet_type == 't') continue;

					if (strcmp(msgrecv.data, "ACK") == 0) {
						sequence = (sequence == 1) ? 0 : 1;
						totalEBytes += bufferSize;
						if (TRACE) fprintf(logfile, "Sender: received %s for packet %d\n", msgrecv.data, msgrecv.snwseq);
						break; //Client received data
					}
				}
				else if (!outfds) {				
					totalBytes += bufferSize;
					totalPackets++;
					ibytessent = sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
					if (TRACE) fprintf(logfile, "Sender: packet %d dropped, resent\n", msgfrm.snwseq);
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
					// Inform user that end of file is reached.
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
			// Inform user that end of file is reached.
			sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
			numTries++;
		}

		// File doesn't exists		
		cout << "\nFILE DOESN'T EXIST" << endl;
	}
}

void handleList(FILE *logfile) {
	int sequence = 0;
	while (true) {
		FD_ZERO(&readfds);
		FD_SET(s, &readfds); //always check the listener

		outfds = select(s, &readfds, NULL, NULL, &timeout);//timed out, return
		int size = sizeof(sa_in);

		if (outfds > 0) {
			//receive frame
			memset(&msgrecv, 0, sizeof(msgrecv));
			ibytesrecv = recvfrom(s, (char*)&msgrecv, sizeof(msgrecv), 0, (struct sockaddr*)&sa_in, &size);

			if(msgrecv.packet_type == 't') continue;
			if (strcmp(msgrecv.data, "ENDOFLIST") == 0) break;

			//What happens when ACK is lost
			if (msgrecv.snwseq == sequence) {
				if (TRACE) fprintf(logfile, "Sender: received packet %d\n", msgrecv.snwseq);
				// << msgrecv.data << endl;
				//Write buffer into current file
				sequence = (sequence == 1) ? 0 : 1;

				//Clean buffer
				memset(&msgfrm, 0, sizeof(msgfrm));
				strcat(msgfrm.data, "ACK");
				msgfrm.snwseq = sequence;
				sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
				if (TRACE) fprintf(logfile, "Sender: sent %s for packet %d\n", msgfrm.data, msgrecv.snwseq);
			}
			else {
				//Resend ACK if unwanted packet is received
				sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
				if (TRACE) fprintf(logfile, "Sender: resent %s for packet %d\n", msgfrm.data, msgrecv.snwseq);
			}
		}
	}
}

boolean isFileValid(char namefile[]) {
	char keys[] = "\\/:*?\"<>|";

	// False: Not valid, True: Valid
	return (strpbrk(namefile, keys) != NULL) ? false : true;
}

boolean isDirectionValid(char directiontype[]) {
	string strdirection = lowerCaseString(directiontype);

	if (strdirection.compare("get") == 0 ||
		strdirection.compare("put") == 0 ||
		strdirection.compare("list") == 0 ||
		strdirection.compare("quit") == 0 ||
		strdirection.compare("delete") == 0) {
		return true; // Valid
	}
	else {
		return false; // Not valid
	}
}

string lowerCaseString(char stringChars[]) {
	string newString;
	
	for (int i = 0; i < strlen(stringChars); i++) {
		newString += tolower(stringChars[i]);
	}

	return newString;
}