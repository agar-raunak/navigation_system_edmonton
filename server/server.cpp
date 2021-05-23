/*
Name: Raunak Agarwal
Student Id: 1636678
CMPUT 275 Winter 21
Assignment 1: Part 2
*/
#include <iostream>
#include <cassert>
#include <fstream>
#include <sstream>
#include <string>
#include <string.h>
#include <list>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "digraph.cpp"
#include "wdigraph.h"
#include "dijkstra.cpp"
#include <sys/types.h>    // include for portability
#include <sys/time.h>   // timeval
#include <netdb.h>      // getaddrinfo, freeaddrinfo, INADDR_ANY
#include <cstdlib>      // atoi
#include <cstring>      // strlen, strcmp
 
using namespace std;
#define LISTEN_BACKLOG 50
#define BUFFER_SIZE 1024

struct Point {
    long long lat, lon;
};

// returns the manhattan distance between two points
long long manhattan(const Point& pt1, const Point& pt2) {
  long long dLat = pt1.lat - pt2.lat, dLon = pt1.lon - pt2.lon;
  return abs(dLat) + abs(dLon);
}

// finds the point that is closest to a given point, pt
int findClosest(const Point& pt, const unordered_map<int, Point>& points) {
  pair<int, Point> best = *points.begin();

  for (const auto& check : points) {
    if (manhattan(pt, check.second) < manhattan(pt, best.second)) {
      best = check;
    }
  }
  return best.first;
}

