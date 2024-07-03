#pragma once
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE

#include <iostream>
using namespace std;
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <string>
#include <string.h>
#include <sstream>
#include <time.h>
#include <vector>
#include <cstring>

#define BASE_PATH "C:/Temp/"


const int ERR = -1;
const int OPTIONS = 1;
const int GET = 2;
const int HEAD = 3;
const int POST = 4;
const int PUT = 5;
const int DEL = 6;
const int TRACE = 7;
const int MINIMUM_NUMBER_OF_THREAD = 2;
const int MAXIMUM_NUMBER_OF_THREAD = 8;


struct Attribute {
	string key;
	string data;
};

struct PostInfo {
	string type;
	string length;
	string data;
};

struct WantedFile {
	string name;
	string language;
	string content;
};

struct Request {
	int method = 0;         
	int status = 0;         
	WantedFile file;        
	PostInfo postContent;   
};

struct SocketState {
	SOCKET id = 0;         
	int recv = 0;          
	int send = 0;          
	char buffer[1500] = { 0 }; // Initialize buffer to zeros
	int len = 0;           
	Request req;           
	clock_t responseTime = 0; 
	clock_t turnaroundTime = 0;

	SocketState() {
		std::memset(buffer, 0, sizeof(buffer)); 
	}
};

const int TIME_PORT = 8080;
const int MAX_SOCKETS = 60;
const int EMPTY = 0;
const int LISTEN = 1;
const int RECEIVE = 2;
const int IDLE = 3;
const int SEND = 4;
const int SEND_TIME = 1;
const int SEND_SECONDS = 2;


void FindWantedFile(string path, struct Request* req, string* cmd);
void ParseGetMsg(stringstream& mesStream, struct Request* req);
void ParseMsg(stringstream& mesStream, struct Request* req);
void ParsePostMsg(stringstream& mesStream, struct Request* req);
void ParsePutMsg(stringstream& mesStream, struct Request* req);
string CreateMessage(struct SocketState socket);
struct Attribute findNextAtt(stringstream& msg, string input);
void AddStatusMessage(int status, stringstream& message);
void AddMethodSpecificHeaders(struct SocketState socket, stringstream& message);
void AddGETResponse(struct SocketState socket, stringstream& message);
void AddHEADResponse(struct SocketState socket, stringstream& message);
void AddDELResponse(struct SocketState socket, stringstream& message);
void AddPUTResponse(struct SocketState socket, stringstream& message);
void PrintPostData(const string& postData);
