#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
using namespace std;
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <string>
#include <sstream>
#include <time.h>

//string CreateMessage(void);

//macros for request methods:
const int ERR = -1;
const int OPTIONS = 1;
const int GET = 2;
const int HEAD = 3;
const int POST = 4;
const int PUT = 5;
const int DEL = 6;
const int TRACE = 7;


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
	int method;
	int status;
	struct WantedFile file;
	struct PostInfo postContent;// for POST requests only
};

struct SocketState
{
	SOCKET id;			// Socket handle
	int	recv;			// Receiving?
	int	send;			// Sending?
	char buffer[1500];
	int len;
	struct Request req;
	clock_t responseTime = NULL, turnaroundTime = NULL;
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
