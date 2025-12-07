#include "client.h"
#include <sstream>
#include <iomanip>
#include <algorithm>  // Required for std::transform

CampusClient::CampusClient(const std::string& campus, const std::string& pass)
    : campusName(campus), password(pass), tcpSocket(-1), udpSocket(-1),
      isConnected(false), isRunning(false), currentDepartment("General") {
}

CampusClient::~CampusClient() {
    stop();
}

void CampusClient::initializeTCPSocket() {
    tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpSocket < 0) {
        throw std::runtime_error("Failed to create TCP socket");
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(TCP_PORT);
    
    if (inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid server address");
    }

    if (connect(tcpSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        throw std::runtime_error("Connection to server failed");
    }

    std::cout << "[INFO] TCP connection established with server\n";
}

void CampusClient::initializeUDPSocket() {
    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket < 0) {
        throw std::runtime_error("Failed to create UDP socket");
    }

    // Enable broadcast receiving
    int broadcast = 1;
    setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    
    // Bind to receive broadcasts on a unique port per client
    struct sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons(0); // Let OS assign a port

    if (bind(udpSocket, (struct sockaddr*)&localAddr, sizeof(localAddr)) < 0) {
        std::cerr << "[WARNING] UDP bind failed, broadcasts may not work\n";
    }

    std::cout << "[INFO] UDP socket initialized\n";
}

bool CampusClient::authenticate() {
    std::string authMsg = "AUTH:Campus:" + campusName + ",Pass:" + password;
    
    if (send(tcpSocket, authMsg.c_str(), authMsg.length(), 0) < 0) {
        std::cerr << "[ERROR] Failed to send authentication\n";
        return false;
    }

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    
    int bytesRead = recv(tcpSocket, buffer, BUFFER_SIZE - 1, 0);
    if (bytesRead <= 0) {
        std::cerr << "[ERROR] No response from server\n";
        return false;
    }

    std::string response(buffer);
    if (response == "AUTH:SUCCESS") {
        std::cout << "[SUCCESS] Authentication successful for " << campusName << " campus\n";
        isConnected = true;
        return true;
    } else {
        std::cerr << "[ERROR] Authentication failed\n";
        return false;
    }
}

void CampusClient::sendHeartbeat() {
    struct sockaddr_in udpServerAddr;
    memset(&udpServerAddr, 0, sizeof(udpServerAddr));
    udpServerAddr.sin_family = AF_INET;
    udpServerAddr.sin_port = htons(UDP_PORT);
    inet_pton(AF_INET, SERVER_IP, &udpServerAddr.sin_addr);

    while (isRunning && isConnected) {
        std::string heartbeat = "HEARTBEAT:" + campusName;
        
        sendto(udpSocket, heartbeat.c_str(), heartbeat.length(), 0,
               (struct sockaddr*)&udpServerAddr, sizeof(udpServerAddr));
        
        sleep(10); // Send heartbeat every 10 seconds
    }
}

void CampusClient::receiveMessages() {
    char buffer[BUFFER_SIZE];
    
    while (isRunning && isConnected) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytesRead = recv(tcpSocket, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytesRead <= 0) {
            std::cout << "[INFO] Connection lost with server\n";
            isConnected = false;
            break;
        }

        std::string message(buffer);
        
        // Check if it's a broadcast message
        if (message.find("BROADCAST:") == 0) {
            std::string broadcastMsg = message.substr(10);
            std::cout << "\n╔════════════════════════════════════════╗\n";
            std::cout << "║      SYSTEM BROADCAST MESSAGE          ║\n";
            std::cout << "╠════════════════════════════════════════╣\n";
            std::cout << "║ " << broadcastMsg << std::endl;
            std::cout << "╚════════════════════════════════════════╝\n";
            std::cout << "Campus " << campusName << "> ";
            std::cout.flush();
        } 
        // Check if it's a file transfer
        else if (message.find("FILE:FROM:") == 0) {
            // Parse file message: "FILE:FROM:LAHORE|NAME:doc.txt|SIZE:123|DATA:..."
            size_t namePos = message.find("|NAME:");
            size_t sizePos = message.find("|SIZE:");
            size_t dataPos = message.find("|DATA:");
            
            if (namePos != std::string::npos && sizePos != std::string::npos && dataPos != std::string::npos) {
                std::string fromCampus = message.substr(10, namePos - 10);
                std::string filename = message.substr(namePos + 6, sizePos - namePos - 6);
                std::string sizeStr = message.substr(sizePos + 6, dataPos - sizePos - 6);
                std::string encodedData = message.substr(dataPos + 6);
                
                // Decode hex data
                std::string fileContent;
                for (size_t i = 0; i < encodedData.length(); i += 2) {
                    std::string byteStr = encodedData.substr(i, 2);
                    char byte = (char)strtol(byteStr.c_str(), nullptr, 16);
                    fileContent += byte;
                }
                
                // Save file with prefix
                std::string savedFilename = "received_" + filename;
                std::ofstream outFile(savedFilename, std::ios::binary);
                outFile.write(fileContent.c_str(), fileContent.size());
                outFile.close();
                
                std::cout << "\n╔════════════════════════════════════════╗\n";
                std::cout << "║         FILE RECEIVED                  ║\n";
                std::cout << "╠════════════════════════════════════════╣\n";
                std::cout << "║ From: " << fromCampus << std::endl;
                std::cout << "║ File: " << filename << std::endl;
                std::cout << "║ Size: " << sizeStr << " bytes\n";
                std::cout << "║ Saved as: " << savedFilename << std::endl;
                std::cout << "╚════════════════════════════════════════╝\n";
                std::cout << "Campus " << campusName << "> ";
                std::cout.flush();
            }
        }
        else {
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                messageQueue.push(message);
            }
            
            std::cout << "\n[NEW MESSAGE RECEIVED] - Check messages to view\n";
            std::cout << "Campus " << campusName << "> ";
            std::cout.flush();
        }
    }
}

