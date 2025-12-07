#include "server.h"
#include <sstream>
#include <algorithm>
#include <iomanip>

CentralServer::CentralServer() : tcpSocket(-1), udpSocket(-1), isRunning(false) {
    loadCredentials();
}

CentralServer::~CentralServer() {
    stop();
}

void CentralServer::loadCredentials() {
    // Initialize campus credentials (Campus:Password)
    campusCredentials["LAHORE"] = "NU-LHR-123";
    campusCredentials["KARACHI"] = "NU-KHI-123";
    campusCredentials["PESHAWAR"] = "NU-PWR-123";
    campusCredentials["CFD"] = "NU-CFD-123";
    campusCredentials["MULTAN"] = "NU-MLN-123";
    
    logEvent("Campus credentials loaded successfully");
}

void CentralServer::initializeTCPSocket() {
    tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpSocket < 0) {
        throw std::runtime_error("Failed to create TCP socket");
    }

    int opt = 1;
    setsockopt(tcpSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(TCP_PORT);

    if (bind(tcpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        throw std::runtime_error("TCP bind failed");
    }

    if (listen(tcpSocket, MAX_CLIENTS) < 0) {
        throw std::runtime_error("TCP listen failed");
    }

    logEvent("TCP socket initialized on port " + std::to_string(TCP_PORT));
}

void CentralServer::initializeUDPSocket() {
    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket < 0) {
        throw std::runtime_error("Failed to create UDP socket");
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(UDP_PORT);

    if (bind(udpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        throw std::runtime_error("UDP bind failed");
    }

    logEvent("UDP socket initialized on port " + std::to_string(UDP_PORT));
}

bool CentralServer::authenticateClient(const std::string& campusName, const std::string& password) {
    auto it = campusCredentials.find(campusName);
    if (it != campusCredentials.end() && it->second == password) {
        return true;
    }
    return false;
}

void CentralServer::handleTCPClient(int clientSocket, std::string clientIP) {
    char buffer[BUFFER_SIZE];
    std::string campusName;

    // Receive authentication message
    memset(buffer, 0, BUFFER_SIZE);
    int bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
    
    if (bytesRead <= 0) {
        close(clientSocket);
        return;
    }

    std::string authMsg(buffer);
    
    // Parse authentication: "AUTH:Campus:LAHORE,Pass:NU-LHR-123"
    size_t campusPos = authMsg.find("Campus:");
    size_t passPos = authMsg.find("Pass:");
    
    if (campusPos != std::string::npos && passPos != std::string::npos) {
        campusName = authMsg.substr(campusPos + 7, passPos - campusPos - 8);
        std::string password = authMsg.substr(passPos + 5);
        
        // Remove any trailing whitespace
        campusName.erase(campusName.find_last_not_of(" \n\r\t") + 1);
        password.erase(password.find_last_not_of(" \n\r\t") + 1);

        if (authenticateClient(campusName, password)) {
            std::string response = "AUTH:SUCCESS";
            send(clientSocket, response.c_str(), response.length(), 0);
            
            // Store client info
            {
                std::lock_guard<std::mutex> lock(clientMutex);
                connectedCampuses[campusName] = {clientSocket, campusName, clientIP, time(nullptr), true};
            }
            
            logEvent("Campus " + campusName + " authenticated successfully from " + clientIP);
        } else {
            std::string response = "AUTH:FAILED";
            send(clientSocket, response.c_str(), response.length(), 0);
            logEvent("Authentication failed for campus " + campusName);
            close(clientSocket);
            return;
        }
    } else {
        close(clientSocket);
        return;
    }

    // Handle messages from this client
    while (isRunning) {
        memset(buffer, 0, BUFFER_SIZE);
        bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytesRead <= 0) {
            logEvent("Campus " + campusName + " disconnected");
            break;
        }

        std::string message(buffer);
        logEvent("Message received from " + campusName + ": " + message);
        parseAndRouteMessage(message, campusName);
    }

    // Cleanup
    {
        std::lock_guard<std::mutex> lock(clientMutex);
        connectedCampuses[campusName].isActive = false;
    }
    close(clientSocket);
}

void CentralServer::parseAndRouteMessage(const std::string& message, const std::string& sourceCampus) {
    // Check if it's a file transfer: "FILE:TO:KARACHI|NAME:doc.txt|SIZE:123|DATA:..."
    if (message.find("FILE:TO:") == 0) {
        size_t namePos = message.find("|NAME:");
        if (namePos == std::string::npos) return;
        
        std::string targetCampus = message.substr(8, namePos - 8);
        std::string fileData = message.substr(namePos + 1); // Everything after TO:
        
        // Find target campus socket
        std::lock_guard<std::mutex> lock(clientMutex);
        auto it = connectedCampuses.find(targetCampus);
        
        if (it != connectedCampuses.end() && it->second.isActive) {
            std::string routedFile = "FILE:FROM:" + sourceCampus + "|" + fileData;
            send(it->second.tcpSocket, routedFile.c_str(), routedFile.length(), 0);
            logEvent("File routed from " + sourceCampus + " to " + targetCampus);
        } else {
            logEvent("Target campus " + targetCampus + " not connected for file transfer");
        }
        return;
    }
    
    // Regular message format: "TO:KARACHI|DEPT:Admissions|MSG:Hello from Lahore"
    size_t toPos = message.find("TO:");
    size_t deptPos = message.find("|DEPT:");
    size_t msgPos = message.find("|MSG:");

    if (toPos == std::string::npos || deptPos == std::string::npos || msgPos == std::string::npos) {
        return;
    }

    std::string targetCampus = message.substr(toPos + 3, deptPos - toPos - 3);
    std::string targetDept = message.substr(deptPos + 6, msgPos - deptPos - 6);
    std::string msgContent = message.substr(msgPos + 5);

    // Find target campus socket
    std::lock_guard<std::mutex> lock(clientMutex);
    auto it = connectedCampuses.find(targetCampus);
    
    if (it != connectedCampuses.end() && it->second.isActive) {
        std::string routedMsg = "FROM:" + sourceCampus + "|DEPT:" + targetDept + "|MSG:" + msgContent;
        send(it->second.tcpSocket, routedMsg.c_str(), routedMsg.length(), 0);
        logEvent("Message routed from " + sourceCampus + " to " + targetCampus);
    } else {
        logEvent("Target campus " + targetCampus + " not connected");
    }
}

void CentralServer::handleUDPMessages() {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);

    while (isRunning) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytesRead = recvfrom(udpSocket, buffer, BUFFER_SIZE - 1, 0,
                                 (struct sockaddr*)&clientAddr, &addrLen);
        
        if (bytesRead > 0) {
            std::string message(buffer);
            
            // Check if it's a heartbeat message: "HEARTBEAT:LAHORE"
            if (message.find("HEARTBEAT:") != std::string::npos) {
                std::string campusName = message.substr(10);
                campusName.erase(campusName.find_last_not_of(" \n\r\t") + 1);
                
                std::lock_guard<std::mutex> lock(clientMutex);
                if (connectedCampuses.find(campusName) != connectedCampuses.end()) {
                    connectedCampuses[campusName].lastHeartbeat = time(nullptr);
                }
            }
        }
    }
}

