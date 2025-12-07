#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctime>

#define TCP_PORT 8080
#define UDP_PORT 8081
#define BUFFER_SIZE 4096
#define MAX_CLIENTS 10

// Campus credentials structure
struct CampusCredentials {
    std::string campusName;
    std::string password;
};

// Client information structure
struct ClientInfo {
    int tcpSocket;
    std::string campusName;
    std::string ipAddress;
    time_t lastHeartbeat;
    bool isActive;
};

class CentralServer {
private:
    int tcpSocket;
    int udpSocket;
    std::map<std::string, ClientInfo> connectedCampuses;
    std::map<std::string, std::string> campusCredentials;
    std::mutex clientMutex;
    bool isRunning;

    // Private methods
    void initializeTCPSocket();
    void initializeUDPSocket();
    void loadCredentials();
    bool authenticateClient(const std::string& campusName, const std::string& password);
    void handleTCPClient(int clientSocket, std::string clientIP);
    void handleUDPMessages();
    void monitorHeartbeats();
    void parseAndRouteMessage(const std::string& message, const std::string& sourceCampus);
    void broadcastUDPMessage(const std::string& message);
    void displayConnectedCampuses();
    void adminConsole();

public:
    CentralServer();
    ~CentralServer();
    void start();
    void stop();
    void logEvent(const std::string& event);
};

#endif // SERVER_H