void CampusClient::receiveUDPBroadcasts() {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in fromAddr;
    socklen_t addrLen = sizeof(fromAddr);
    
    // Bind UDP socket to receive broadcasts
    struct sockaddr_in localAddr;
    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons(UDP_PORT + 1); // Use different port for receiving

    while (isRunning) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytesRead = recvfrom(udpSocket, buffer, BUFFER_SIZE - 1, 0,
                                 (struct sockaddr*)&fromAddr, &addrLen);
        
        if (bytesRead > 0) {
            std::string message(buffer);
            
            if (message.find("BROADCAST:") != std::string::npos) {
                std::string broadcastMsg = message.substr(10);
                std::cout << "\n╔════════════════════════════════════════╗\n";
                std::cout << "║      SYSTEM BROADCAST MESSAGE          ║\n";
                std::cout << "╠════════════════════════════════════════╣\n";
                std::cout << "║ " << broadcastMsg << std::endl;
                std::cout << "╚════════════════════════════════════════╝\n";
                std::cout << "Campus " << campusName << "> ";
                std::cout.flush();
            }
        }
    }
}

void CampusClient::displayReceivedMessage(const std::string& message) {
    // Message format: "FROM:LAHORE|DEPT:Admissions|MSG:Hello"
    size_t fromPos = message.find("FROM:");
    size_t deptPos = message.find("|DEPT:");
    size_t msgPos = message.find("|MSG:");

    if (fromPos != std::string::npos && deptPos != std::string::npos && msgPos != std::string::npos) {
        std::string fromCampus = message.substr(fromPos + 5, deptPos - fromPos - 5);
        std::string dept = message.substr(deptPos + 6, msgPos - deptPos - 6);
        std::string msgContent = message.substr(msgPos + 5);

        std::cout << "\n┌────────────────────────────────────────┐\n";
        std::cout << "│ From Campus: " << std::setw(24) << std::left << fromCampus << "│\n";
        std::cout << "│ Department:  " << std::setw(24) << std::left << dept << "│\n";
        std::cout << "├────────────────────────────────────────┤\n";
        std::cout << "│ Message:                               │\n";
        std::cout << "│ " << std::setw(38) << std::left << msgContent << "│\n";
        std::cout << "└────────────────────────────────────────┘\n";
    }
}

void CampusClient::displayMenu() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   " << campusName << " CAMPUS - NU Information Exchange   \n";
    std::cout << "╠════════════════════════════════════════╣\n";
    std::cout << "║ Current Department: " << currentDepartment << std::endl;
    std::cout << "╠════════════════════════════════════════╣\n";
    std::cout << "║ 1. Send Message to Another Campus     ║\n";
    std::cout << "║ 2. Send File to Another Campus        ║\n";
    std::cout << "║ 3. View Received Messages              ║\n";
    std::cout << "║ 4. Change Department                   ║\n";
    std::cout << "║ 5. Exit                                ║\n";
    std::cout << "╚════════════════════════════════════════╝\n";
}

void CampusClient::sendMessage() {
    std::string targetCampus, targetDept, message;
    
    std::cout << "\nAvailable Campuses: LAHORE, KARACHI, PESHAWAR, CFD, MULTAN\n";
    std::cout << "Enter target campus: ";
    std::getline(std::cin, targetCampus);
    
    // Convert to uppercase
    std::transform(targetCampus.begin(), targetCampus.end(), targetCampus.begin(), ::toupper);
    
    std::cout << "Enter target department (Admissions/Academics/IT/Sports): ";
    std::getline(std::cin, targetDept);
    
    std::cout << "Enter your message: ";
    std::getline(std::cin, message);
    
    // Format: "TO:KARACHI|DEPT:Admissions|MSG:Hello from Lahore"
    std::string fullMessage = "TO:" + targetCampus + "|DEPT:" + targetDept + "|MSG:" + message;
    
    if (send(tcpSocket, fullMessage.c_str(), fullMessage.length(), 0) < 0) {
        std::cerr << "[ERROR] Failed to send message\n";
    } else {
        std::cout << "[SUCCESS] Message sent to " << targetCampus << "\n";
    }
}

