/*
Name: Raunak Agarwal
Student Id: 1636678
CMPUT 275 Winter 21
Assignment 1: Part 2
*/
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>     // socket, connect
#include <arpa/inet.h>
#include <errno.h>
#include <cstring>
//#include <string.h>
#include <string>
#include <utility>
#include <typeinfo>
#include <algorithm>
#define MSG_SIZE 1024
#define BUFFER_SIZE 1024
// Add more libraries, macros, functions, and global variables if needed

using namespace std;

int create_and_open_fifo(const char * pname, int mode) {
    // creating a fifo special file in the current working directory
    // with read-write permissions for communication with the plotter
    // both proecsses must open the fifo before they can perform
    // read and write operations on it
    if (mkfifo(pname, 0666) == -1) {
        cout << "Unable to make a fifo. Ensure that this pipe does not exist already!" << endl;
        exit(-1);
    }

    // opening the fifo for read-only or write-only access
    // a file descriptor that refers to the open file description is
    // returned
    int fd = open(pname, mode);

    if (fd == -1) {
        cout << "Error: failed on opening named pipe." << endl;
        exit(-1);
    }

    return fd;
}

int main(int argc, char const *argv[]) {
	int SERVER_PORT;
	string server_IP;
	if (argc != 3){
	cout<<"Wrong input format\nEnter in the following way: ./client [port] [server IP address]\n";	
	return 1;
	}
    //declare port and server_id
    SERVER_PORT = atoi(argv[1]);
    server_IP = argv[2];
    const char *inpipe = "inpipe";
    const char *outpipe = "outpipe";

    //creating inpipe and outpipe
    int in = create_and_open_fifo(inpipe, O_RDONLY);
    cout << "inpipe opened..." << endl;
    int out = create_and_open_fifo(outpipe, O_WRONLY);
    cout << "outpipe opened..." << endl;

    // Your code starts here
    struct sockaddr_in my_addr, peer_addr;

    // zero out the structor variable because it has an unused part
    memset(&my_addr, '\0', sizeof my_addr);

    // Declare socket descriptor
    int socket_desc;

    //char outbound[BUFFER_SIZE] = {};
    char inbound[BUFFER_SIZE] = {};

    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        std::cerr << "Listening socket creation failed!\n";
        return 1;
    }
    

    // Prepare sockaddr_in structure variable
    peer_addr.sin_family = AF_INET;                         // address family (2 bytes)
    peer_addr.sin_port = htons(SERVER_PORT);
    peer_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (connect(socket_desc, (struct sockaddr *) &peer_addr, sizeof peer_addr) == -1) {
        std::cerr << "Cannot connect to the host!\n";
        close(socket_desc);
        return 1;
    }
    std::cout << "Connection established with " << inet_ntoa(peer_addr.sin_addr) << ":" << ntohs(peer_addr.sin_port) << "\n";
    

    while(true){
    char line1[MSG_SIZE] = {};
    char line2[MSG_SIZE] = {};
    char ack[]={"A"}; //for acknowledgement
    char waypt[BUFFER_SIZE]={}; //for receiving waypts

    int bytes_read1;
    int bytes_read2;
    
    /* reads up to MSG_SIZE bytes from fd into the line1 */
    bytes_read1 = read(in, line1, MSG_SIZE);
    string li1(line1); //convert to string
    size_t backn = li1.find("\n"); //find the position between the coordinates
    string firstcoord = li1.substr(0, backn-1); //first coord
    string secondcoord = li1.substr(backn+1); //second coord

    //if plotter is closed send Q to server and exit.
    if (strcmp("Q\n",line1) == 0){
    	cout<<"You exited the plotter\n";
        send(socket_desc, line1, strlen(line1)+ 1,0);
        break;
    }
    //converting the coords into readable form for server
    size_t space1 = firstcoord.find(" ");
    string slat1 = firstcoord.substr(0,space1-1);
    string slon1 = firstcoord.substr(space1+1);
    double dlat1 = stod(slat1);
    double dlon1 = stod(slon1);
    long long lat1 = static_cast< long long >(dlat1*100000);
    long long lon1 = static_cast< long long >(dlon1*100000);

    size_t space2 = secondcoord.find(" ");
    string slat2 = secondcoord.substr(0,space1-1);
    string slon2 = secondcoord.substr(space1+1);
    double dlat2 = stod(slat2);
    double dlon2 = stod(slon2);
    long long lat2 = static_cast< long long >(dlat2*100000);
    long long lon2 = static_cast< long long >(dlon2*100000);

    string tr = "";
    tr = "R " + to_string(lat1)+ " " + to_string(lon1) + " " + to_string(lat2)+ " "+ to_string(lon2);
    char outbound[tr.length()+1];

    strcpy(outbound,tr.c_str()); //convert to array

    send(socket_desc, outbound, strlen(outbound) + 1, 0); //sending request to server

    read(socket_desc, inbound, BUFFER_SIZE); //receiving message from server

    string pathlen(inbound); //converting it to string

    //finding number of waypoints
    size_t space = pathlen.find(" ") + 1;
    int no_way_pts = stoi(pathlen.substr(space));
    char fsc[BUFFER_SIZE]={};
    
    if(no_way_pts == 0){
    	//if number of waypts is 0, send E to plotter and continue.
        char endit[BUFFER_SIZE]={"E\n"};
        write(out, endit, strlen(endit));
        continue;
    }
    else{
        

        for(int i = 0; i < no_way_pts; i++){
        	/*
        	send acknowledgement to server and recieve waypoint from it
        	convert the waypoint to coordinate form and write to the 
        	plotter.
        	*/
            send(socket_desc, ack, strlen(ack) +1, 0);
            read(socket_desc, waypt, BUFFER_SIZE);
            string coord(waypt);
            size_t sp = coord.find(" ",2);
            string lat = coord.substr(2, sp-1);
            string lon = coord.substr(sp+1);
            double dlat = stod(lat)/100000;
            double dlon = stod(lon)/100000;
    
            string s = to_string(dlat);
            string s2 = to_string(dlon);
            string fincord = s + " " + s2 + "\n";
            strcpy(fsc, fincord.c_str());
            if(write(out, fincord.c_str(), fincord.length()) == -1){
                cout << "Error in writing\n";
            }
        }
        //send final acknowledgement to server
        send(socket_desc, ack, strlen(ack)+1, 0);

        char endak[] = {};
        //wait for final E acknowledgement from server and 
        //write it to plotter. After that continue 
        read(socket_desc, endak, BUFFER_SIZE);
        if(strcmp(endak, "E") == 0){
            char endit[BUFFER_SIZE] = {"E\n"};
            write(out, endit, strlen(endit));
            continue;
        }

    }
    

    }
    close(in);
    close(out);
    unlink(inpipe);
    unlink(outpipe);
    return 0;
}
