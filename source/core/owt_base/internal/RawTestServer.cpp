#include "TransportServer.h"
#include "TransportClient.h"
#include <iostream>
#include <cstdlib>
#include <thread>
#include <chrono>

using namespace std;

class SListener : public owt_base::TransportServer::Listener {
public:
    SListener() {}
    // Implements TransportServer::Listener
    void onSessionData(int id, char* data, int len) override {
        cout << "Session:" << id << ", Received: " << data << endl;
    }
    void onSessionAdded(int id) override {
        cout << "Connected: " << id << endl;
    }
    void onSessionRemoved(int id) override {
        cout << "Disconnected: " << m_id << endl;
    }
};

int main(int argc, char *argv[]) {
    int port = 3456;
    if (argc > 2) {
        port = std::atoi(argv[1]);
    }
    SListener sl;
    owt_base::TransportServer s(&sl);
    s.listenTo(port, port);
    std::this_thread::sleep_for (std::chrono::seconds(1));
    cout << "port:" << s.getListeningPort() << endl;

    string msg;
    while(cin) {
        getline(std::cin, msg);
        s.sendData(msg.c_str(), msg.length());
        cout << "Send: " << msg << endl;
    };
    return 0;
}