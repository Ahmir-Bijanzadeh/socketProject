all:client.o server.o TCPLib.o
	g++  Client/client.o Client/TCPLib.o -o Client/client
	g++  Server/server.o Server/TCPLib.o -o Server/server

client.o:
	g++ -c Client/client.cpp -o Client/client.o

server.o:
	g++ -c Server/server.cpp -o Server/server.o

TCPLib.o:
	g++ -c Client/TCPLib.cpp -o Client/TCPLib.o
	g++ -c Server/TCPLib.cpp -o Server/TCPLib.o

clean:
	rm -r Client/client.o
	rm -r Client/TCPLib.o
	rm -r Server/server.o
	rm -r Server/TCPLib.o
	rm -r Client/client
	rm -r Server/server
