#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAXLINE 1024
#define PORT 1e4

struct sockaddr_in serv_addr;

int BroadcastFirstConnect(){
    printf("Broadcast connecting..\n");
    char buf[MAXLINE] = "BroadcastMess";
    memset(&serv_addr, 0, sizeof(serv_addr));

    int sock_fd;
    if ( (sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    int a = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST, &a, sizeof(a));
    //bind(sock_fd, (struct sockaddr*) &serv_addr, sizeof (serv_addr));

    sendto(sock_fd, buf, strlen(buf), MSG_CONFIRM, (const struct sockaddr *) &serv_addr, sizeof serv_addr);

    // получаем
    socklen_t len = sizeof(serv_addr);
    recvfrom(sock_fd, buf, sizeof(buf), MSG_WAITALL, (struct sockaddr *) &serv_addr, &len);
    printf("Yeah! Get answer from server: ip = %s\n", inet_ntoa(serv_addr.sin_addr));
    close(sock_fd);
    return 0;
}

int DoUDP(char *buf){
    printf("UDP mode\n");
    int sock_fd;
    // Creating socket file descriptor
    if ( (sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    int n, len;
    char *str = "I know client datas\n\0";
    sendto(sock_fd, str, 21, MSG_CONFIRM, (const struct sockaddr *) &serv_addr, sizeof serv_addr);
    n = recvfrom(sock_fd, buf, MAXLINE, MSG_WAITALL, (struct sockaddr *) &serv_addr, (socklen_t *)&len);
    write(STDOUT_FILENO, buf, n);
    int forked = fork();
    if(forked) {
        while (1) {
            n = read(STDIN_FILENO, buf, MAXLINE);
            if (!strcmp(buf, "-quit\n")) {
                break;
            }
            sendto(sock_fd, buf, n, MSG_CONFIRM, (const struct sockaddr *) &serv_addr, sizeof serv_addr);
            memset(buf, 0, MAXLINE);
        }
    } else{
        while (1) {
            n = recvfrom(sock_fd, buf, MAXLINE, MSG_WAITALL, (struct sockaddr *) &serv_addr, (socklen_t *)&len);
            write(STDOUT_FILENO, buf, n);
            memset(buf, 0, MAXLINE);
        }
    }
    return 0;
}

int DoTCP(char *buf){
    printf("TCP mode\n");
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        printf("sock_fd == -1\n");
        exit(1);
    }
    printf("connecting..\n");
    int connect_fd = connect(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    if (connect_fd < 0) {
        printf("connect_fd == %d\n", connect_fd);
        return 1;
    }

    printf("connect_fd == %d\n", connect_fd);
    int n;
    int forked = fork();
    if(forked){
        while (1) {
            n = read(STDIN_FILENO, buf, MAXLINE);
            write(sock_fd, buf, n);
            memset(buf, 0, MAXLINE);
        }
    } else {
        while (1) {
            n = read(sock_fd, buf, MAXLINE);
            write(STDOUT_FILENO, buf, n);
            memset(buf, 0, MAXLINE);
        }
    }
    return 0;
}

int main(int argc, char** argv) {
    char buf[MAXLINE] = {0};

    BroadcastFirstConnect();
    serv_addr.sin_port = htons(PORT);

    if(argc == 2 && (!strcmp(argv[1], "UDP") || !strcmp(argv[1], "udp"))) {
        DoUDP(buf);
    } else {
        DoTCP(buf);
    }
}
