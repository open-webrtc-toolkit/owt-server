#include "TransportServer.h"
#include "TransportClient.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace std;

class SListener : public owt_base::TransportClient::Listener {
public:
    SListener(std::string id) : m_id(id) {}
    // Implements TransportClient::Listener
    void onConnected() override;
    void onData(char* data, int len) override;
    void onDisconnected() override;

    void onData(char* data, int len) override {
        cout << "Received: " << data << endl;
    }
    void onConnected() override {
        cout << "Connected: " << m_id << endl;
    }
    void onDisconnected() override {
        cout << "Disconnected: " << m_id << endl;
    }
private:
    std::string m_id;
};

int main(int argc, char *argv[]) {
    string ip = "127.0.0.1";
    int port = 3456;
    if (argc > 3) {
        ip = std::string(argv[1]);
        port = std::atoi(argv[2]);
    }
    SListener ci("c");
    owt_base::TransportClient c(&ci);
    c.createConnection(ip, port);

    string msg;
    while(cin) {
        getline(std::cin, msg);
        c.sendData(msg.c_str(), msg.length());
        cout << "Send: " << msg << endl;
    };
    return 0;
}