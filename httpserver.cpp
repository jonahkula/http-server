#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <err.h>
#include <string>
#include <bits/stdc++.h>
#include <fcntl.h>
#include <ctime>
#include <vector>
#include <dirent.h>

#define DEFAULTPORT 1025
#define BUFFER 4096

// for shell script to start automatic new terminal
extern const std::string SHELL = "newTerminal.sh";
extern const std::string PERMISSION = "chmod +x ";

unsigned short setPortNum(int argc, const char **argv);
unsigned long getAddr(char *address);
std::string checkFileName(const char *name);
long int unixTimestamp();
void sendHTTP(int client_sockd, std::string httpCode, int length);

int main(int argc, const char **argv)
{
    // set the port and check for errors in command line
    unsigned short port = setPortNum(argc, argv);

    // automatically open new terminal to run GET and PUT curl commands from shell script
    system(std::string(PERMISSION + SHELL).c_str());
    system(std::string("./" + SHELL).c_str());
    system("clear");

    // create sockaddr_in with server info
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = getAddr((char *)argv[1]);
    server_addr.sin_port = htons(port);
    memset(server_addr.sin_zero, '\0', sizeof server_addr.sin_zero);

    // create client addresses
    struct sockaddr_in client_addr;
    socklen_t client_addrlen;

    // create server socket and check for success
    int server_sockd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sockd < 0)
    {
        err(1, "socket()");
    }

    // configure server socket and avoid 'Bind: Address Already in Use' error
    int enable = 1;
    setsockopt(server_sockd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));

    // bind server address to socket that is open and check for success
    if (bind(server_sockd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        err(1, "bind()");
    }

    // listen for incoming connections and check for success
    if (listen(server_sockd, SOMAXCONN) < 0)
    {
        err(1, "listen()");
    }

    // strings for http codes
    std::string httpCode200 = "HTTP/1.1 200 OK\r\n";
    std::string httpCode201 = "HTTP/1.1 201 Created\r\n";
    std::string httpCode400 = "HTTP/1.1 400 Bad Request\r\n";
    std::string httpCode403 = "HTTP/1.1 403 Forbidden\r\n";
    std::string httpCode404 = "HTTP/1.1 404 Not Found\r\n";
    std::string httpCode500 = "HTTP/1.1 500 Internal Server Error\r\n";
    std::string httpCode505 = "HTTP/1.1 505 HTTP Version Not Supported\r\n";

    // display port that you're listening on
    std::string waiting = "   - Listening on port: " + std::to_string(port) + " -\n\n";
    write(STDOUT_FILENO, waiting.c_str(), waiting.size());

    // initiate vector of unix timestamps for backups
    std::vector<long int> backups;

    // open current directory and scan for backup directories that already exist
    // push existing timestamps to the back of the backups vector
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(".")) != NULL)
    {
        // directory accessed, enter while to loop through all the files inside
        while ((ent = readdir(dir)) != NULL)
        {
            std::string file = ent->d_name;
            std::string fileCheck = checkFileName(file.c_str());

            // if name matches backup schema get timestamp and push_back()
            if (fileCheck == "backupDir")
            {
                long int fileNo = std::atoi(file.substr(7).c_str());
                backups.push_back(fileNo);
            }
        }
        closedir(dir);
    }
    else
    {
        // if unable to access the directory through an error
        perror("");
        return EXIT_FAILURE;
    }

    while (1)
    {
        // wait for and accept connection
        client_addrlen = sizeof(client_addr);
        int client_sockd = accept(server_sockd, (sockaddr *)&client_addr, &client_addrlen);
        if (client_sockd < 0)
        {
            // error when accepting connection
            sendHTTP(client_sockd, httpCode500, 0);
            close(client_sockd);
        }

        // prints header from curl command
        char buf[BUFFER];
        char c[BUFFER];
        int i = 0;

        // print out request information
        while (read(client_sockd, c, 1) > 0)
        {
            buf[i] = c[0];
            i++;
            if (strstr(buf, "\r\n\r\n") != NULL)
            {
                buf[i] = '\0';
                break;
            }
            c[1] = '\0';
        }

        // print header to server terminal
        write(STDOUT_FILENO, buf, BUFFER);
        std::vector<std::string> header;

        // parse to string vector by line
        char *token = strtok(buf, "\n");
        while (token != NULL)
        {
            header.push_back(std::string(token));
            token = strtok(NULL, "\n");
        }

        // vector of string to save tokens
        std::vector<std::string> tokens;
        std::stringstream ss(header[0]);
        std::string intermediate;

        // tokenizing top line
        while (std::getline(ss, intermediate, ' '))
        {
            tokens.push_back(intermediate);
        }

        // remove the slash at the beginning of file name, if it exists
        if (tokens[1][0] == '/')
        {
            tokens[1] = tokens[1].substr(1);
        }

        // check the file  name
        std::string nameCheck = checkFileName(tokens[1].c_str());

        if (tokens[2].find("HTTP/1.1") == std::string::npos)
        {
            // wrong HTTP version
            sendHTTP(client_sockd, httpCode505, 0);
        }
        else if (tokens[0] != "PUT" && tokens[0] != "GET")
        {
            // not PUT or GET
            sendHTTP(client_sockd, httpCode500, 0);
        }
        else if (nameCheck == "invalid")
        {
            // not valid file name
            sendHTTP(client_sockd, httpCode400, 0);
        }
        else if (nameCheck == "file")
        {
            // PUT or GET
            if (tokens[0] == "PUT")
            {
                // construct a file to write to
                int fd = creat(tokens[1].c_str(), 0666);
                int len = std::atoi(header[4].substr(16).c_str());

                if (fd == -1)
                {
                    // check for errors and send http codes
                    if (errno == EACCES)
                    {
                        sendHTTP(client_sockd, httpCode403, 0);
                    }
                    else
                    {
                        sendHTTP(client_sockd, httpCode500, 0);
                    }
                    close(fd);
                }
                else if (len == 0)
                {
                    // blank file
                    close(fd);
                    sendHTTP(client_sockd, httpCode201, len);
                }
                else
                {
                    // malloc char array for file contents
                    char *buffer = (char *)malloc(BUFFER);
                    int n;

                    // print file data by buffer sized chunks
                    while ((n = read(client_sockd, buffer, BUFFER)))
                    {
                        n = write(fd, buffer, n);
                        len -= n;
                        if (len <= 0)
                        {
                            break;
                        }
                    }

                    // close file, free char, and send success code
                    free(buffer);
                    close(fd);
                    sendHTTP(client_sockd, httpCode201, len);
                }
            }
            else if (tokens[0] == "GET")
            {
                if (access(tokens[1].c_str(), F_OK) == -1)
                {
                    // file not found
                    sendHTTP(client_sockd, httpCode404, 0);
                }
                else if (access(tokens[1].c_str(), R_OK) == -1)
                {
                    // permission error when reading
                    sendHTTP(client_sockd, httpCode403, 0);
                }
                else
                {
                    // file accessible, open and write file data to the client line by line
                    int fd = open(tokens[1].c_str(), O_RDONLY);

                    if (fd == -1)
                    {
                        // error opening file
                        sendHTTP(client_sockd, httpCode500, 0);
                    }
                    else
                    {
                        int n;
                        off_t fsize;
                        fsize = lseek(fd, 0, SEEK_END);
                        int len = (int)fsize;
                        lseek(fd, 0, SEEK_SET);

                        // send 200 code for successful GET
                        sendHTTP(client_sockd, httpCode200, len);

                        // read line from file and print it
                        while ((n = read(fd, buf, BUFFER)))
                        {
                            n = write(client_sockd, buf, n);
                            len -= n;
                            if (len <= 0)
                            {
                                break;
                            }
                        }
                    }

                    // close file from GET
                    close(fd);
                }
            }
            else
            {
                // in any other case
                sendHTTP(client_sockd, httpCode500, 0);
            }
        }
        else if (nameCheck == "backup" && tokens[0] == "GET")
        {
            // add to list of backups for LIST
            long int currentTime = unixTimestamp();
            backups.push_back(currentTime);
            std::string newDir = "backup-" + std::to_string(currentTime);

            // make a new backup directory
            std::string call = "mkdir " + newDir;
            system(call.c_str());

            // server message
            std::string message = "Creating backup directory '" + newDir + "'\n";

            // check all existing file names
            if ((dir = opendir(".")) != NULL)
            {
                while ((ent = readdir(dir)) != NULL)
                {
                    std::string file = ent->d_name;
                    std::string fileCheck = checkFileName(file.c_str());

                    // if passes file name schema and file name is not a directory
                    // begin copy to backup
                    if (fileCheck == "file" && ent->d_type != DT_DIR)
                    {
                        std::string newFile = "./" + newDir + "/" + file;

                        // open the file from working directory
                        int rfd = open(file.c_str(), O_RDONLY);
                        if (access(file.c_str(), R_OK) == -1)
                        {
                            // no READ permission, so skip this file
                            continue;
                        }
                        message += "+ " + file + "\n";

                        // create the new file in file path to the new directory
                        int wfd = creat(newFile.c_str(), 0666);

                        // grab file size and handle the 'copy'
                        int n;
                        off_t fsize;
                        fsize = lseek(rfd, 0, SEEK_END);
                        int len = (int)fsize;
                        lseek(rfd, 0, SEEK_SET);
                        char *buffer = (char *)malloc(BUFFER);
                        // print file data by buffer sized chunks
                        while ((n = read(rfd, buffer, BUFFER)))
                        {
                            n = write(wfd, buffer, n);
                            len -= n;
                            if (len <= 0)
                            {
                                break;
                            }
                        }
                        close(rfd);
                        close(wfd);
                    }
                }
                closedir(dir);
                message += "\n";
                write(STDOUT_FILENO, message.c_str(), message.size());
                sendHTTP(client_sockd, httpCode200, 0);
            }
            else
            {
                perror("");
                return EXIT_FAILURE;
            }
        }
        else if (nameCheck.find("recovery") != std::string::npos && tokens[0] == "GET")
        {
            // decide whether accessing most recent backup or user specified
            std::string theDir = "backup-";
            if (nameCheck == "recovery")
            {
                // most recent timestamp
                theDir += std::to_string(backups.back());
            }
            else
            {
                // specified timestamp
                theDir += tokens[1].substr(2);
            }

            // set the file path based on backup directory
            std::string path = "./" + theDir;
            std::string message = "Recovering files from '" + theDir + "'\n";

            // check all existing file names
            if ((dir = opendir(path.c_str())) != NULL)
            {
                while ((ent = readdir(dir)) != NULL)
                {
                    std::string file = ent->d_name;
                    std::string fileCheck = checkFileName(file.c_str());

                    // if file name passes file schema begin 'recovery'
                    if (fileCheck == "file")
                    {
                        message += "<- " + file + "\n";
                        std::string oldFile = path + "/" + file;

                        // read file from backup
                        int rfd = open(oldFile.c_str(), O_RDONLY);

                        // write file in the working directory
                        int wfd = creat(file.c_str(), 0666);

                        // 'recovery'
                        int n;
                        off_t fsize;
                        fsize = lseek(rfd, 0, SEEK_END);
                        int len = (int)fsize;
                        lseek(rfd, 0, SEEK_SET);

                        char *buffer = (char *)malloc(BUFFER);
                        // print file data by buffer sized chunks
                        while ((n = read(rfd, buffer, BUFFER)))
                        {
                            n = write(wfd, buffer, n);
                            len -= n;
                            if (len <= 0)
                            {
                                break;
                            }
                        }
                        close(rfd);
                        close(wfd);
                    }
                }
                closedir(dir);
                message += "\n";
                write(STDOUT_FILENO, message.c_str(), message.size());
                sendHTTP(client_sockd, httpCode200, 0);
            }
            else
            {
                sendHTTP(client_sockd, httpCode404, 0);
            }
        }
        else if (nameCheck == "list" && tokens[0] == "GET")
        {
            if (backups.empty())
            {
                // if no backups exist, return 200 with nothing
                sendHTTP(client_sockd, httpCode200, 0);
            }
            else
            {
                // if backups do exist return list with '\n' delimiter
                int len = (backups.size() * 11) + 1;
                sendHTTP(client_sockd, httpCode200, len);
                std::string numToString;
                for (auto backupTime : backups)
                {
                    numToString += std::to_string(backupTime) + "\n";
                }
                numToString += "\n";
                write(client_sockd, numToString.c_str(), strlen(numToString.c_str()));
            }
        }
        else
        {
            // file name is backup, recovery, or list but with a PUT
            sendHTTP(client_sockd, httpCode400, 0);
        }

        // closing connection with client
        std::string closing = "  - Closing client connection -\n\n";
        write(STDOUT_FILENO, closing.c_str(), strlen(closing.c_str()));
        close(client_sockd);
    }

    // close server socket and end program
    close(server_sockd);
    return 0;
}

