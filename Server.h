#pragma once
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include "HTTP.h"
#include "ThreadPool .h"
#include <fstream>



void importLib();
SOCKET initiateSocket();
void bindSocket(const SOCKET* m_socket);
bool addSocket(SOCKET id, int what, struct SocketState* sockets, int* socketsCount);
void createSets(fd_set* waitRecv, fd_set* waitSend, struct SocketState* sockets);
void filterUpcomingEvents(int* nfd, fd_set* waitRecv, fd_set* waitSend);
void handleEvents(int* nfd, fd_set* waitRecv, fd_set* waitSend, struct SocketState* sockets, int* socketsCount, ThreadPool* threadPool);
void finishingUp(SOCKET connSocket);
void acceptConnection(int index, struct SocketState* sockets, int* socketsCount);
void receiveMessage(int index, struct SocketState* sockets, int* socketsCount);
void sendMessage(int index, struct SocketState* sockets, int* socketsCount);
void removeSocket(int index, struct SocketState* sockets, int* socketsCount);
void GetFile(struct Request* req);
void handleRecvError(int index, SOCKET msgSocket, struct SocketState* sockets, int* socketsCount);
void handleRecvSuccess(int index, SOCKET msgSocket, int bytesRecv, struct SocketState* sockets, int* socketsCount);
void parseRequest( string& cmd,  string& path, stringstream& mesStream, Request* req);
void AppendLanguageToFileName(Request* req);
void CheckFileExistence(Request* req);
void GetFileContent(Request* req);