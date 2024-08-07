#include "Server.h"

void importLib() {
    WSAData wsaData{};

    if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData)) {
        cout << "T3 server: Error at WSAStartup()\n";
        return;
    }
}

SOCKET initiateSocket() {
    SOCKET m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (INVALID_SOCKET == m_socket) {
        cout << "T3 Server: Error at socket(): " << WSAGetLastError() << endl;
        WSACleanup();
        return 0;
    }

    return m_socket;
}

void bindSocket(const SOCKET *m_socket) {

    sockaddr_in serverService{};
    memset(&serverService, 0, sizeof(serverService));
    serverService.sin_family = AF_INET;
    serverService.sin_addr.s_addr = INADDR_ANY;
    serverService.sin_port = htons(TIME_PORT);

    if (SOCKET_ERROR == bind((*m_socket), (SOCKADDR *) &serverService, sizeof(serverService))) {

        cout << "T3 Server: Error at bind(): " << WSAGetLastError() << endl;
        closesocket((*m_socket));
        WSACleanup();

        return;
    }
}

bool addSocket(SOCKET id, int what, struct SocketState *sockets, int *socketsCount) {

    // init an array of sockets and init the all the socketState to empty
    // when I want to add a new socket (=> any client wants to create a new connection)
    // I check if there is any socket with status Empty
    // override the value it had before

    for (int i = 0; i < MAX_SOCKETS; i++) {

        if (sockets[i].recv == EMPTY) {
            sockets[i].id = id;
            sockets[i].recv = what;
            sockets[i].send = IDLE;
            sockets[i].len = 0;
            (*socketsCount)++;

            return (true);
        }
    }
    return (false);
}

void createSets(fd_set *waitRecv, fd_set *waitSend, struct SocketState *sockets) {
    FD_ZERO(waitRecv);

    for (int i = 0; i < MAX_SOCKETS; i++) {

        if ((sockets[i].recv == LISTEN) || (sockets[i].recv == RECEIVE)) {
            FD_SET(sockets[i].id, waitRecv);
        }
    }

    FD_ZERO(waitSend);

    for (int i = 0; i < MAX_SOCKETS; i++) {

        if (sockets[i].send == SEND) {
            FD_SET(sockets[i].id, waitSend);
        }
    }
}

void filterUpcomingEvents(int *nfd, fd_set *waitRecv, fd_set *waitSend) {
    *nfd = select(0, waitRecv, waitSend, nullptr, nullptr);

    if (*nfd == SOCKET_ERROR) {
        cout << "T3 Server: Error at select(): " << WSAGetLastError() << endl;
        WSACleanup();
        return;
    }
}

void finishingUp(SOCKET connSocket) {
    // Closing connections and Windsock.
    cout << "T3 server: Closing Connection.\n";

    closesocket(connSocket);
    WSACleanup();
}

void handleEvents(int *nfd, fd_set *waitRecv, fd_set *waitSend, struct SocketState *sockets, int *socketsCount,
                  ThreadPool *threadPool) {

    for (int i = 0; i < MAX_SOCKETS && *nfd > 0; i++) {

        if (FD_ISSET(sockets[i].id, waitRecv)) {

            (*nfd)--;

            switch (sockets[i].recv) {
                case LISTEN:
                        acceptConnection(i, sockets, socketsCount);
                    break;
                case RECEIVE:
                       receiveMessage(i, sockets, socketsCount);
                    break;
            }
        }
    }

    for (int i = 0; i < MAX_SOCKETS && *nfd > 0; i++) {

        if (FD_ISSET(sockets[i].id, waitSend)) {
            (*nfd)--;

            threadPool->enqueue([i, sockets, socketsCount] {
                sendMessage(i, sockets, socketsCount);
            });

        }
    }
}

void acceptConnection(int index, struct SocketState *sockets, int *socketsCount) {
    SOCKET id = sockets[index].id;
    // Address of sending partner
    struct sockaddr_in from{};
    int fromLen = sizeof(from);

    SOCKET msgSocket = accept(id, (struct sockaddr *) &from, &fromLen);

    if (INVALID_SOCKET == msgSocket) {
        cout << "T3 Server: Error at accept(): " << WSAGetLastError() << endl;
        return;
    }

    // Non-blocking mode
    unsigned long flag = 1;

    if (ioctlsocket(msgSocket, FIONBIO, &flag) != 0) {
        cout << "T3 Server: Error at ioctl socket(): " << WSAGetLastError() << endl;
    }

    if (!addSocket(msgSocket, RECEIVE, sockets, socketsCount)) {
        cout << "\t\tToo many connections, dropped!\n";
        closesocket(id);
    }
}

void receiveMessage(int index, struct SocketState *sockets, int *socketsCount) {

    SOCKET msgSocket = sockets[index].id;
    int len = sockets[index].len;
    int bytesReceive = recv(msgSocket, &sockets[index].buffer[len], sizeof(sockets[index].buffer) - len, 0);

    sockets[index].responseTime = clock();

    if (SOCKET_ERROR == bytesReceive) {
        handleRecvError(index, msgSocket, sockets, socketsCount);
        return;
    }

    if (bytesReceive == 0) {
        closesocket(msgSocket);
        removeSocket(index, sockets, socketsCount);
        return;
    }

    handleRecvSuccess(index, msgSocket, bytesReceive, sockets, socketsCount);
}

void handleRecvError(int index, SOCKET msgSocket, struct SocketState *sockets, int *socketsCount) {
    cout << "T3 Server: Error at receive(): " << WSAGetLastError() << endl;
    closesocket(msgSocket);
    removeSocket(index, sockets, socketsCount);
}

