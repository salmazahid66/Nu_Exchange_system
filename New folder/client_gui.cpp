#include "client_gui.h"

CampusClientGUI::CampusClientGUI() 
    : tcpSocket(-1), udpSocket(-1), isConnected(false), 
      isRunning(false), currentDepartment("General") {
}

CampusClientGUI::~CampusClientGUI() {
    stop();
}

void CampusClientGUI::createGUI() {
    // Create main window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "NU Information Exchange System");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    g_signal_connect(window, "destroy", G_CALLBACK(onWindowDestroy), this);
    
    // Main vertical box
    mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), mainBox);
    
    // Header label
    headerLabel = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(headerLabel), 
        "<span font='16' weight='bold'>FAST-NUCES Campus Client</span>");
    gtk_box_pack_start(GTK_BOX(mainBox), headerLabel, FALSE, FALSE, 5);
    
    // Status label
    statusLabel = gtk_label_new("Status: Disconnected");
    gtk_box_pack_start(GTK_BOX(mainBox), statusLabel, FALSE, FALSE, 5);
    
    // Create notebook (tabbed interface)
    notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(mainBox), notebook, TRUE, TRUE, 5);
    
    // === TAB 1: Connection ===
    GtkWidget *connectionBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(connectionBox), 10);
    
    GtkWidget *campusLabel = gtk_label_new("Select Campus:");
    gtk_box_pack_start(GTK_BOX(connectionBox), campusLabel, FALSE, FALSE, 0);
    
    campusCombo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(campusCombo), "LAHORE");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(campusCombo), "KARACHI");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(campusCombo), "PESHAWAR");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(campusCombo), "CFD");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(campusCombo), "MULTAN");
    gtk_combo_box_set_active(GTK_COMBO_BOX(campusCombo), 0);
    gtk_box_pack_start(GTK_BOX(connectionBox), campusCombo, FALSE, FALSE, 0);
    
    GtkWidget *passwordLabel = gtk_label_new("Password:");
    gtk_box_pack_start(GTK_BOX(connectionBox), passwordLabel, FALSE, FALSE, 0);
    
    passwordEntry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(passwordEntry), FALSE);
    gtk_entry_set_text(GTK_ENTRY(passwordEntry), "NU-LHR-123");
    gtk_box_pack_start(GTK_BOX(connectionBox), passwordEntry, FALSE, FALSE, 0);
    
    GtkWidget *buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    connectButton = gtk_button_new_with_label("Connect");
    disconnectButton = gtk_button_new_with_label("Disconnect");
    gtk_widget_set_sensitive(disconnectButton, FALSE);
    
    g_signal_connect(connectButton, "clicked", G_CALLBACK(onConnectClicked), this);
    g_signal_connect(disconnectButton, "clicked", G_CALLBACK(onDisconnectClicked), this);
    
    gtk_box_pack_start(GTK_BOX(buttonBox), connectButton, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(buttonBox), disconnectButton, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(connectionBox), buttonBox, FALSE, FALSE, 10);
    
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), connectionBox, 
                             gtk_label_new("Connection"));
    
    // === TAB 2: Send Message ===
    GtkWidget *sendBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(sendBox), 10);
    
    GtkWidget *targetLabel = gtk_label_new("Target Campus:");
    gtk_box_pack_start(GTK_BOX(sendBox), targetLabel, FALSE, FALSE, 0);
    
    targetCampusCombo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(targetCampusCombo), "LAHORE");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(targetCampusCombo), "KARACHI");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(targetCampusCombo), "PESHAWAR");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(targetCampusCombo), "CFD");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(targetCampusCombo), "MULTAN");
    gtk_combo_box_set_active(GTK_COMBO_BOX(targetCampusCombo), 0);
    gtk_box_pack_start(GTK_BOX(sendBox), targetCampusCombo, FALSE, FALSE, 0);
    
    GtkWidget *deptLabel = gtk_label_new("Target Department:");
    gtk_box_pack_start(GTK_BOX(sendBox), deptLabel, FALSE, FALSE, 0);
    
    targetDeptEntry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(targetDeptEntry), "Admissions");
    gtk_box_pack_start(GTK_BOX(sendBox), targetDeptEntry, FALSE, FALSE, 0);
    
    GtkWidget *msgLabel = gtk_label_new("Message:");
    gtk_box_pack_start(GTK_BOX(sendBox), msgLabel, FALSE, FALSE, 0);
    
    GtkWidget *scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledWindow),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    messageTextView = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(messageTextView), GTK_WRAP_WORD);
    gtk_container_add(GTK_CONTAINER(scrolledWindow), messageTextView);
    gtk_box_pack_start(GTK_BOX(sendBox), scrolledWindow, TRUE, TRUE, 0);
    
    sendButton = gtk_button_new_with_label("Send Message");
    gtk_widget_set_sensitive(sendButton, FALSE);
    g_signal_connect(sendButton, "clicked", G_CALLBACK(onSendMessageClicked), this);
    gtk_box_pack_start(GTK_BOX(sendBox), sendButton, FALSE, FALSE, 0);
    
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), sendBox, 
                             gtk_label_new("Send Message"));
    
    // === TAB 3: Send File ===
    GtkWidget *fileBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(fileBox), 10);
    
    GtkWidget *fileTargetLabel = gtk_label_new("Target Campus:");
    gtk_box_pack_start(GTK_BOX(fileBox), fileTargetLabel, FALSE, FALSE, 0);
    
    fileTargetCombo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(fileTargetCombo), "LAHORE");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(fileTargetCombo), "KARACHI");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(fileTargetCombo), "PESHAWAR");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(fileTargetCombo), "CFD");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(fileTargetCombo), "MULTAN");
    gtk_combo_box_set_active(GTK_COMBO_BOX(fileTargetCombo), 0);
    gtk_box_pack_start(GTK_BOX(fileBox), fileTargetCombo, FALSE, FALSE, 0);
    
    GtkWidget *fileLabel = gtk_label_new("Select File:");
    gtk_box_pack_start(GTK_BOX(fileBox), fileLabel, FALSE, FALSE, 0);
    
    fileChooserButton = gtk_file_chooser_button_new("Select a File", 
                                                     GTK_FILE_CHOOSER_ACTION_OPEN);
    gtk_box_pack_start(GTK_BOX(fileBox), fileChooserButton, FALSE, FALSE, 0);
    
    sendFileButton = gtk_button_new_with_label("Send File");
    gtk_widget_set_sensitive(sendFileButton, FALSE);
    g_signal_connect(sendFileButton, "clicked", G_CALLBACK(onSendFileClicked), this);
    gtk_box_pack_start(GTK_BOX(fileBox), sendFileButton, FALSE, FALSE, 10);
    
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), fileBox, 
                             gtk_label_new("Send File"));
    
    // === TAB 4: Received Messages ===
    GtkWidget *receivedBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(receivedBox), 10);
    
    GtkWidget *receivedLabel = gtk_label_new("Received Messages:");
    gtk_box_pack_start(GTK_BOX(receivedBox), receivedLabel, FALSE, FALSE, 0);
    
    GtkWidget *msgScrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(msgScrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    messagesTextView = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(messagesTextView), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(messagesTextView), GTK_WRAP_WORD);
    gtk_container_add(GTK_CONTAINER(msgScrolled), messagesTextView);
    gtk_box_pack_start(GTK_BOX(receivedBox), msgScrolled, TRUE, TRUE, 0);
    
    refreshButton = gtk_button_new_with_label("Refresh");
    g_signal_connect(refreshButton, "clicked", G_CALLBACK(onRefreshClicked), this);
    gtk_box_pack_start(GTK_BOX(receivedBox), refreshButton, FALSE, FALSE, 0);
    
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), receivedBox, 
                             gtk_label_new("Messages"));
    
    gtk_widget_show_all(window);
}