void CentralServer::monitorHeartbeats() {
    while (isRunning) {
        sleep(15); // Check every 15 seconds
        
        std::lock_guard<std::mutex> lock(clientMutex);
        time_t currentTime = time(nullptr);
        
        for (auto& campus : connectedCampuses) {
            if (campus.second.isActive) {
                int timeSinceHeartbeat = difftime(currentTime, campus.second.lastHeartbeat);
                if (timeSinceHeartbeat > 30) {
                    logEvent("WARNING: No heartbeat from " + campus.first + " for " + 
                            std::to_string(timeSinceHeartbeat) + " seconds");
                }
            }
        }
    }
}

void CentralServer::broadcastUDPMessage(const std::string& message) {
    std::string broadcastMsg = "BROADCAST:" + message;
    
    std::lock_guard<std::mutex> lock(clientMutex);
    
    for (const auto& campus : connectedCampuses) {
        if (campus.second.isActive) {
            // Send directly to the client's TCP socket as a special message
            std::string tcpBroadcast = "BROADCAST:" + message;
            send(campus.second.tcpSocket, tcpBroadcast.c_str(), tcpBroadcast.length(), 0);
        }
    }
    
    logEvent("Broadcast message sent to all campuses");
}

void CentralServer::displayConnectedCampuses() {
    std::lock_guard<std::mutex> lock(clientMutex);
    
    std::cout << "\n========== Connected Campuses ==========\n";
    std::cout << std::left << std::setw(15) << "Campus" 
              << std::setw(20) << "IP Address" 
              << std::setw(10) << "Status" << "\n";
    std::cout << "----------------------------------------\n";
    
    for (const auto& campus : connectedCampuses) {
        std::cout << std::left << std::setw(15) << campus.first
                  << std::setw(20) << campus.second.ipAddress
                  << std::setw(10) << (campus.second.isActive ? "ONLINE" : "OFFLINE") << "\n";
    }
    std::cout << "========================================\n\n";
}

