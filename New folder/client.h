#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <cstring>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"  // Change this to server IP in your network
#define TCP_PORT 8080
#define UDP_PORT 8081
#define BUFFER_SIZE 4096

class CampusClient {
private:
    std::string campusName;
    std::string password;
    std::string currentDepartment;
    
    int tcpSocket;
    int udpSocket;
    struct sockaddr_in serverAddr;
    
    bool isConnected;
    bool isRunning;
    
    std::queue<std::string> messageQueue;
    std::mutex queueMutex;

    // Private methods
    void initializeTCPSocket();
    void initializeUDPSocket();
    bool authenticate();
    void sendHeartbeat();
    void receiveMessages();
    void receiveUDPBroadcasts();
    void displayMenu();
    void sendMessage();
    void sendFile();
    void viewMessages();
    void displayReceivedMessage(const std::string& message);

public:
    CampusClient(const std::string& campus, const std::string& pass);
    ~CampusClient();
    void start();
    void stop();
    void run();
};

#endif // CLIENT_H