void CampusClientGUI::initializeTCPSocket() {
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
}

void CampusClientGUI::initializeUDPSocket() {
    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSocket < 0) {
        throw std::runtime_error("Failed to create UDP socket");
    }
}

bool CampusClientGUI::authenticate() {
    std::string authMsg = "AUTH:Campus:" + campusName + ",Pass:" + password;
    
    if (send(tcpSocket, authMsg.c_str(), authMsg.length(), 0) < 0) {
        return false;
    }

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    
    int bytesRead = recv(tcpSocket, buffer, BUFFER_SIZE - 1, 0);
    if (bytesRead <= 0) {
        return false;
    }

    std::string response(buffer);
    if (response == "AUTH:SUCCESS") {
        isConnected = true;
        return true;
    }
    return false;
}

void CampusClientGUI::sendHeartbeat() {
    struct sockaddr_in udpServerAddr;
    memset(&udpServerAddr, 0, sizeof(udpServerAddr));
    udpServerAddr.sin_family = AF_INET;
    udpServerAddr.sin_port = htons(UDP_PORT);
    inet_pton(AF_INET, SERVER_IP, &udpServerAddr.sin_addr);

    while (isRunning && isConnected) {
        std::string heartbeat = "HEARTBEAT:" + campusName;
        sendto(udpSocket, heartbeat.c_str(), heartbeat.length(), 0,
               (struct sockaddr*)&udpServerAddr, sizeof(udpServerAddr));
        sleep(10);
    }
}

