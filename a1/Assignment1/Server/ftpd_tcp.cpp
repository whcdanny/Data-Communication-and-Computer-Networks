// SERVER TCP PROGRAM
// revised and tidied up by
// J.W. Atwood
// 1999 June 30
// There is still some leftover trash in this code.


//COMP 445 Assignment1
//Name: Haochen Wang
//Date: 10/2/2015

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
#include <strsafe.h>
#include <vector>
#include <comdef.h> 
#pragma comment(lib, "User32.lib")



using namespace std;

//port data types

#define REQUEST_PORT 0x7070

int port = REQUEST_PORT;

//socket data types
SOCKET s;
SOCKADDR_IN sa;      // filled by bind
SOCKADDR_IN sa1;     // fill with server info, IP, port
union {
	struct sockaddr generic;
	struct sockaddr_in ca_in;
}ca;

int calen = sizeof(ca);

//buffer data types
char clientbuffer[128];

char *buffer;
int ibufferlen;
int ibytesrecv;

int ibytessent;

//host data types
char localhost[21];

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

unsigned __stdcall client_server(void *data);

//My laptop
//string filesPath = "C:\\Danny-Fox\\study\\COMP\\COMP445\\A\\Assignment1\\Sever\\file\\";
//School Computer
string filesPath = "G:\\My Documents\\Visual Studio 2013\\Projects\\Assignment1\\Server\\files\\";


int main(void){

	WSADATA wsadata;

	try{
		if (WSAStartup(0x0202, &wsadata) != 0){
			cout << "Error in starting WSAStartup()\n";
		}
		else{
			buffer = "WSAStartup was suuccessful\n";
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

		// Display info of local host

		gethostname(localhost, 20); // Get computer name: change to localhost
		cout << "ftpd_tcp starting at host: " << localhost << endl;

		if ((hp = gethostbyname(localhost)) == NULL) {
			cout << "gethostbyname() cannot get local host info?"
				<< WSAGetLastError() << endl;
			exit(1);
		}

		// Create the server socket
		if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
			throw "can't initialize socket";
		// For UDP protocol replace SOCK_STREAM with SOCK_DGRAM 

		// Fill-in Server Port and Address info.
		sa.sin_family = AF_INET;
		sa.sin_port = htons(port);
		sa.sin_addr.s_addr = htonl(INADDR_ANY);


		//Bind the server port
		if (bind(s, (LPSOCKADDR)&sa, sizeof(sa)) == SOCKET_ERROR)
			throw "can't bind the socket";
		//else cout << "Bind was successful" << endl;

		//Successfull bind, now listen for client requests.

		if (listen(s, 10) == SOCKET_ERROR)
			throw "couldn't  set up listen on socket";
		//else cout << "Listen was successful" << endl;

		FD_ZERO(&readfds);

		//wait loop
		//Create Client Socket
		SOCKET cl_socket;
		while (true) {
			//Find a request, and to accept.  

			//Multi-thread

			cl_socket = accept(s, &ca.generic, &calen);

			FD_SET(s, &readfds);  //always check the listener

			if (!(outfds = select(infds, &readfds, NULL, NULL, tp))) {
				cout << "waiting to be contacted for transferring files...\n" << endl;
			}
			else if (outfds == SOCKET_ERROR) {
				throw "failure in Select";
			}
			else if (FD_ISSET(s, &readfds)) {
				cout << "got a connection request" << endl;
			}

			//Connection request accepted.

			/*cout << "accepted connection from " << inet_ntoa(ca.ca_in.sin_addr) << ":"
				<< hex << htons(ca.ca_in.sin_port) << "\n" << endl;*/


			unsigned threadID;
			HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, &client_server, (void*)cl_socket, 0, &threadID);


		} //wait loop
		//Close Client Socket
		closesocket(cl_socket);

	} //try loop

	//Display needed error message.

	catch (char* str) { cerr << str << WSAGetLastError() << endl; }

	//close server socket
	closesocket(s);

	/* When done uninstall winsock.dll (WSACleanup()) and exit */
	WSACleanup();
	return 0;
}