void handleRecvSuccess(int index, SOCKET msgSocket, int bytesRecv, struct SocketState *sockets, int *socketsCount) {
    sockets[index].buffer[sockets[index].len + bytesRecv] = '\0'; // add the null-terminating to make it a string
    sockets[index].len += bytesRecv;
    sockets[index].req.file.name.clear();
    sockets[index].req.file.content.clear();
    sockets[index].req.postContent.data.clear();

    stringstream mesStream(sockets[index].buffer);
    string cmd, path, httpVersion;
    mesStream >> cmd;
    mesStream >> path;
    mesStream >> httpVersion;

    parseRequest(cmd, path, mesStream, &(sockets[index].req));

    size_t currIndex = mesStream.tellg();
    int totalReqBytes = (currIndex == -1) ? sockets[index].len : static_cast<int>(currIndex);

    sockets[index].send = SEND;
    memcpy(sockets[index].buffer, &sockets[index].buffer[totalReqBytes],
           sockets[index].len - totalReqBytes); // move to the start of the buffer
    sockets[index].len -= totalReqBytes;
}

void parseRequest(string &cmd, string &path, stringstream &mesStream, Request *req) {
    if (cmd == "GET") {
        FindWantedFile(path, req, &cmd);
        req->method = GET;
        ParseGetMsg(mesStream, req);
        GetFile(req);
    } else if (cmd == "OPTIONS") {
        req->method = OPTIONS;
        req->status = 204;
        ParseMsg(mesStream, req);
    } else if (cmd == "HEAD") {
        FindWantedFile(path, req, &cmd);
        req->method = HEAD;
        ParseGetMsg(mesStream, req);
        GetFile(req);
    } else if (cmd == "POST") {
        req->method = POST;
        req->status = 200;
        ParsePostMsg(mesStream, req);
    } else if (cmd == "DELETE") {
        req->method = DEL;
        FindWantedFile(path, req, &cmd);
        string fullPath = BASE_PATH + req->file.name;
        req->status = remove(fullPath.c_str()) ? 404 : 200;
        ParseMsg(mesStream, req);
    } else if (cmd == "TRACE") {
        req->method = TRACE;
        req->status = 200;
        ParseMsg(mesStream, req);
    } else if (cmd == "PUT") {
        req->method = PUT;
        FindWantedFile(path, req, &cmd);
        ParsePutMsg(mesStream, req);
        GetFile(req);
        string fullPath = BASE_PATH + req->file.name;
        ofstream basicOfstream(fullPath);
        basicOfstream << req->file.content;
        basicOfstream.close();
    } else {
        // internal server error
        req->method = ERR;
        req->status = 501;
    }
}

void removeSocket(int index, struct SocketState *sockets, int *socketsCount) {
    sockets[index].recv = EMPTY;
    sockets[index].send = EMPTY;
    (*socketsCount)--;
}

void sendMessage(int index, struct SocketState *sockets, int *socketsCount) {
    size_t bytesSent = 0;
    string sendBuff;
    double responseTime;
    SOCKET msgSocket = sockets[index].id;
    sockets[index].turnaroundTime = clock();

    responseTime = ((double) sockets[index].turnaroundTime - (double) sockets[index].responseTime) / CLOCKS_PER_SEC;

    if (responseTime <= 120) {
        sendBuff = CreateMessage(sockets[index]);
        bytesSent = send(msgSocket, sendBuff.c_str(), static_cast<int>( sendBuff.size()), 0);

        if (SOCKET_ERROR == bytesSent) {
            cout << "T3 Server: Error at send(): " << WSAGetLastError() << endl;
            return;
        }

        if (sockets[index].len > 0) {
            // if there are more requests
            sockets[index].send = SEND;
        } else {
            sockets[index].send = IDLE;
        }
    } else {
        closesocket(sockets[index].id);
        removeSocket(index, sockets, socketsCount);
        cout << "\nclosing socket: " << sockets[index].id << endl;
    }
}

void AppendLanguageToFileName(Request *req) {
    if (req->file.language != "Not specified") {
        string fileEnding;
        char c;

        do {
            c = req->file.name.back();
            fileEnding.push_back(c);
            req->file.name.pop_back();
        } while (c != '.');

        if (req->file.language == "Hebrew") {
            req->file.name.append("_he");
        } else if (req->file.language == "French") {
            req->file.name.append("_fr");
        } else {
            req->file.name.append("_en");
        }

        size_t size = fileEnding.size();

        for (size_t i = 0; i < size; i++) {
            c = fileEnding.back();
            req->file.name.push_back(c);
            fileEnding.pop_back();
        }
    }
}

void CheckFileExistence(Request *req) {

    string fullPath = BASE_PATH + req->file.name;
    ifstream file(fullPath);

    if (!file.fail()) {
        cout << "File exists" << endl;
        req->status = 200;
    } else {
        cout << "File does not exist" << endl;
        req->status = 404;
    }
}

void GetFileContent(Request *req) {
    string fullPath = BASE_PATH + req->file.name;
    ifstream file(fullPath);

    if (req->status == 200 && req->method == GET) {
        stringstream stream;
        stream << file.rdbuf();
        req->file.content = stream.str();
    }
}

void GetFile(Request *req) {
    AppendLanguageToFileName(req);
    CheckFileExistence(req);
    GetFileContent(req);

    if (req->method != GET) {
        req->status = 201; // PUT
    }
}