void CampusClientGUI::receiveMessages() {
    char buffer[BUFFER_SIZE];
    
    while (isRunning && isConnected) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytesRead = recv(tcpSocket, buffer, BUFFER_SIZE - 1, 0);
        
        if (bytesRead <= 0) {
            isConnected = false;
            g_idle_add(updateMessagesCallback, this);
            break;
        }

        std::string message(buffer);
        processReceivedMessage(message);
    }
}

void CampusClientGUI::processReceivedMessage(const std::string& message) {
    std::lock_guard<std::mutex> lock(queueMutex);
    messageQueue.push(message);
    g_idle_add(updateMessagesCallback, this);
}

gboolean CampusClientGUI::updateMessagesCallback(gpointer data) {
    CampusClientGUI *client = static_cast<CampusClientGUI*>(data);
    
    std::lock_guard<std::mutex> lock(client->queueMutex);
    while (!client->messageQueue.empty()) {
        std::string message = client->messageQueue.front();
        client->messageQueue.pop();
        
        if (message.find("BROADCAST:") == 0) {
            client->appendToMessageView("\n=== BROADCAST ===\n" + 
                                       message.substr(10) + "\n================\n");
        } else if (message.find("FILE:FROM:") == 0) {
            // Handle file reception
            size_t namePos = message.find("|NAME:");
            size_t sizePos = message.find("|SIZE:");
            size_t dataPos = message.find("|DATA:");
            
            if (namePos != std::string::npos && sizePos != std::string::npos) {
                std::string from = message.substr(10, namePos - 10);
                std::string filename = message.substr(namePos + 6, sizePos - namePos - 6);
                std::string sizeStr = message.substr(sizePos + 6, dataPos - sizePos - 6);
                
                if (dataPos != std::string::npos) {
                    std::string encodedData = message.substr(dataPos + 6);
                    
                    // Decode and save file
                    std::string fileContent;
                    for (size_t i = 0; i < encodedData.length(); i += 2) {
                        std::string byteStr = encodedData.substr(i, 2);
                        char byte = (char)strtol(byteStr.c_str(), nullptr, 16);
                        fileContent += byte;
                    }
                    
                    std::string savedFilename = "received_" + filename;
                    std::ofstream outFile(savedFilename, std::ios::binary);
                    outFile.write(fileContent.c_str(), fileContent.size());
                    outFile.close();
                    
                    client->appendToMessageView("\n=== FILE RECEIVED ===\n");
                    client->appendToMessageView("From: " + from + "\n");
                    client->appendToMessageView("File: " + filename + "\n");
                    client->appendToMessageView("Size: " + sizeStr + " bytes\n");
                    client->appendToMessageView("Saved as: " + savedFilename + "\n");
                    client->appendToMessageView("====================\n");
                }
            }
        } else {
            client->appendToMessageView(message + "\n");
        }
    }
    
    return FALSE;
}

void CampusClientGUI::onConnectClicked(GtkWidget *widget, gpointer data) {
    CampusClientGUI *client = static_cast<CampusClientGUI*>(data);
    
    gchar *campus = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(client->campusCombo));
    const gchar *password = gtk_entry_get_text(GTK_ENTRY(client->passwordEntry));
    
    client->start(std::string(campus), std::string(password));
    
    g_free(campus);
}

void CampusClientGUI::onDisconnectClicked(GtkWidget *widget, gpointer data) {
    CampusClientGUI *client = static_cast<CampusClientGUI*>(data);
    client->stop();
}

void CampusClientGUI::onSendMessageClicked(GtkWidget *widget, gpointer data) {
    CampusClientGUI *client = static_cast<CampusClientGUI*>(data);
    
    if (!client->isConnected) return;
    
    gchar *target = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(client->targetCampusCombo));
    const gchar *dept = gtk_entry_get_text(GTK_ENTRY(client->targetDeptEntry));
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(client->messageTextView));
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    gchar *messageText = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    
    std::string fullMessage = "TO:" + std::string(target) + 
                              "|DEPT:" + std::string(dept) + 
                              "|MSG:" + std::string(messageText);
    
    send(client->tcpSocket, fullMessage.c_str(), fullMessage.length(), 0);
    
    gtk_text_buffer_set_text(buffer, "", 0);
    
    g_free(target);
    g_free(messageText);
    
    client->updateStatus("Message sent successfully");
}

