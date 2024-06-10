#include "Server.h"

int main()
{
    fd_set waitRecv, waitSend;
    int nfd;
   
    SocketState* sockets = new SocketState[MAX_SOCKETS](); 
    int socketsCount = 0;

    importLib();

    SOCKET listenSocket = initiateSocket();

    bindSocket(&listenSocket);

    if (SOCKET_ERROR == listen(listenSocket, 5)) 
    {
        cout << "Time Server: Error at listen(): " << WSAGetLastError() << endl;
        closesocket(listenSocket);
        WSACleanup();
        return 0;
    }

    // Add the listen socket to the array of all the sockets
    addSocket(listenSocket, LISTEN, sockets, &socketsCount);

    while (true)
    {
        createSets(&waitRecv, &waitSend, sockets);

       filterUpcomingEvents(&nfd, &waitRecv, &waitSend);

       handleEvents(&nfd, &waitRecv, &waitSend, sockets, &socketsCount);
    }

    finishingUp(listenSocket);

    delete[] sockets;
    return 0;
}

