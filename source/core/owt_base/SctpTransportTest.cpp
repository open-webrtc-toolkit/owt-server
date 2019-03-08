// Test SctpTransport functionality

#include <iostream>
#include <cassert>
#include "RawTransport.h"
#include "SctpTransport.h"


using namespace std;

class SctpPeer : public owt_base::RawTransportListener {
public:
    SctpPeer() : m_recvCount(0) {
    	m_transport.reset(new owt_base::SctpTransport(this));
    	m_transport->open();
    }

    virtual ~SctpPeer() {
    	m_transport->close();
    }

    void connect(const std::string& dest_ip, unsigned int udpPort, unsigned int sctpPort) {
    	m_transport->connect(dest_ip, udpPort, sctpPort);
    }

    unsigned int getUdpPort() {
    	return m_transport->getLocalUdpPort();
    }

    unsigned int getSctpPort() {
    	return m_transport->getLocalSctpPort();
    }

    void onTransportData(char* buf, int len) {
    	cout << "Sctp received:" << std::string(buf, len) << endl;
    	m_recvCount++;
        m_received.append(buf, len);
    	cout << "Recv count:" << m_recvCount << endl;
    }

    void close() {
    	m_transport->close();
    }

    void sendData(const std::string& msg) {
	    m_transport->sendData(msg.c_str(), msg.length());
    }
    void onTransportError() { }
    void onTransportConnected() { }

    std::string& getReceivedData() { return m_received; }

private:
    boost::shared_ptr<owt_base::SctpTransport> m_transport;
    std::string m_received;
    int m_recvCount;
};

void testCase0(int argc, char *argv[]) {
	SctpPeer peer1, peer2;

	cout << "Ports1:" << peer1.getUdpPort() << "," << peer1.getSctpPort() << endl;
	cout << "Ports2:" << peer2.getUdpPort() << "," << peer2.getSctpPort() << endl;

	peer1.connect("127.0.0.1", peer2.getUdpPort(), peer2.getSctpPort());
	peer2.connect("127.0.0.1", peer1.getUdpPort(), peer1.getSctpPort());

    std::string msg1 = "Hello, I'm peer One.";
    std::string msg2 = "Hi, I'm peer Two.";

	peer1.sendData(msg1);
    peer2.sendData(msg2);

    peer1.close();
    peer2.close();

    assert(peer1.getReceivedData() == msg2);
    assert(peer1.getReceivedData() == msg1);

    cout << "Case 0: PASS" << endl;
}

int main(int argc, char *argv[]) {
	testCase0(argc, argv);
	cout << "finish test" << endl;
	return 0;
}
