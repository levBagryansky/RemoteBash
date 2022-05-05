#include "StringFunctions.h"
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define PORT      1e4
#define EXIT_CODE -2

#define TRY(cmd) \
  if (cmd < 0) \
  {              \
    perror(#cmd); \
    return(-1); \
  }

struct sockaddr_in serv_addr;
int udp_flag = 0;
int len = sizeof (serv_addr);

int Send2Server(char *buf, int sock_fd, int n);
int RcvAndWrite(char *buf, int sock_fd);
int PrintServersByBroadcast(char *buf);
int CP2Server(char *str, int n, int sock_fd);
int BroadcastFirstConnect();
int DoCommunication(char* buf);

// Must be
// ./Client [-t <UDP|TCP>] [<user>@]<IP>
// ./Client —broadcast
int main(int argc, char** argv) {
    char buf[MAXLINE] = {0};
    if(argc < 2 || argc > 3){
        printf("Wring format\n");
        return 0;
    }

    if(!strcmp(argv[1], "--broadcast") || !strcmp(argv[1], "--b")){
        PrintServersByBroadcast(buf);
        return 0;
    }

    char *user_ip_str = argv[1];
    if(!strcmp(argv[1], "-t")){
        if(!strcmp(argv[1], "udp") || !strcmp(argv[1], "UDP")){
            udp_flag++;
        } else if(!strcmp(argv[1], "tcp") || !strcmp(argv[1], "TCP")){
            udp_flag = 0;
        } else{
            printf("Wrong format\n");
            return 0;
        }
        user_ip_str = argv[3];
    }

    BroadcastFirstConnect();
    serv_addr.sin_port = htons(PORT);

    if(argc == 2 && (!strcmp(argv[1], "UDP") || !strcmp(argv[1], "udp"))) {
        udp_flag++;
    } else {
    }
    DoCommunication(buf);
}


int Send2Server(char *buf, int sock_fd, int n){
    int ret;
    if(udp_flag) {
        ret = sendto(sock_fd, buf, n, MSG_CONFIRM, (const struct sockaddr *) &serv_addr, sizeof serv_addr);
    }
    else{
        ret = write(sock_fd, buf, n);
    }
    return ret;
}

int RcvAndWrite(char *buf, int sock_fd){
    int n;
    if(udp_flag){
        n = recvfrom(sock_fd, buf, MAXLINE, MSG_WAITALL, (struct sockaddr *) &serv_addr, (socklen_t *)&len);
    } else{
        n = read(sock_fd, buf, MAXLINE);
    }
    if (!strcmp(buf, "exit\n")) {
        printf("Recver exits\n");
        return EXIT_CODE;
    }
    write(STDOUT_FILENO, buf, n);
    memset(buf, 0, MAXLINE);
    return n;
}

int CP2Server(char *str, int n, int sock_fd){
    char path_from[MAXLINE];
    GetNumWord(str, path_from, 1);
    int path_from_fd = open(path_from, O_RDONLY);
    if(path_from_fd == -1){
        perror("Open error");
        return -1;
    }

    int file_len = GetFileSize(path_from_fd);
    if(file_len < 0){
        printf("GetFileSize returned %d\n", file_len);
        return file_len;
    }
    char *text = (char *) calloc(file_len, sizeof (char));
    if(text == NULL){
        perror("calloc error");
        return -1;
    }
    int num_of_send = (file_len + MAXLINE - 1) / MAXLINE;
    if(read(path_from_fd, text, file_len) == -1){
        perror("read error");
        return -1;
    }
    Send2Server(str, sock_fd, n);

    if(udp_flag) {
        sendto(sock_fd, &num_of_send, sizeof(int), MSG_CONFIRM, (const struct sockaddr *) &serv_addr, sizeof serv_addr);
    }
    else{
        write(sock_fd, &num_of_send, sizeof(int));
    }
    char buf[MAXLINE];
    for (int i = 0; i < (file_len - 1) / MAXLINE; ++i) {
        for (int j = 0; j < MAXLINE; ++j) {
            buf[j] = text[MAXLINE * i + j];
        }
        TRY(Send2Server(buf, sock_fd, MAXLINE))
    }
    for (int i = 0; i < (file_len - 1) % MAXLINE + 1; ++i) {
        buf[i] = text[MAXLINE * (file_len / MAXLINE) + i];
    }
    TRY(Send2Server(buf, sock_fd, file_len % MAXLINE))
    close(path_from_fd);

    return 0;
}

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
    TRY(setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST, &a, sizeof(a)))
    //bind(sock_fd, (struct sockaddr*) &serv_addr, sizeof (serv_addr));

    TRY(sendto(sock_fd, buf, strlen(buf), MSG_CONFIRM, (const struct sockaddr *) &serv_addr, sizeof serv_addr))

    // получаем
    socklen_t len = sizeof(serv_addr);
    TRY(recvfrom(sock_fd, buf, sizeof(buf), MSG_WAITALL, (struct sockaddr *) &serv_addr, &len))
    printf("Yeah! Get answer from server: ip = %s\n", inet_ntoa(serv_addr.sin_addr));
    close(sock_fd);
    return 0;
}

int DoCommunication(char* buf){
    int sock_fd;
    if(udp_flag){
        printf("UDP mode\n");
        sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    } else{
        printf("TCP mode\n");
        sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    }
    if (sock_fd == -1) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    printf("connecting..\n");
    int len = sizeof (serv_addr);
    if(udp_flag){
        char *str = "I know client datas\n\0";
        sendto(sock_fd, str, 21, MSG_CONFIRM, (const struct sockaddr *) &serv_addr, sizeof serv_addr);
        int n = recvfrom(sock_fd, buf, MAXLINE, MSG_WAITALL, (struct sockaddr *) &serv_addr, (socklen_t *)&len);
        write(STDOUT_FILENO, buf, n);
    } else{ //TCP
        int connect_fd = connect(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
        if (connect_fd < 0) {
            perror("connect error");
            return 1;
        }
        printf("connect_fd = %d\n", connect_fd);
    }
    printf("sock_fd = %d\nConnected, you can start\n", sock_fd);

    int forked = fork();
    if(forked) {
        while (1) {
            int n = read(STDIN_FILENO, buf, MAXLINE);
            if(CP_CommandDetected(buf)){
                CP2Server(buf, n, sock_fd);
                printf("You can continue\n");
                goto L;
            }
            if(Send2Server(buf, sock_fd, n) == -1){
                printf("Send2Server error\n");
                break;
            }
            if (!strcmp(buf, "exit\n")) {
                printf("Sender exits\n");
                break;
            }
            L:
            memset(buf, 0, MAXLINE);
        }
    } else{
        while (1) {
            if(RcvAndWrite(buf, sock_fd) == EXIT_CODE){
                break;
            }
        }
    }
    return 0;
}
