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
#include <vector>

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
#define MAX_BUFFER_SIZE 256
#define UTIMER 300000;
#define STIMER 0;
struct timeval timeouts;
//timeouts.tv_sec=STIMER;
//timeouts.tv_usec=UTIMER;

//packet structure definition
#define STOPNWAIT 1 //Bits width For stop and wait
struct MESSAGE_FRAME {
	unsigned int header;
	unsigned int snwseq;
	int sequence;
	char packet_type;//t:three_way; m:message;
	char data[MAX_BUFFER_SIZE];//or error message
} msgfrm, msgrecv;

//three way handshake definition
struct THREE_WAY_HS{
	int client_number;
	int server_number;
	int window_size;
	char packet_type;//t:three_way; m:message;
	char direction[10];
	char file_name[1000];
	char file_rename[1000];
} three_way_send, three_way_recv;

//buffer data types
char getRename[1000];
char getFile[1000];
bool fileRenamed = false;
int windowSize = 0;

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

boolean fileExists(char namefile[]);
boolean isFileValid(char namefile[]);
boolean isDirectionValid(char directiontype[]);
boolean isStringEmpty(char stringChars[]);
string removeWhiteSpaces(char paramText[]);
string lowerCaseString(char stringChars[]);

void handleGet(FILE *logfile);
void handlePut(FILE *logfile);
void handleList(FILE *logfile);
void handleRename(FILE *logfile);
void sort(int (&list)[4], int length );

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

		if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
			throw "Socket failed\n";
		
		//Create the port and the address
		memset(&sa, 0, sizeof(sa));
		sa.sin_family = AF_INET;
		sa.sin_port = htons(port);
		//Host to network
		sa.sin_addr.s_addr = htonl(INADDR_ANY); 

		//Bind the port to the socket
		if (bind(s, (LPSOCKADDR)&sa, sizeof(sa)) == SOCKET_ERROR)
			throw "can't bind the socket";

		//Get the name of remote server
		cout << "Type name of ftp server: " << flush;
		cin >> remotehost;

		//Quit in any case end session
		string quitCommand = lowerCaseString(remotehost);
		if (quitCommand.compare("quit") == 0) {
			cout << "Quitting..." << endl;
			closesocket(s);
			Sleep(3000);
			exit(0);
		}

		if ((rp = gethostbyname(remotehost)) == NULL)
			throw "remote gethostbyname failed\n";

		memset(&sa_in, 0, sizeof(sa_in));
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

			//Ask for type direction of transfer: get, put, list
			cout << "\nType direction of transfer \n(get, put, rename, quit): " << flush;
			cin >> three_way_send.direction;

			//Validate namefile
			while (!isDirectionValid(three_way_send.direction) || string(three_way_send.direction).empty()) {
				cout << "Available directions: \n" <<
					"\t get  - get file from server\n" <<
					"\t put  - put file to server\n" <<
					"\t rename - rename to server\n"<<
					"\t quit - quit session\n" << endl;
				cout << "Type direction of transfer: " << flush;
				cin >> three_way_send.direction;
			}

			if (strcmp(three_way_send.direction, "quit") != 0) {
				string direction = lowerCaseString(three_way_send.direction);

				if(direction.compare("rename") == 0) {
					//Ask for file to be renamed from server
					cout << "\nFile to be renamed: " << flush;
					cin >> three_way_send.file_name;

					//Name to be used in replacing
					cout << "Rename file with: " << flush;
					cin >> three_way_send.file_rename;

					//Validate file rename
					while (!isFileValid(three_way_send.file_rename) || string(three_way_send.file_rename).empty()) {
						cout << "\nFile names should not contain: \\/:*?\"<>| or any blank spaces." << endl;
						cout << "Rename file with: " << flush;
						cin >> three_way_send.file_rename;
					}

					//Only Yes or No for answers
					char answer[10];
					do {
						cout << "Are you sure you want to rename " << three_way_send.file_name << " to " << three_way_send.file_rename << " (Y/N)?" << flush;
						cin >> answer;
					} while(strcmp(answer, "y") != 0 && strcmp(answer, "n") != 0);

					//Ask for a new file transfer if answer is no; 
					//otherwise initiate handshake
					if(strcmp(answer, "n") == 0) {
						cout << "File rename cancelled." << endl;
						continue;
					}
				}
				else if (direction.compare("get") == 0 || direction.compare("put") == 0) {
					//Ask for file to be put to/get from server
					cout << "\nFile to be transferred: " << flush;
					cin >> three_way_send.file_name;

					//Validate name file
					while (!isFileValid(three_way_send.file_name) || string(three_way_send.file_name).empty()) {
						cout << "\nFile names should not contain: \\/:*?\"<>| or any blank spaces." << endl;
						cout << "File to be transferred: " << flush;
						cin >> three_way_send.file_name;
					}

					//Ask for window size
					do {
						cout << "\nEnter window size 1 - 19: " << flush;
						cin >> three_way_send.window_size;
					} while(three_way_send.window_size < 1 || three_way_send.window_size > 19);
				}
			}

			cout << "Initiating handshake..." << endl;

			srand((unsigned)time(NULL));
			three_way_send.client_number = rand() % 255 + 1;
			three_way_send.packet_type = 't';

			cout << "Sending random number to server: " << three_way_send.client_number << endl;
			ibytessent = sendto(s, (const char*)&three_way_send, sizeof(three_way_send), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));

			/* Initiate three way handshake */
			
			//First wait for server/receiver 
			while (true) {
				FD_ZERO(&readfds);
				FD_SET(s, &readfds); //always check the listener

				outfds = select(s, &readfds, NULL, NULL, &timeout); //timed out, return
				int size = sizeof(sa_in);

				if (outfds > 0) {
					//receive frame
					//Clean buffer
					memset(&three_way_recv, 0, sizeof(three_way_recv));
					ibytesrecv = recvfrom(s, (char*)&three_way_recv, sizeof(three_way_recv), 0, (struct sockaddr*)&sa_in, &size);

					if (three_way_recv.packet_type == 't') {
						cout << "\nServer random number received: " << three_way_recv.server_number << endl;
						cout << "Client random number included: " << three_way_recv.client_number << endl;

						if (three_way_recv.client_number == three_way_send.client_number) {
							ibytessent = sendto(s, (const char*)&three_way_recv, sizeof(three_way_recv), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
							break;
						}
					}
					else if (three_way_recv.packet_type == 'm') continue;
				}
				else if (!outfds) {
					cout << "Resending client random number to server..." << endl;
					ibytessent = sendto(s, (const char*)&three_way_send, sizeof(three_way_send), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
				}
			}

			cout << "\nWaiting for final response from server..." << endl;
			int waitingTime = 0;

			//Second wait for server/receiver
			while (true) {
				if (waitingTime == 3) break;
				FD_ZERO(&readfds);
				FD_SET(s, &readfds);  //always check the listener

				//Send ACK2 to server/receiver
				cout << "Resending number back to server..." << endl;
				sendto(s, (const char*)&three_way_recv, sizeof(three_way_recv), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
				waitingTime++;
			}

			Sleep(3);

			/* End three way handshake */
			if (TRACE) fprintf(logfile, "\nSender: Handshake established\n");
			cout << "Handshake established...\n" << endl;
			windowSize = three_way_send.window_size;

			/* GET FROM SERVER - GET */
			if (strcmp(three_way_send.direction, "get") == 0) {
				if (strcmp(three_way_send.file_name, "list") == 0) handleList(logfile); // Do get list
				else handleGet(logfile); // Do get with file
			}
			/* PUT TO SERVER - PUT */
			else if (strcmp(three_way_send.direction, "put") == 0) {
				handlePut(logfile);
			}
			/* RENAME FILE FROM SERVER - RENAME */
			else if (strcmp(three_way_send.direction, "rename") == 0) {
				handleRename(logfile);
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
	//File exists
	while (fileExists(three_way_send.file_name)) {
		cout << "File exists please rename the file to be submitted \n(type n or N to cancel): " << flush;
		cin >> three_way_send.file_name;

		//Check validate or no-validate namefile
		while (!isFileValid(three_way_send.file_name)) {
			cout << "\nFile names should not contain: \\/:*?\"<>| or any blank spaces." << endl;
			cout << "File to be transferred: " << flush;
			cin >> three_way_send.file_name;
		}

		if(strcmp(three_way_send.file_name,"n") == 0 || strcmp(three_way_send.file_name,"N") == 0) {
			//Cancel the transfer
			memset(&msgfrm, 0, sizeof(msgfrm));
			strcpy(msgfrm.data, "n"); 
			msgfrm.packet_type = 'r';
			//These issues is droped 
			//Then resend this much
			for(int i = 0; i < 4; i++) 
				sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
			return;
		}
	}

	//File name is right then start transfer
	cout << "Initialise file transfer..." << endl;
	memset(&msgfrm, 0, sizeof(msgfrm));
	strcpy(msgfrm.data, "SENDFILE");
	msgfrm.packet_type = 'r';
	sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));

	if (TRACE) fprintf(logfile, "Sender: GET %s from server\n", three_way_send.file_name);
	cout << "Retrieving file " << three_way_send.file_name << " from " << remotehost << ", waiting...\n" << endl;
	//In the local client folder
	string _filepath = string(three_way_send.file_name); 
	//Create the file
	FILE *fp = fopen(_filepath.c_str(), "wb+"); 

	//The packet info
	int totalBytes = 0,
		totalPackets = 0,
		totalEBytes = 0,
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
			}
			else if (msgrecv.packet_type == 'm') {
				//End of the file reach
				if (strcmp(msgrecv.data, "ENDOFFILE") == 0) {
					fclose(fp);
					break;
				}
				else if (strcmp(msgrecv.data, "NOFILEEXISTS") == 0) {
					fclose(fp);
					if (remove(_filepath.c_str()) == 0) cout << "FILE DOESN'T EXIST" << endl;
					break;
				}

				//It is right then update sending frame
				if (msgrecv.sequence == waitingSeq) {
					if (TRACE) fprintf(logfile, "Sender: Sequence %d accepted\n", msgrecv.sequence);
					cout << "Sequence " << msgrecv.sequence << " written to file." << endl;
					//Write file
					fwrite(msgrecv.data, 1, msgrecv.header, fp);
					memset(&msgfrm, 0, sizeof(msgfrm));
					strcpy(msgfrm.data, "ACK");
					msgfrm.packet_type = 'm';
					msgfrm.sequence = msgrecv.sequence;
					waitingSeq++; //Go next sequence
				}
				else {
					if (TRACE) fprintf(logfile, "Sender: Sequence %d rejected\n", msgrecv.sequence);
					//It is not wanted packet
					memset(&msgfrm, 0, sizeof(msgfrm));
					strcpy(msgfrm.data, "NAK");
					msgfrm.packet_type = 'm';
					//Let the sequence is waited sequence
					msgfrm.sequence = waitingSeq; 
					totalPackets++;
					totalBytes += msgrecv.header;
				}

				//Send ACK or NAK packets
				sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
			}
		} 
		else {
			sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
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
	//Can allow to rename
	memset(&msgfrm, 0, sizeof(msgfrm));
	msgfrm.packet_type = 'r';
	strcpy(msgfrm.data, three_way_send.file_name);

	while (true) {
		FD_ZERO(&readfds);
		FD_SET(s, &readfds);

		outfds = select(s, &readfds, NULL, NULL, &timeout); //timed out, return
		int size = sizeof(sa_in);

		if (outfds > 0) {
			memset(&msgrecv, 0, sizeof(msgrecv));
			recvfrom(s, (char*)&msgrecv, sizeof(msgrecv), 0, (struct sockaddr*)&sa_in, &size);

			if(strcmp(msgrecv.data, "SENDFILE") == 0) { 
			//Can start to send the file
				break;

			} else if(strcmp(msgrecv.data, "RENAMEFILE") == 0) { 
			//Initialize the file rename
				if (TRACE) fprintf(logfile, "Sender: Filename exists\n");
				memset(&msgfrm, 0, sizeof(msgfrm));
				msgfrm.packet_type = 'r';

				cout << "File exists please rename the file to be submitted \n(type n or N to cancel): " << flush; 
				cin >> msgfrm.data;

				if(strcmp(msgfrm.data,"n") == 0 || strcmp(msgfrm.data,"N") == 0) {
					cout << "File transfer cancelled..." << endl;
					//Cancel transfer
					for(int i = 0; i < 4; i++) // Drop issues
						sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
					return;
				}

				sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
			}
		} 
		else {
			//No response from receiver; p
			//Ack must have been dropped and resend
			sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
		}
	}

	if (TRACE) fprintf(logfile, "Sender: PUT %s to server\n", three_way_send.file_name);
	cout << "Sending file " << three_way_send.file_name << " to " << remotehost << ", waiting...\n" << endl;

	FILE *fp;
	string _filepath = string(three_way_send.file_name);

	if (fp = fopen(_filepath.c_str(), "rb")) {
		//Packet info
		int totalBytes = 0, 
			totalPackets = 0, 
			totalEffBytes = 0,
			currentBase = 0,
			waitingSeq = 0;

		//Get the file length and the total frames
		fseek(fp, 0, SEEK_END);
		long fileSize = ftell(fp);
		rewind(fp);
		int totalFrames = (int) ceil((float)fileSize / MAX_BUFFER_SIZE);

		while(true) {
			//Send packets in window size limit
			for (int i = 0; i < windowSize && (currentBase + i) < totalFrames; i++) {
				memset(&msgfrm, 0, sizeof(msgfrm));
				msgfrm.header = (fileSize < MAX_BUFFER_SIZE) ? fileSize : MAX_BUFFER_SIZE;
				//Gets larger
				msgfrm.sequence = currentBase + i; 
				msgfrm.packet_type = 'm';
				fread(msgfrm.data, 1, MAX_BUFFER_SIZE, fp);
				totalBytes += MAX_BUFFER_SIZE;
				totalPackets++;

				sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
			}
			
			if (TRACE) fprintf(logfile, "Sender: Packets %d - %d sent.\n", currentBase, currentBase+windowSize-1); 

			//Waiting for ACKs of 4 sent packets
			bool isNak = false;
			for (int j = 0; j < windowSize; j++) {
				FD_ZERO(&readfds);
				FD_SET(s, &readfds);

				outfds = select(s, &readfds, NULL, NULL, &timeout); //timed out, return
				int size = sizeof(sa_in);

				if (outfds > 0) {
					memset(&msgrecv, 0, sizeof(msgrecv));
					recvfrom(s, (char*)&msgrecv, sizeof(msgrecv), 0, (struct sockaddr*)&sa_in, &size);

					//Correct packet to be sent
					if (msgrecv.packet_type == 'm') {
						if (strcmp(msgrecv.data, "ACK") == 0) {
							//Comparison to currentBase and ignore delayed packets
							if (msgrecv.sequence > currentBase)
								currentBase = msgrecv.sequence; 
								//Assign base for next packet
								cout << "ACK: " << currentBase << endl;
							
							totalEffBytes += MAX_BUFFER_SIZE;
							if (TRACE) fprintf(logfile, "Sender: Packet ACK'd - %d\n", msgrecv.sequence); 
						}
						//Dropped or delayed packet
						else if (strcmp(msgrecv.data, "NAK") == 0) {
							isNak = true;
							currentBase = msgrecv.sequence;
							//Recalculate file pointer position
							fseek(fp, currentBase * MAX_BUFFER_SIZE, SEEK_SET); 

							if (TRACE) fprintf(logfile, "Sender: Packet NAK'd - %d\n", msgrecv.sequence); 
							break;
						}
					}
				}
			}
			
			//End of file
			if (currentBase == totalFrames) {
				memset(&msgfrm, 0, sizeof(msgfrm));
				msgfrm.packet_type = 'm';
				strcat(msgfrm.data, "ENDOFFILE");
				
				//End of file reached
				for (int k = 0; k < windowSize; k++)
					sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
				
				cout << "End of file reached." << endl;
				if (TRACE) fprintf(logfile, "Sender: End of file reached\n"); 
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

void handleList(FILE *logfile) {
	int sequence = 0;
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
			if (strcmp(msgrecv.data, "ENDOFLIST") == 0) break;

			//If ACK is lost
			if (msgrecv.snwseq == sequence) {
				if (TRACE) fprintf(logfile, "Sender: received packet %d\n", msgrecv.snwseq);
				cout << msgrecv.data << endl;
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
				//Resend ACK if it is not wanted packet is received
				sendto(s, (const char*)&msgfrm, sizeof(msgfrm), 0, (struct sockaddr*) &sa_in, sizeof(sa_in));
				if (TRACE) fprintf(logfile, "Sender: resent %s for packet %d\n", msgfrm.data, msgrecv.snwseq);
			}
		}
	}
}

void handleRename(FILE *logfile) {
	if (TRACE) fprintf(logfile, "Renaming file %s to %s.", three_way_send.file_name, three_way_send.file_rename);
	int sequence = 0;
	while (true) {
		FD_ZERO(&readfds);
		FD_SET(s, &readfds);  //always check the listener

		outfds = select(s, &readfds, NULL, NULL, &timeout);//timed out, return
		int size = sizeof(sa_in);

		if (outfds > 0) {
			//receive frame
			memset(&msgrecv, 0, sizeof(msgrecv));
			ibytesrecv = recvfrom(s, (char*)&msgrecv, sizeof(msgrecv), 0, (struct sockaddr*)&sa_in, &size);

			if(msgrecv.packet_type == 't') break;
			if (strcmp(msgrecv.data, "FILERENAMED") == 0) {
				cout << "File " << three_way_send.file_name << " successfully renamed to " << three_way_send.file_rename << "." << endl;
				break;
			} 
			else if (strcmp(msgrecv.data, "FILEERROR") == 0) {
				cout << "File renaming encountered an error." << endl;
				break;
			}
		}
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
		strdirection.compare("rename") == 0 ||
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