// decide and set port number, also checks for errors in command line
unsigned short setPortNum(int argc, const char **argv)
{
    if (argc == 1)
    {
        // no address or port number in command line
        warn("Error: address and port not specified on command line");
        exit(1);
    }
    else if (argc > 3)
    {
        // too many arguments in command line
        warn("Error: too many arguments in command line");
        exit(1);
    }
    else if (argc == 2)
    {
        // address specified but not port, return default port
        return DEFAULTPORT;
    }
    else
    {
        // address and port specified so run check on port
        int port = std::atoi(argv[2]);
        if (port <= 1024)
        {
            return DEFAULTPORT;
        }
        return port;
    }
}

// returns numerical representation of address for struct sockaddr_in
unsigned long getAddr(char *address)
{
    unsigned long res;
    struct addrinfo hints;
    struct addrinfo *info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    if (getaddrinfo(address, NULL, &hints, &info) != 0 || info == NULL)
    {
        warn("getaddrinfo(): address identification error");
        exit(1);
    }
    res = ((struct sockaddr_in *)info->ai_addr)->sin_addr.s_addr;
    freeaddrinfo(info);
    return res;
}

// check file name and return proper string
std::string checkFileName(const char *name)
{
    int nameLength = strlen((const char *)name);
    if (nameLength == 1)
    {
        // case of 'r', 'b', or 'l'
        switch (name[0])
        {
        case 'r':
            return "recovery";
        case 'l':
            return "list";
        case 'b':
            return "backup";
        default:
            return "invalid";
        }
    }
    else if (nameLength == 10)
    {
        for (int i = 0; i < nameLength; i++)
        {
            char character = name[i];
            if (!(character >= 65 && character <= 90) && !(character >= 97 && character <= 122) && (isdigit(character) == 0))
            {
                // false if not A-Z, a-z, or 0-9
                return "invalid";
            }
        }
        return "file";
    }
    else if (nameLength == 17)
    {
        // check if a backup directory
        std::string str(name);
        std::string noDigits = str.substr(0, 7);
        if (noDigits.compare("backup-") != 0)
        {
            return "invalid";
        }
        for (int i = 7; i < nameLength; i++)
        {
            if (isdigit(name[i]) == 0)
            {
                // false if not a digit
                return "invalid";
            }
        }
        return "backupDir";
    }
    else
    {
        // check to see if recovering a proper unix timestamp
        if (name[0] == 'r' && name[1] == '/' && nameLength == 12)
        {
            for (int i = 2; i < nameLength; i++)
            {
                if (isdigit(name[i]) == 0)
                {
                    // false if not a digit
                    return "invalid";
                }
            }
            return "recovery with timestamp";
        }
        return "invalid";
    }
}

// return the current unix timestamp
long int unixTimestamp()
{
    time_t t = std::time(0);
    long int now = static_cast<long int>(t);
    return now;
}

// print out http code message with content length
void sendHTTP(int client_sockd, std::string httpCode, int length)
{
    std::string content = "Content-Length: " + std::to_string(length) + "\r\n\r\n";
    httpCode += content;
    send(client_sockd, httpCode.c_str(), httpCode.size(), 0);
}