void CampusClient::sendFile() {
    std::string targetCampus, filename;
    
    std::cout << "\nAvailable Campuses: LAHORE, KARACHI, PESHAWAR, CFD, MULTAN\n";
    std::cout << "Enter target campus: ";
    std::getline(std::cin, targetCampus);
    
    // Convert to uppercase
    std::transform(targetCampus.begin(), targetCampus.end(), targetCampus.begin(), ::toupper);
    
    std::cout << "Enter filename to send (in current directory): ";
    std::getline(std::cin, filename);
    
    // Open and read file
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Cannot open file: " << filename << "\n";
        return;
    }
    
    // Get file size
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    if (fileSize > 1000000) { // 1MB limit
        std::cerr << "[ERROR] File too large (max 1MB)\n";
        file.close();
        return;
    }
    
    // Read file content
    std::string fileContent;
    fileContent.resize(fileSize);
    file.read(&fileContent[0], fileSize);
    file.close();
    
    // Encode file content in base64-like format (simplified)
    std::string encodedContent;
    for (unsigned char c : fileContent) {
        char hex[3];
        sprintf(hex, "%02X", c);
        encodedContent += hex;
    }
    
    // Format: "FILE:TO:KARACHI|NAME:document.txt|SIZE:1234|DATA:..."
    std::string fileMessage = "FILE:TO:" + targetCampus + 
                              "|NAME:" + filename + 
                              "|SIZE:" + std::to_string(fileSize) + 
                              "|DATA:" + encodedContent;
    
    if (send(tcpSocket, fileMessage.c_str(), fileMessage.length(), 0) < 0) {
        std::cerr << "[ERROR] Failed to send file\n";
    } else {
        std::cout << "[SUCCESS] File '" << filename << "' (" << fileSize << " bytes) sent to " << targetCampus << "\n";
    }
}

void CampusClient::viewMessages() {
    std::lock_guard<std::mutex> lock(queueMutex);
    
    if (messageQueue.empty()) {
        std::cout << "\n[INFO] No new messages\n";
        return;
    }
    
    std::cout << "\n========== Received Messages ==========\n";
    while (!messageQueue.empty()) {
        displayReceivedMessage(messageQueue.front());
        messageQueue.pop();
    }
    std::cout << "======================================\n";
}

void CampusClient::start() {
    try {
        isRunning = true;
        
        std::cout << "\n[INFO] Initializing " << campusName << " Campus Client...\n";
        
        initializeTCPSocket();
        initializeUDPSocket();
        
        if (!authenticate()) {
            throw std::runtime_error("Authentication failed");
        }
        
        // Start background threads
        std::thread heartbeatThread(&CampusClient::sendHeartbeat, this);
        heartbeatThread.detach();
        
        std::thread receiveThread(&CampusClient::receiveMessages, this);
        receiveThread.detach();
        
        std::thread udpThread(&CampusClient::receiveUDPBroadcasts, this);
        udpThread.detach();
        
        std::cout << "[SUCCESS] Client started successfully\n";
        
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        isRunning = false;
    }
}

void CampusClient::run() {
    if (!isConnected || !isRunning) {
        std::cerr << "[ERROR] Client not connected\n";
        return;
    }
    
    std::string choice;
    
    while (isRunning && isConnected) {
        displayMenu();
        std::cout << "\nCampus " << campusName << "> ";
        std::getline(std::cin, choice);
        
        if (choice == "1") {
            sendMessage();
        } else if (choice == "2") {
            sendFile();
        } else if (choice == "3") {
            viewMessages();
        } else if (choice == "4") {
            std::cout << "Enter department name: ";
            std::getline(std::cin, currentDepartment);
            std::cout << "[INFO] Department changed to " << currentDepartment << "\n";
        } else if (choice == "5") {
            std::cout << "[INFO] Disconnecting from server...\n";
            stop();
            break;
        } else {
            std::cout << "[ERROR] Invalid choice\n";
        }
    }
}

void CampusClient::stop() {
    isRunning = false;
    isConnected = false;
    
    if (tcpSocket >= 0) {
        close(tcpSocket);
    }
    if (udpSocket >= 0) {
        close(udpSocket);
    }
    
    std::cout << "[INFO] Client stopped\n";
}

// Main function
int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Usage: ./client <CAMPUS_NAME> <PASSWORD>\n";
        std::cout << "Example: ./client LAHORE NU-LHR-123\n\n";
        std::cout << "Available Campuses:\n";
        std::cout << "  LAHORE    : NU-LHR-123\n";
        std::cout << "  KARACHI   : NU-KHI-123\n";
        std::cout << "  PESHAWAR  : NU-PWR-123\n";
        std::cout << "  CFD       : NU-CFD-123\n";
        std::cout << "  MULTAN    : NU-MLN-123\n";
        return 1;
    }

    std::string campusName = argv[1];
    std::string password = argv[2];
    
    // Convert campus name to uppercase
    std::transform(campusName.begin(), campusName.end(), campusName.begin(), ::toupper);

    std::cout << "========================================\n";
    std::cout << "   NU-Information Exchange System\n";
    std::cout << "   Campus Client - " << campusName << "\n";
    std::cout << "========================================\n";

    CampusClient client(campusName, password);
    client.start();
    
    sleep(1); // Give time for threads to initialize
    
    client.run();

    return 0;
}
