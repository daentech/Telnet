#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

void* SocketHandler(void* lp){
    int *csock = (int*)lp;

    char buffer[1024];
    int buffer_len = 1024;
    int bytecount;

    memset(buffer, 0, buffer_len);
    if((bytecount = recv(*csock, buffer, buffer_len, 0))== -1){
        cout << "Error receiving data" <<endl;
        return 0;
    }
    printf("Received bytes %d\nReceived string \"%s\"\n", bytecount, buffer);

    if((bytecount = send(*csock, buffer, strlen(buffer), 0))== -1){
        cout << "Error sending data" << endl;
        return 0;
    }
    
    cout << "Sent bytes " << bytecount << endl;

}

int main(){

    cout << "Launching telnet server" << endl;

    // Begin listening for a connection on port 23
    // Default telnet port
    int listen_port = 6770;

    struct sockaddr_in host_addr;
    int sck = socket(AF_INET, SOCK_STREAM, 0);
    if(sck < 0){
        // There was an error opening the socket
        cout << "Error opening socket" << endl;
        return 1;
    }

    bzero((char*) &host_addr, sizeof(host_addr));
    
    host_addr.sin_family = AF_INET;
    host_addr.sin_addr.s_addr = INADDR_ANY;
    host_addr.sin_port = htons(listen_port);

    // Bind the socket to the host address
    if(bind(sck, (struct sockaddr*) &host_addr, sizeof(host_addr)) < 0){
        // Problem binding
        cout << "Error binding socket to host" << endl;
        return 1;
    }

    // Begin listening on the socket
    listen(sck, 5);

    // Begin server

    socklen_t addr_size = sizeof(sockaddr_in);
    int* csock;
    pthread_t thread_id = 0;

    // Run the server forever
    while(true){
        cout << "Waiting for a connection..." << endl;
        csock = (int*)malloc(sizeof(int));
        if((*csock = accept(sck, (sockaddr*)&host_addr, &addr_size)) >= 0){
            cout << "Received a connection" << endl;
            // A connection has been received, so spawn a thread to handle it 
            pthread_create(&thread_id,0,&SocketHandler, (void*)csock );
            pthread_detach(thread_id);
        }
    }

}
