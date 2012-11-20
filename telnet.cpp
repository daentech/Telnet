#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "unit_test.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

using namespace std;

// Execute the given command by passing it to the pipe stream
// Return the results from both stdout and stderr (so we get any missing programs as well)
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

// Remove the newline characters to handle linux and mac line endings
string removeNewlineChars(string input){
    return input.erase(input.find_last_not_of(" \n\r\t")+1);
}

// Function which takes the action and performs it
string performAction(string command, string *wd, string program_dir){
    command = removeNewlineChars(command);
    // Change to the current directory of this thread to perform the action
    chdir(wd->c_str());
    cout << "Performing action: " << command << endl;
    string response = "";
    if(command.length() == 0){
    } else if (command.find("?") == 0 || command.find("help") == 0){
        response = "Available commands:\n";
        response.append("cd <directory>\t\tChange directory to the one specified\n");
        response.append("help <program>\t\tShow the help for the given program. If none provided, displays this message\n");
        response.append("ls <directory>\t\tDisplays all files in the provided directory. Defaults to current working directory\n");
        response.append("mkdir <directory>\tCreates the directory of the given name in the current working directory\n");
        response.append("pwd\t\t\tPrint the current working directory\n");
        response.append("logout\t\t\tLogout of the system\n");
        response.append("\n");
        response.append("Other shell commands are available in their standard form, such as rm, rmdir, touch etc.\n");
        response.append("Programs which require the full terminal screen, such as 'vim' will not work\n");
    } else if(command.find("cd ") == 0){
        // cd command found. Change the working directory
        cout << "cd command found" << endl;
        int pos = command.find(" ");
        string path = command.substr(pos + 1, command.length() - pos); 
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
    } else if (command.find("ls ") == 0 || command.compare("ls") == 0){
        // ls command found
        cout << "ls command found" << endl;
        response = exec(command.append(" 2>&1").c_str());
    } else if (command.find("mkdir ") == 0){
        // mkdir command found
        cout << "mkdir found" << endl;
        // Make the folder named in the command
        response = exec(command.append(" 2>&1").c_str());
    } else if (command.find("pwd") == 0){
        // pwd found, so display the current working directory
        cout << "pwd found" << endl;
        response = *wd;
        response.append("\n");
    } else if (command.find("logout") == 0){
        response = "exit\n";
    } else {
        response = exec(command.append(" 2>&1").c_str());;
    }

    // Change back to the program directory 
    chdir(program_dir.c_str());
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
    string program_dir = getcwd(path, 2048);
    string MOD = "Welcome to the terminal. Type ? for the list of commands\n> ";
    const char* mod_string = MOD.c_str();
    if((bytecount = send(*csock, mod_string, strlen(mod_string), 0))== -1){
        cout << "Error sending data" << endl;
        shutdown(*csock, 0);
        return 0;
    }

    // Loop the connection until logout is received
    while(true){
    
    memset(buffer, 0, buffer_len);
    if((bytecount = recv(*csock, buffer, buffer_len, 0))== -1){
        cout << "Error receiving data" <<endl;
        
        return 0;
    }
    cout << "Received string: " <<  buffer;
    string s(buffer);
    // Get the response from the command and return it to the client
    string response = performAction(s, &wd, program_dir);
    if(response.find("exit") == 0){
        // We are quitting
        response = "[END OF INPUT]\n";
        if((bytecount = send(*csock, response.c_str(), response.length(), 0))== -1){
            cout << "Error sending data" << endl;
            return 0;
        }
        cout << "Client disconnected" << endl;
        shutdown(*csock, 0);
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
    cout << "Client disconnected" << endl;
    return 0;
}

bool is_number(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

void test(){
    string actionresult;
    char path[2048];
    string cwd = getcwd(path, 2048);
    string program_dir = getcwd(path, 2048);    

    // Run a cd test
    actionresult = performAction("cd ../", &cwd, program_dir);
    // assert that the current working directory is one above the program dir
    assert(isParentDirectory(program_dir, cwd));
    
    // Change back to program directory
    string command = "cd ";
    command.append(program_dir);
    performAction(command, &cwd, program_dir);

    // Run a ls test
    performAction("touch tmp.myfile", &cwd, program_dir);
    actionresult = performAction("ls", &cwd, program_dir);

    // assert that tmp.myfile exists
    assert(fileExists("tmp.myfile", actionresult));

    // Remove the temporary file
    performAction("rm tmp.myfile", &cwd, program_dir);

    // Run a mkdir test
    performAction("mkdir tmp", &cwd, program_dir);
    actionresult = performAction("ls", &cwd, program_dir);
    
    // assert that tmp folder exists
    assert(fileExists("tmp", actionresult));

    // Remove the tmp folder
    performAction("rmdir tmp", &cwd, program_dir);
}

int main(int argc, char* argv[]){

    cout << "Launching telnet server" << endl;

    // Begin listening for a connection on port 23 or whichever port is passed in
    // Default telnet port
    int listen_port = 6770;
    bool tests = false;

    if(argc > 1){
        // We have had some arguments passed in
        for(int i = 1; i < argc; i++){
            string args(argv[i]);
            if(args.compare("-test") == 0){
                tests = true;
            } else if (is_number(args)){
                listen_port = atoi(args.c_str());
            }
        }
    }

    // if tests is true, execute the unit tests
    if(tests){
        cout << "Beginning unit testing" << endl;
        test();
    }


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