// reads graph description from the input file and builts a graph instance
void readGraph(const string& filename, WDigraph& g, unordered_map<int, Point>& points) {
  ifstream fin(filename);
  string line;

  while (getline(fin, line)) {
    // split the string around the commas, there will be 4 substrings either way
    string p[4];
    int at = 0;
    for (auto c : line) {
      if (c == ',') {
        // starting a new string
        ++at;
      }
      else {
        // appending a character to the string we are building
        p[at] += c;
      }
    }

    if (at != 3) {
      // empty line
      break;
    }

    if (p[0] == "V") {
      // adding a new vertex
      int id = stoi(p[1]);
      assert(id == stoll(p[1])); // sanity check: asserts if some id is not 32-bit
      points[id].lat = static_cast<long long>(stod(p[2])*100000);
      points[id].lon = static_cast<long long>(stod(p[3])*100000);
      g.addVertex(id);
    }
    else {
      // adding a new directed edge
      int u = stoi(p[1]), v = stoi(p[2]);
      g.addEdge(u, v, manhattan(points[u], points[v]));
    }
  }
}
int main(int argc, char* argv[]){
  WDigraph graph;
  unordered_map<int, Point> points;
  int PORT;
  if (argc != 2){
    cout<<"Please enter the Port in the terminal\nUse format: ./server [port]\n";
    return 1;
  }
  PORT = atoi(argv[1]);

  // build the graph
  readGraph("edmonton-roads-2.0.1.txt", graph, points);

  struct sockaddr_in my_addr, peer_addr;

  // zero out the structor variable because it has an unused part
  memset(&my_addr, '\0', sizeof my_addr);

  // Declare variables for socket descriptors 
  int lstn_socket_desc, conn_socket_desc;

  char echobuffer[BUFFER_SIZE] = {};
  char waypoint[BUFFER_SIZE] = {};
  char pathlen[BUFFER_SIZE] = {};

  lstn_socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  if (lstn_socket_desc == -1) {
    std::cerr << "Listening socket creation failed!\n";
    return 1;
  }

  // Prepare sockaddr_in structure variable
  my_addr.sin_family = AF_INET;       // address family (2 bytes)
  my_addr.sin_port = htons(PORT);       // port in network byte order (2 bytes)
                        // htons takes care of host-order to short network-order conversion.
  my_addr.sin_addr.s_addr = htonl(INADDR_ANY);// internet address (4 bytes) INADDR_ANY is all local interfaces
                        // htons takes care of host-order to long network-order conversion.
  if (bind(lstn_socket_desc, (struct sockaddr *) &my_addr, sizeof my_addr) == -1) {
      std::cerr << "Binding failed!\n";
      close(lstn_socket_desc);
      return 1;
  }
  std::cout << "Binding was successful\n";


    if (listen(lstn_socket_desc, LISTEN_BACKLOG) == -1) {
      std::cerr << "Cannot listen to the specified socket!\n";
        close(lstn_socket_desc);
        return 1;
    }

  socklen_t peer_addr_size = sizeof my_addr;

    // Extract the first connection request from the queue of pending connection requests
    // Return a new connection socket descriptor which is not in the listening state
    conn_socket_desc = accept(lstn_socket_desc, (struct sockaddr *) &peer_addr, &peer_addr_size);
    if (conn_socket_desc == -1){
      std::cerr << "Connection socket creation failed!\n";
      return 1;
    }
    std::cout << "Connection request accepted from " << inet_ntoa(peer_addr.sin_addr) << ":" << ntohs(peer_addr.sin_port) << "\n";

    while (true) {
        // Block until a message arrives (unless O_NONBLOCK is set on the socket's file descriptor)
        int rec_size = read(conn_socket_desc, echobuffer, BUFFER_SIZE); //receives the coordinates
        if (rec_size == -1) {
          cout << "Timeout occurred... still waiting!\n";
        }
        cout << "Message received\n";
        if (strcmp("Q\n", echobuffer) == 0) { //if the plotter was closed, then the connection breaks
          cout << "Connection will be closed\n";
          break;

        }
        string req(echobuffer); //convert to string
        if(req[0]!='R'){
          continue;
        }
        else{
          //this process finda the latitude and longitude of start point and end point
          size_t sp1 = req.find(" ",2);
          size_t sp2 = req.find(" ", sp1+ 1);
          size_t sp3 = req.find(" ", sp2+ 1);
          long long lat1 = stoll(req.substr(2,sp1-1));
          long long lon1 = stoll(req.substr(sp1+1, sp2-1));
          long long lat2 = stoll(req.substr(sp2 +1, sp3 -1));
          long long lon2 = stoll(req.substr(sp3 + 1));

          //conversion to Point struct
          Point sPoint = {lat1, lon1};
          Point ePoint = {lat2, lon2};

          //finds the closest vertex to start point and end point
          int start = findClosest(sPoint, points), end = findClosest(ePoint, points);
          unordered_map<int, PIL> tree;
          dijkstra(graph, start, tree);

          //if the end point does not lie in the tree, then it send a message to client and continues.
          if (tree.find(end) == tree.end()) {
            string msg = "N 0";
            strcpy(pathlen,msg.c_str());
            send(conn_socket_desc, pathlen, strlen(pathlen)+1, 0);
            continue;
          }
          else {
          // read off the path by stepping back through the search tree
          list<int> path;
          while (end != start) {
          path.push_front(end);
          end = tree[end].first;
          }
          path.push_front(start);



          // creates a message to send to the client. Message is number of waypoints.
          char recv_ack[]={};
          if(path.size()!=0){
            string msg = "N "+ to_string(path.size());
            strcpy(pathlen, msg.c_str());
            send(conn_socket_desc, pathlen, strlen(pathlen)+1, 0);
          }

          string waypt="";
          for(int v: path){
            //send each waypoint to client. Wait for acknowledgement
            // and repeat
            read(conn_socket_desc, recv_ack, BUFFER_SIZE);
            if(strcmp(recv_ack,"A") == 0){
            waypt = "W " + to_string(points[v].lat) + " " + to_string(points[v].lon);
            strcpy(waypoint, waypt.c_str());
            send(conn_socket_desc, waypoint, strlen(waypoint)+1, 0);
            }

            }
            read(conn_socket_desc, recv_ack, BUFFER_SIZE);
            if(strcmp(recv_ack,"A") == 0){
              //after final acknowledgement, send E to client
            char end_path[] = {"E"};
            send(conn_socket_desc, end_path, strlen(end_path)+1, 0);
          
          }
          }
          }

          }
        close(lstn_socket_desc);
        close(conn_socket_desc);

  return 0;
        }
