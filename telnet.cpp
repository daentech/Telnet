#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <unistd.h>
#include <errno.h>

using namespace std;

string exec(const char* cmd) {
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "ERROR";
    char buffer[128];
    std::string result = "";
    while(!feof(pipe)) {
        if(fgets(buffer, 128, pipe) != NULL)
            result += buffer;
    }
    pclose(pipe);
    return result;
}

string performAction(string command, string *wd){
    cout << "Performing action: " << command << endl;
    cout << "Comparing command to ls: " << command.find("ls\n") << endl;
    string response = "";
    if (command.find("?") == 0 || command.find("help") == 0){
        response = "Available commands:\n";
        response.append("cd <directory>\t\tChange directory to the one specified\n");
        response.append("help <program>\t\tShow the help for the given program. If none provided, displays this message\n");
        response.append("ls <directory>\t\tDisplays all files in the provided directory. Defaults to current working directory\n");
        response.append("mkdir <directory>\tCreates the directory of the given name in the current working directory\n");
        response.append("pwd\t\t\tPrint the current working directory\n");
        response.append("logout\t\t\tLogout of the system\n");
    } else if(command.find("cd ") == 0){
        // cd command found. Change the working directory
        cout << "cd command found" << endl;
        int pos = command.find(" ");
        string path = command.substr(pos + 1, command.length() - pos - 3); 
        cout << "Path provided: " << path << endl;
        if(chdir(path.c_str()) != 0){
            // There was an error
            switch(errno){
            case EACCES:
                response = "Search permission is denied\n";
                break;
            case EFAULT:
                response = "Path outside accesible address space\n";
                break;
            case EIO:
                response = "An I/O error occurred\n";
                break;
            case ELOOP:
                response = "Too many symbolic links when resolving the path\n";
                break;
            case ENAMETOOLONG:
                response = "Path provided was too long\n";
                break;
            case ENOENT:
                response = "File does not exist\n";
                break;
            case ENOMEM:
                response = "Insufficient Kernel memory available\n";
                break;
            case ENOTDIR:
                response = "Path is not a directory\n";
                break;
            }
        } else {
            char newpath[2048];
            *wd = getcwd(newpath, 2048);
        }
    } else if (command.find("ls ") == 0 || command.find("ls\n") == 0){
        // ls command found
        cout << "ls command found" << endl;
        response = exec(command.substr(0, command.length() - 2).c_str());
    } else if (command.find("mkdir ") == 0){
        // mkdir command found
        cout << "mkdir found" << endl;
        // Make the folder named in the command
    } else if (command.find("pwd") == 0){
        // pwd found, so display the current working directory
        cout << "pwd found" << endl;
        response = *wd;
        response.append("\n");
    } else if (command.find("logout") == 0){
        response = "quit\n";
    } else {
        response = "Command not found\n";
    }
    
    return response;
}

void* SocketHandler(void* lp){
    int *csock = (int*)lp;

    char buffer[1024];
    int buffer_len = 1024;
    int bytecount;

    // Upon first connection, set the working directory and send the MOD
    char path[2048];
    string wd = getcwd(path, 2048);
    string MOD = "Welcome to the terminal. Type ? for the list of commands\n> ";
    const char* mod_string = MOD.c_str();
    if((bytecount = send(*csock, mod_string, strlen(mod_string), 0))== -1){
        cout << "Error sending data" << endl;
        return 0;
    }

    while(true){
    
    memset(buffer, 0, buffer_len);
    if((bytecount = recv(*csock, buffer, buffer_len, 0))== -1){
        cout << "Error receiving data" <<endl;
        
        return 0;
    }
    cout << "Received string: " <<  buffer;
    string s(buffer);
    // Get the response from the command and return it to the client
    string response = performAction(s, &wd);
    if(response.find("quit") == 0){
        // We are quitting
        free(csock);
        return 0;
    }
    response.append("> ");
    if((bytecount = send(*csock, response.c_str(), response.length(), 0))== -1){
        cout << "Error sending data" << endl;
        return 0;
    }
    
    cout << "Sent bytes " << bytecount << endl;
    }
    return 0;
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
