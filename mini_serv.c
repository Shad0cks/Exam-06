#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

int max, curr = 0;
int fd[10000];

char sendMessage[4096*42], bufRead[4096*42], bufWrite[4096*42];
fd_set active, fdRead, fdWrite;

void err()
{
    write(2, "Fatal error\n", 12);
    exit(1);
}

void sendAll(int client)
{
    for (int i = 0; i <= max; i++)
    {
        if (FD_ISSET(i, &fdWrite) && i != client)
        {
            send(i, bufWrite, strlen(bufWrite), 0);
        }
    }
    bzero(&bufWrite, sizeof(bufWrite));
}

int main(int argc, char **argv) {
    if (argc != 2)
    {
        write(2, "Wrong number of arguments\n", 26);
        return 1;
    }
	int sockfd, connfd;
	struct sockaddr_in servaddr, cli; 
    socklen_t len = sizeof(cli);

    FD_ZERO(&active);
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		err();
	} 

    bzero(&fd, sizeof(fd));
	bzero(&servaddr, sizeof(servaddr)); 
    FD_SET(sockfd, &active);
    max = sockfd;
	
    // assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1])); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
		err();
	} 
	if (listen(sockfd, 128) != 0) {
		err();
	}
    while (1)
    {
        fdRead = fdWrite = active;
        if (select(max + 1, &fdRead, &fdWrite, 0, 0) < 0)
            continue;

        for (int i = 0; i <= max; i++)
        {
            if (FD_ISSET(i, &fdRead) && i == sockfd)
            {
                connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
                if (connfd < 0) { 
                    continue;
                } 
                max = connfd > max ? connfd : max;
                FD_SET(connfd, &active);
                fd[connfd] = curr++;
                sprintf(bufWrite, "server: client %d just arrived\n", fd[connfd]);
                sendAll(connfd);
                break;
            }
            if (FD_ISSET(i, &fdRead) && i != sockfd)
            {
                int ret_recv = 1;
                while (ret_recv == 1 && bufRead[strlen(bufRead) - 1] != '\n')
                {
                    ret_recv = recv(i, bufRead + strlen(bufRead), 1, 0);
                    if (ret_recv <= 0)
                        break;
                }

                if (ret_recv <= 0)
                {
                    sprintf(bufWrite, "server: client %d just left\n", fd[i]);
                    sendAll(i);
                    FD_CLR(i, &active);
                    close(i);
                    break;
                }
                else
                {
                    for (int j = 0, h = 0; bufRead[j]; j++, h++)
                    {
                        sendMessage[h] =  bufRead[j];
                        if (sendMessage[h] == '\n')
                        {
                            sendMessage[h] = '\0';
                            sprintf(bufWrite, "client %d: %s\n", fd[i], sendMessage);
                            sendAll(i);
                            h = -1;
                            bzero(&sendMessage, sizeof(sendMessage));
                        }
                    }
                }
                bzero(&bufRead, sizeof(bufRead));
            }
        }
    }
}