void CampusClientGUI::onSendFileClicked(GtkWidget *widget, gpointer data) {
    CampusClientGUI *client = static_cast<CampusClientGUI*>(data);
    
    if (!client->isConnected) return;
    
    gchar *target = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(client->fileTargetCombo));
    gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(client->fileChooserButton));
    
    if (filename) {
        // Read file
        std::ifstream file(filename, std::ios::binary);
        if (file.is_open()) {
            file.seekg(0, std::ios::end);
            size_t fileSize = file.tellg();
            file.seekg(0, std::ios::beg);
            
            if (fileSize <= 1000000) {
                std::string fileContent;
                fileContent.resize(fileSize);
                file.read(&fileContent[0], fileSize);
                file.close();
                
                // Encode
                std::string encodedContent;
                for (unsigned char c : fileContent) {
                    char hex[3];
                    sprintf(hex, "%02X", c);
                    encodedContent += hex;
                }
                
                // Get just the filename without path
                std::string justFilename = filename;
                size_t lastSlash = justFilename.find_last_of("/\\");
                if (lastSlash != std::string::npos) {
                    justFilename = justFilename.substr(lastSlash + 1);
                }
                
                std::string fileMessage = "FILE:TO:" + std::string(target) + 
                                          "|NAME:" + justFilename + 
                                          "|SIZE:" + std::to_string(fileSize) + 
                                          "|DATA:" + encodedContent;
                
                send(client->tcpSocket, fileMessage.c_str(), fileMessage.length(), 0);
                
                client->updateStatus("File sent: " + justFilename);
            } else {
                client->updateStatus("Error: File too large (max 1MB)");
            }
        } else {
            client->updateStatus("Error: Cannot open file");
        }
        g_free(filename);
    }
    
    g_free(target);
}

void CampusClientGUI::onRefreshClicked(GtkWidget *widget, gpointer data) {
    // Messages are automatically updated via callback
    CampusClientGUI *client = static_cast<CampusClientGUI*>(data);
    client->updateStatus("Messages refreshed");
}

void CampusClientGUI::onWindowDestroy(GtkWidget *widget, gpointer data) {
    CampusClientGUI *client = static_cast<CampusClientGUI*>(data);
    client->stop();
    gtk_main_quit();
}

void CampusClientGUI::start(const std::string& campus, const std::string& pass) {
    try {
        campusName = campus;
        password = pass;
        isRunning = true;
        
        initializeTCPSocket();
        initializeUDPSocket();
        
        if (!authenticate()) {
            updateStatus("Authentication failed!");
            isRunning = false;
            return;
        }
        
        updateStatus("Connected to server as " + campusName);
        
        // Enable buttons
        gtk_widget_set_sensitive(sendButton, TRUE);
        gtk_widget_set_sensitive(sendFileButton, TRUE);
        gtk_widget_set_sensitive(connectButton, FALSE);
        gtk_widget_set_sensitive(disconnectButton, TRUE);
        
        // Start background threads
        std::thread heartbeatThread(&CampusClientGUI::sendHeartbeat, this);
        heartbeatThread.detach();
        
        std::thread receiveThread(&CampusClientGUI::receiveMessages, this);
        receiveThread.detach();
        
    } catch (const std::exception& e) {
        updateStatus(std::string("Error: ") + e.what());
        isRunning = false;
    }
}

void CampusClientGUI::stop() {
    isRunning = false;
    isConnected = false;
    
    if (tcpSocket >= 0) {
        close(tcpSocket);
        tcpSocket = -1;
    }
    if (udpSocket >= 0) {
        close(udpSocket);
        udpSocket = -1;
    }
    
    updateStatus("Disconnected from server");
    
    gtk_widget_set_sensitive(sendButton, FALSE);
    gtk_widget_set_sensitive(sendFileButton, FALSE);
    gtk_widget_set_sensitive(connectButton, TRUE);
    gtk_widget_set_sensitive(disconnectButton, FALSE);
}

void CampusClientGUI::run() {
    gtk_main();
}

void CampusClientGUI::appendToMessageView(const std::string& message) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(messagesTextView));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);
    gtk_text_buffer_insert(buffer, &end, message.c_str(), -1);
    
    // Auto-scroll to bottom
    GtkTextMark *mark = gtk_text_buffer_get_insert(buffer);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(messagesTextView), mark, 0.0, FALSE, 0.0, 0.0);
}

void CampusClientGUI::updateStatus(const std::string& status) {
    std::string statusText = "Status: " + status;
    gtk_label_set_text(GTK_LABEL(statusLabel), statusText.c_str());
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    
    CampusClientGUI client;
    client.createGUI();
    client.run();
    
    return 0;
}
