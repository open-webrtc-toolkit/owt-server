CXX ?= "g++"

all: echo_client echo_server
echo_client: QuicEchoClient.cc
	$(CXX) -std=c++11 -I../src/wrapper -c -o QuicEchoClient.o QuicEchoClient.cc && \
	$(CXX) -o echo_client QuicEchoClient.o -L../lib -lrawquic
echo_server: QuicEchoServer.cc
	$(CXX) -std=c++11 -I../src/wrapper -c -o QuicEchoServer.o QuicEchoServer.cc && \
	$(CXX) -o echo_server QuicEchoServer.o -L../lib -lrawquic
clean:
	rm *.o echo_*