void CentralServer::adminConsole() {
    std::cout << "\n===== ADMIN CONSOLE STARTED =====\n";
    std::cout << "Commands:\n";
    std::cout << "  1. View connected campuses\n";
    std::cout << "  2. Broadcast message\n";
    std::cout << "  3. Exit\n";
    std::cout << "=================================\n\n";

    std::string input;
    while (isRunning) {
        std::cout << "Admin> ";
        std::getline(std::cin, input);

        if (input == "1") {
            displayConnectedCampuses();
        } else if (input == "2") {
            std::cout << "Enter broadcast message: ";
            std::string msg;
            std::getline(std::cin, msg);
            broadcastUDPMessage(msg);
        } else if (input == "3") {
            stop();
            break;
        }
    }
}

void CentralServer::start() {
    try {
        isRunning = true;
        
        initializeTCPSocket();
        initializeUDPSocket();
        
        logEvent("Central Server (ISLAMABAD) started successfully");

        // Start UDP handler thread
        std::thread udpThread(&CentralServer::handleUDPMessages, this);
        udpThread.detach();

        // Start heartbeat monitor thread
        std::thread heartbeatThread(&CentralServer::monitorHeartbeats, this);
        heartbeatThread.detach();

        // Start admin console thread
        std::thread adminThread(&CentralServer::adminConsole, this);
        adminThread.detach();

        // Accept TCP connections
        while (isRunning) {
            struct sockaddr_in clientAddr;
            socklen_t addrLen = sizeof(clientAddr);
            
            int clientSocket = accept(tcpSocket, (struct sockaddr*)&clientAddr, &addrLen);
            
            if (clientSocket < 0) {
                if (isRunning) {
                    logEvent("Error accepting connection");
                }
                continue;
            }

            std::string clientIP = inet_ntoa(clientAddr.sin_addr);
            logEvent("New connection from " + clientIP);

            // Handle client in a new thread
            std::thread clientThread(&CentralServer::handleTCPClient, this, clientSocket, clientIP);
            clientThread.detach();
        }
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
    }
}

void CentralServer::stop() {
    isRunning = false;
    
    if (tcpSocket >= 0) {
        close(tcpSocket);
    }
    if (udpSocket >= 0) {
        close(udpSocket);
    }
    
    logEvent("Central Server shutting down");
}

void CentralServer::logEvent(const std::string& event) {
    time_t now = time(nullptr);
    char timeStr[26];
    ctime_r(&now, timeStr);
    timeStr[24] = '\0'; // Remove newline
    
    std::cout << "[" << timeStr << "] " << event << std::endl;
}

// Main function
int main() {
    std::cout << "========================================\n";
    std::cout << "   NU-Information Exchange System\n";
    std::cout << "   Central Server - ISLAMABAD Campus\n";
    std::cout << "========================================\n\n";

    CentralServer server;
    server.start();

    return 0;
}
