// CLIENT TCP PROGRAM
// Revised and tidied up by
// J.W. Atwood
// 1999 June 30

//COMP 445 Assignment1
//Name: Haochen Wang
//Date: 10/2/2015



char* getmessage(char *);



/* send and receive codes between client and server */
/* This is your basic WINSOCK shell */
#pragma comment( linker, "/defaultlib:ws2_32.lib" )
#include <winsock2.h>
#include <ws2tcpip.h>
#include <winsock.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <string>
#include <windows.h>
#include <fstream>
#include <ctype.h>
#include <locale>

using namespace std;

//user defined port number
#define REQUEST_PORT 0x7070;

int port = REQUEST_PORT;



//socket data types
SOCKET s;
SOCKADDR_IN sa;         // filled by bind
SOCKADDR_IN sa_in;      // fill with server info, IP, port



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

boolean fileValid(char namefile[]);
boolean directionValid(char directionType[]);
string lowerCaseString(char stringChars[]);

int main(void){

	WSADATA wsadata;

	try {

		if (WSAStartup(0x0202, &wsadata) != 0){
			cout << "Error in starting WSAStartup()" << endl;
		}
		else {
			buffer = "WSAStartup was successful\n";
			WriteFile(test, buffer, sizeof(buffer), &dwtest, NULL);

			/* Display the wsadata structure
			cout << endl
				<< "wsadata.wVersion " << wsadata.wVersion << endl
				<< "wsadata.wHighVersion " << wsadata.wHighVersion << endl
				<< "wsadata.szDescription " << wsadata.szDescription << endl
				<< "wsadata.szSystemStatus " << wsadata.szSystemStatus << endl
				<< "wsadata.iMaxSockets " << wsadata.iMaxSockets << endl
				<< "wsadata.iMaxUdpDg " << wsadata.iMaxUdpDg << endl;*/
		}




		// HANDLE IF HOST NAME IS QUIT

		//Create the socket
		if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
			throw "Socket failed\n";
		/* For UDP protocol replace SOCK_STREAM with SOCK_DGRAM */

		//Ask for name of remote server
		cout << "Type name of ftp server: " << flush;
		cin >> remotehost;

		//Quit command
		string quitCommand = lowerCaseString(remotehost);
		if (quitCommand.compare("quit") == 0) {
			cout << "Quitting..." << endl;
			closesocket(s);
			Sleep(3000);
			exit(0);
		}

		if ((rp = gethostbyname(remotehost)) == NULL)
			throw "remote gethostbyname failed\n";

		//Specify server address for client to connect to server
		memset(&sa_in, 0, sizeof(sa_in));
		memcpy(&sa_in.sin_addr, rp->h_addr, rp->h_length);
		sa_in.sin_family = rp->h_addrtype;
		sa_in.sin_port = htons(port);

		
		//Connect Client to the server
		if (connect(s, (LPSOCKADDR)&sa_in, sizeof(sa_in)) == SOCKET_ERROR)
			throw "connect failed\n";
		else cout << "Connected to server: \"" << remotehost << "\"" << endl;

		while (true) {

			//Clean buffer
			memset(&szbuffer[0], 0, sizeof(szbuffer));

			/* Have an open connection, so, server is
			- waiting for the client request message
			- don't forget to append <carriage return>
			- <line feed> characters after the send buffer to indicate end-of file */

			char localHost[11], nameFile[50], directionType[5];

			// Display info of local host
			gethostname(localHost, 20);
			strcat_s(szbuffer, localHost);
			strcat_s(szbuffer, "?");

			//Ask for file to be transferred from server
			cout << "\nType name of file to be transferred: " << flush;
			cin >> nameFile;

			//Check the namefile is right
			while (!fileValid(nameFile) || string(nameFile).empty()) {
				cout << "\nFile names is not right." << endl;
				cout << "\nType name of file to be transferred: " << flush;
				cin >> nameFile;
			}

			//Connect the localhost and the namefile
			strcat_s(szbuffer, nameFile);

			strcat_s(szbuffer, "?");

			//Ask for direction type: get, put, list
			cout << "\nType direction of transfer (get, put, list, delete, quit): " << flush;
			cin >> directionType;

			//Check the  direction tpye is right
			while (!directionValid(directionType) || string(directionType).empty()) {
				cout << "Right directions: \n" <<
					"\t get: get file from server\n" <<
					"\t put: put file to server\n" <<
					"\t list: list files\n" <<
					"\t delete: delete file from the server\n" <<
					"\t quit: quit session\n" << endl;
				cout << "Type direction of transfer: (get, put, list, delete, quit):" << flush;
				cin >> directionType;
			}
			strcat_s(szbuffer, directionType);

			ibytessent = 0;
			ibufferlen = strlen(szbuffer);
			ibytessent = send(s, szbuffer, ibufferlen, 0); // Send to the Server


			//Server first then quit command
			if (strcmp(directionType, "quit") == 0) {
				cout << "\nQuitting..." << endl;
				closesocket(s);
				Sleep(3000);
				exit(0);
				break;
			}

			//Get file from the Server 
			if (strcmp(directionType, "get") == 0) {
				char response[128];
				cout << "\nSent request to " << remotehost << ", waiting...\n" << endl;

				//Get file Server to Response
				recv(s, response, 128, 0);
				//cout << "Response: " << response << endl;
				string res = response, fExist = "FILE EXISTS";

				if (res.find(fExist) != string::npos) {
					//Check the file length
					long int fileSize = atol(res.substr(strlen("FILE EXISTS")).c_str());
					//cout << "File size: " << _fileSize << endl;

					char *fileBuffer;
					int bufferSize = 256;

					//Create a binary file
					//Local Client folder
					string filePath = string(nameFile);
					FILE *fileOpen = fopen(filePath.c_str(), "wb+");

					while (true) {
						//Get Buffer container
						fileBuffer = new char[256];

						//Get Buffer from Server
						int reader = recv(s, fileBuffer, 256, 0);

						//Check if end is reached
						if (strcmp(fileBuffer, "END OF FILE") == 0) {
							cout << "File has been received." << endl;
							fclose(fileOpen);
							break;
						}

						//Write the Buffer into the file
						fwrite(fileBuffer, 1, bufferSize, fileOpen);

						//The current size
						fileBuffer -= reader;
						bufferSize = (fileSize < 256) ? fileSize : 256;
					}
				}
				else {
					//Error
					cout << "It is no working.\n Server response: " << response << endl;
				}
			}


			//Put file in the Server
			else if (strcmp(directionType, "put") == 0) {
				//Search the file and send it to Server
				FILE *fileOpen;

				//Local client folder
				string filePath = string(nameFile);
				if (fileOpen = fopen(filePath.c_str(), "rb")) {
					cout << "FILE EXISTS" << endl;
					cout << "Sending " << nameFile << " to " << remotehost << ", waiting..." << endl;

					//The file length
					fseek(fileOpen, 0, SEEK_END);
					long fileSize = ftell(fileOpen);
					rewind(fileOpen);

					//Send response to client
					string response = "FILE EXISTS" + to_string(fileSize);
					//cout << response << endl;
					send(s, response.c_str(), 128, 0);
					//printf("length %d \n\n", fileSize);


					while (true) {
						//Create a Buffer to save data
						char* bf = new char[256];

						// Allocate buffer with data from file by size?????????????????
						int reader = fread(bf, 1, 256, fileOpen);
						//cout << " LENGTH: " << strlen(bf) << " READER: " << reader << endl;

						reader;
						strlen(bf);

						//Send the Buffer to Client
						send(s, bf, 256, 0);

						if (feof(fileOpen)) {
							//The file is reached.
							send(s, "END OF FILE", 256, 0);
							cout << "\nFile has been reached." << endl;
							fclose(fileOpen);
							break;
						}
					}
				}
				else {
					//Send message to Server the file donesn't exist
					cout << "File doesn't exist" << endl;
					send(s, "FILE DOESN'T EXIST", 128, 0);
				}
			}


			//List file from Server 
			else if (strcmp(directionType, "list") == 0) {
				char *listOfBuffer;
				//Print files list here 
				while (true) {
					listOfBuffer = new char[128];
					recv(s, listOfBuffer, 128, 0);

					cout << listOfBuffer << endl;

					if (strcmp(listOfBuffer, "End of list") == 0)
						break;
				}
			}

			//Delete the file from the Server 
			else if (strcmp(directionType, "delete") == 0) {
				char *responseOfBuffer = new char[128];
				recv(s, responseOfBuffer, 128, 0);

				if (strcmp(responseOfBuffer, "DELETED") == 0) {
					cout << "File have been deleted." << endl;
				}
				else if (strcmp(responseOfBuffer, "ERROR") == 0) {
					cout << "Error: " << nameFile << "does not exist." << endl;
				}
			}
			else if (strcmp(directionType, "quit") == 0) {
				closesocket(s);
				exit(0);
				break;
			}
			continue;
		} // End while
	}

	//Display any needed error response.

	catch (char *str) { cerr << str << ":" << dec << WSAGetLastError() << endl; }

	//close the client socket


	/* When done uninstall winsock.dll (WSACleanup()) and exit */
	WSACleanup();

	return 0;
}

boolean fileValid(char nameFile[]) {
	char keys[] = "\\/:*?\"<>|";

	return (strpbrk(nameFile, keys) != NULL) ? false : true;
}

boolean directionValid(char directionType[]) {
	string stringDirection = lowerCaseString(directionType);

	if (stringDirection.compare("get") == 0 ||
		stringDirection.compare("put") == 0 ||
		stringDirection.compare("list") == 0 ||
		stringDirection.compare("quit") == 0 ||
		stringDirection.compare("delete") == 0) {
		return true;
	}
	else {
		return false;
	}
}

string lowerCaseString(char stringChars[]) {
	string string;

	for (int i = 0; i < strlen(stringChars); i++) {
		string += tolower(stringChars[i]);
	}

	return string;
}