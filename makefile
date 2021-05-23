all: client server

server: server.o dijkstra.o digraph.o server/wdigraph.h server/dijkstra.h
	g++ server/server.o -o server/server

server.o: server/server.cpp server/dijkstra.h server/wdigraph.h
	g++ server/server.cpp -o server/server.o -c

dijkstra.o: server/dijkstra.cpp server/dijkstra.h server/wdigraph.h
	g++ -c server/dijkstra.cpp -o server/dijkstra.o


digraph.o: server/digraph.cpp server/digraph.h
	g++ -c server/digraph.cpp -o server/digraph.o

client: client.o
	g++ client/client.o -o client/client
	
client.o: client/client.cpp
	g++ client/client.cpp -o client/client.o -c

clean:
	rm server/*.o server/server client/client client/*.o