unsigned __stdcall client_server(void *data)
{
	SOCKET cl_socket = (SOCKET)data;


	while (true) {

		//Clean Buffer
		memset(&clientbuffer[0], 0, sizeof(clientbuffer));

		//Waiting the message
		cout << "\nWaiting for commands...\n" << endl;

		if ((ibytesrecv = recv(cl_socket, clientbuffer, 128, 0)) == SOCKET_ERROR) {
			cout << "Lost connection. " << endl;
			throw "Receive error in server program\n";
		}
		else {

			string clientname, namefile, directiontype;

			// Clientbuffer like it is and accepted this: clienthost?namefile?directiontype
			// Seperate client buffer using '?' as delimeter

			int qPos = 1;
			for (int i = 0; i < strlen(clientbuffer); i++) {
				int j = 0;
				if (clientbuffer[i] != '?') {
					switch (qPos) {
					case 1: clientname += clientbuffer[i]; break;
					case 2: namefile += clientbuffer[i]; break;
					case 3: directiontype += clientbuffer[i]; break;
					}
				}
				else {
					qPos++;
				}
			}

			//Get file from the Server
			if (directiontype.compare("get") == 0) {
				cout << "User \"" << clientname << "\" requested file " << namefile << " to be sent." << endl;

				//Search the file and send it
				FILE *fileOpen;
				string _filepath = filesPath + namefile;

				if (fileOpen = fopen(_filepath.c_str(), "rb")) {
					cout << "FILE EXISTS" << endl;
					cout << "Sending " << namefile << " to " << clientname << ", waiting..." << endl;

					//The file length
					fseek(fileOpen, 0, SEEK_END);
					long fileSize = ftell(fileOpen);
					rewind(fileOpen);

					//Send response to client
					string response = "FILE EXISTS" + to_string(fileSize);
					//cout << response << endl;
					send(cl_socket, response.c_str(), 128, 0);
					//printf("length %d \n\n", fileSize);


					while (true) {
						//Create a Buffer to save data
						char* bf = new char[256];

						// Allocate buffer with data from file by size?????????????
						int reader = fread(bf, 1, 256, fileOpen);
						//cout << " LENGTH: " << strlen(bf) << " READER: " << reader << endl;
						reader;
						strlen(bf);

						//Send the Buffer to Client
						send(cl_socket, bf, 256, 0);


						if (feof(fileOpen)) {
							//The file is reached.
							send(cl_socket, "END OF FILE", 256, 0);
							cout << "\nFile has been reached." << endl;
							fclose(fileOpen);
							break;
						}
					}
				}
				else {
					//Send message to Server the file donesn't exist
					cout << "File doesn't exist" << endl;
					send(cl_socket, "FILE DOESN'T EXIST", 128, 0);
				}
			}



			//Put file from the Server 
			else if (directiontype.compare("put") == 0) {
				cout << "User \"" << clientname << "\" requested file " << namefile << " to be uploaded." << endl;
				cout << "Retrieving file from " << clientname << ", waiting...\n" << endl;

				char response[128];
				//Get file Server to Response
				recv(cl_socket, response, 128, 0);
				//cout << "Response: " << _response << endl;
				string res = response, fExist = "FILE EXISTS";

				if (res.find(fExist) != string::npos) {
					//Check the file length
					long int fileSize = atol(res.substr(strlen("FILE EXISTS")).c_str());
					//cout << "File size: " << _fileSize << endl;

					char *fileBuffer;
					int bufferSize = 256;

					//Create a binary file
					string filePath = filesPath + string(namefile);

					FILE *fileOpen = fopen(filePath.c_str(), "wb+");

					while (true) {
						//Get Buffer container
						fileBuffer = new char[256];

						//Get Buffer from Server
						int reader = recv(cl_socket, fileBuffer, 256, 0);

						//Check if end is reached
						if (strcmp(fileBuffer, "END OF FILE") == 0) {
							cout << "File has been received." << endl;
							fclose(fileOpen);
							break;
						}

						//Write the Buffer into the file
						fwrite(fileBuffer, 1, bufferSize, fileOpen);

						//The current size
						fileSize -= reader;
						bufferSize = (fileSize < 256) ? fileSize : 256;
					}
				}
			}


			//List file from Server
			
			else if (directiontype.compare("list") == 0 || namefile.compare("list") == 0) {
				cout << "User \"" << clientname << "\" requested to display all the files." << endl;

				WIN32_FIND_DATA searchFile;
				memset(&searchFile, 0, sizeof(WIN32_FIND_DATA));


				//School computer directory
				LPCSTR filepath = "G:\\My Documents\\Visual Studio 2013\\Projects\\Assignment1\\Server\\*";
				HANDLE hd = FindFirstFile(filepath, &searchFile);

				while (hd != INVALID_HANDLE_VALUE) {
					//Look for the final string name
					int bfLength = 0;
					for (int filelength = 0; filelength < 260; filelength++)
					{
						if (searchFile.cFileName[filelength] == '\0') {
							bfLength = filelength + 1;
							break;
						}
					}

					_bstr_t b(searchFile.cFileName);
					const char* char1 = b;
					if ((ibytessent = send(cl_socket, char1, bfLength, 0)) == SOCKET_ERROR)
						throw "error in send in server program\n";

					cout << char1 << endl;

					if (!FindNextFile(hd, &searchFile))
							break;
				}

				send(cl_socket, "END OF LIST", 128, 0);
			}
			

			//Delete file from the Server
			else if (directiontype.compare("delete") == 0) {
				string _filepath = filesPath + namefile;

				if (remove(_filepath.c_str()) == 0) {
					send(cl_socket, "DELETED", 128, 0);
					cout << namefile << " has been deleted!" << endl;
				}
				else {
					send(cl_socket, "ERROR", 128, 0);
					cout << "Error: " << namefile << "does not exist." << endl;
				}
			}

			//Close the Client Socket
			else if (directiontype.compare("quit") == 0) {
				cout << "Client \"" << clientname << "\" is disconnected" << endl;
				closesocket(cl_socket);
				break;
			}
		}
	}
}