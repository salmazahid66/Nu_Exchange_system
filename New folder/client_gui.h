#ifndef CLIENT_GUI_H
#define CLIENT_GUI_H

#include <gtk/gtk.h>
#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <cstring>
#include <fstream>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define TCP_PORT 8080
#define UDP_PORT 8081
#define BUFFER_SIZE 4096

class CampusClientGUI {
private:
    // Network members
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
    
    // GTK+ widgets
    GtkWidget *window;
    GtkWidget *mainBox;
    GtkWidget *headerLabel;
    GtkWidget *statusLabel;
    GtkWidget *notebook;
    
    // Send Message tab
    GtkWidget *targetCampusCombo;
    GtkWidget *targetDeptEntry;
    GtkWidget *messageTextView;
    GtkWidget *sendButton;
    
    // Send File tab
    GtkWidget *fileTargetCombo;
    GtkWidget *fileChooserButton;
    GtkWidget *sendFileButton;
    
    // Received Messages tab
    GtkWidget *messagesTextView;
    GtkWidget *refreshButton;
    
    // Connection tab
    GtkWidget *campusCombo;
    GtkWidget *passwordEntry;
    GtkWidget *connectButton;
    GtkWidget *disconnectButton;
    
    // Private methods
    void initializeTCPSocket();
    void initializeUDPSocket();
    bool authenticate();
    void sendHeartbeat();
    void receiveMessages();
    void processReceivedMessage(const std::string& message);
    
    static gboolean updateMessagesCallback(gpointer data);
    static void onSendMessageClicked(GtkWidget *widget, gpointer data);
    static void onSendFileClicked(GtkWidget *widget, gpointer data);
    static void onRefreshClicked(GtkWidget *widget, gpointer data);
    static void onConnectClicked(GtkWidget *widget, gpointer data);
    static void onDisconnectClicked(GtkWidget *widget, gpointer data);
    static void onWindowDestroy(GtkWidget *widget, gpointer data);

public:
    CampusClientGUI();
    ~CampusClientGUI();
    
    void createGUI();
    void start(const std::string& campus, const std::string& pass);
    void stop();
    void run();
    
    void appendToMessageView(const std::string& message);
    void updateStatus(const std::string& status);
};

#endif // CLIENT_GUI_H
