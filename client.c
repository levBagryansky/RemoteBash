#include "StringFunctions.h"
#include "Crypt.h"
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

#define START_PORT      1e4+43
#define EXIT_CODE -2

#define TRY(cmd) \
  if (cmd < 0) \
  {              \
    perror(#cmd); \
    return(-1); \
  }

//int port;
struct sockaddr_in serv_addr;
int udp_flag = 0;
socklen_t serv_addr_size = sizeof (serv_addr);
int sock_fd_For_handler;

int DetermineUserAndIpByStr(char *user_ip_str, char *user, char *ip);
int Send2Server(char *buf, int sock_fd, int n);
int Rcv(char *buf, int sock_fd);
int RcvAndWrite(char *buf, int sock_fd);
int CP2Server(char *str, int n, int sock_fd);
int PrintServersByBroadcast();
int BroadcastFirstConnect(int *p_port, char *ip);
int SetConnection();
int GetKeyByDiffieHellman(int sock_fd){
    int p = 19961; //prime number
    int g = 7;
    srand(time(NULL));
    uint a = rand() % 10000;
    uint A = 1;
    for (uint i = 0; i < a; ++i) {
        A = A * g % p;
    }
    printf("A = %d\n", A);

    int arr[3] = {p, g, A};
    Send2Server((char*) arr, sock_fd, sizeof(arr));
    int B;
    Rcv((char *)&B, sock_fd);
    printf("B = %d\n", B);

    int key = 1;
    for (uint i = 0; i < a; ++i) {
        key = key * B % p;
    }
    return key;
}
int SetSslWithServer(int sock_fd);
int DoCommunication(char* buf, char *login);
void sigint_handler(){
    if(sock_fd_For_handler == -1) {
        return;
    }
    char sig_c = SIGINT;
    Send2Server(&sig_c, sock_fd_For_handler, sizeof(sig_c));
    printf("Sent sigint\n");
}

// Must be
// ./Client [-t <UDP|TCP>] [<user>@]<IP>
// ./Client —broadcast
int main(int argc, char** argv) {
    char buf[MAXLINE] = {0};
    if(argc < 2 || argc > 4){
        printf("Wrong format\n");
        return 0;
    }

    if(!strcmp(argv[1], "--broadcast") || !strcmp(argv[1], "--b")){
        PrintServersByBroadcast();
        return 0;
    }

    char *user_ip_str = argv[1];
    if(!strcmp(argv[1], "-t")){
        if(!strcmp(argv[2], "udp") || !strcmp(argv[2], "UDP")){
            printf("UDP mode\n");
            udp_flag++;
        } else if(!strcmp(argv[2], "tcp") || !strcmp(argv[2], "TCP")){
            udp_flag = 0;
        } else{
            printf("Wrong format\n");
            return 0;
        }
        user_ip_str = argv[3];
    }

    char *user_str = (char *) calloc(MAXLINE, sizeof(char));
    char *ip_str = (char *) calloc(MAXLINE, sizeof(char));
    if(DetermineUserAndIpByStr(user_ip_str, user_str, ip_str) == -1){
        free(user_ip_str);
        free(ip_str);
        exit(1);
    }

    if(user_str[0] == 0 || ip_str[0] == 0){
        printf("Wrong user or ip\n");
        free(user_ip_str);
        free(ip_str);
        return 0;
    }

    int port;
    BroadcastFirstConnect(&port, ip_str);
    serv_addr.sin_port = htons(port);
    printf("freeing\n");
    //free(user_str);
    free(ip_str);
    printf("freed\n");
    if(argc == 2 && (!strcmp(argv[1], "UDP") || !strcmp(argv[1], "udp"))) {
        udp_flag++;
    } else {
    }

    DoCommunication(buf, user_str);
}

int DetermineUserAndIpByStr(char *user_ip_str, char *user, char *ip){
    char *at_sign = strchr(user_ip_str, '@');
    if(at_sign){
        for (int i = 0; i < at_sign - user_ip_str; ++i) {
            user[i] = user_ip_str[i];
        }
        int num_of_at_sign = at_sign  - user_ip_str;
        for (int i = 0; user_ip_str[i + num_of_at_sign + 1] != 0; ++i) {
            ip[i] = user_ip_str[i + num_of_at_sign + 1];
        }
    } else{
        TRY(getlogin_r(user, MAXLINE))
        for (int i = 0; user_ip_str[i] != 0 && i < MAXLINE; ++i) {
            ip[i] = user_ip_str[i];
        }
    }
    printf("user = %s\nip = %s\n", user, ip);
    return 0;
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

int Rcv(char *buf, int sock_fd){
    int n;
    if(udp_flag){
        n = recvfrom(sock_fd, buf, MAXLINE, MSG_WAITALL, (struct sockaddr *) &serv_addr, (socklen_t *)&serv_addr_size);
    } else{
        n = read(sock_fd, buf, MAXLINE);
    }
    printf("Rcv: %s", buf);
    return n;
}

int RcvAndWrite(char *buf, int sock_fd){
    int n;
    if(udp_flag){
        n = recvfrom(sock_fd, buf, MAXLINE, MSG_WAITALL, (struct sockaddr *) &serv_addr, (socklen_t *)&serv_addr_size);
    } else{
        n = read(sock_fd, buf, MAXLINE);
    }
    if (!strcmp(buf, "exit\n")) {
        return EXIT_CODE;
    }
    if (!strcmp(buf, "Login exit\n")) {
        printf("Write \"exit\" to exit\n");
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

int PrintServersByBroadcast(){
    int exit_after_distributed = 1;
    printf("Getting available servers..\n");
    memset(&serv_addr, 0, sizeof(serv_addr));

    int sock_fd;
    if ( (sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(START_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    int a = 1;
    TRY(setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST, &a, sizeof(a)))
    //bind(sock_fd, (struct sockaddr*) &serv_addr, sizeof (serv_addr));

    TRY(sendto(sock_fd, &exit_after_distributed, sizeof(int), MSG_CONFIRM, (const struct sockaddr *) &serv_addr, sizeof serv_addr))

    // получаем
    int port;
    socklen_t len = sizeof(serv_addr);
    TRY(recvfrom(sock_fd, &port, sizeof(port), MSG_WAITALL, (struct sockaddr *) &serv_addr, &len))

    printf("port = %d\n", port);
    printf("See server: ip = %s\n", inet_ntoa(serv_addr.sin_addr));
    close(sock_fd);
    return 0;
}

int BroadcastFirstConnect(int *p_port, char* ip){
    printf("Broadcast connecting..\n");
    memset(&serv_addr, 0, sizeof(serv_addr));

    int sock_fd;
    if ( (sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(START_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

    int a = 1;
    TRY(setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST, &a, sizeof(a)))
    int exit_after_distributed = 0;
    TRY(sendto(sock_fd, &exit_after_distributed, sizeof(exit_after_distributed), MSG_CONFIRM, (const struct sockaddr *) &serv_addr, sizeof serv_addr))

    // получаем
    socklen_t len = sizeof(serv_addr);
    while (1) {
        printf("RECEIVING\n");
        TRY(recvfrom(sock_fd, p_port, sizeof(int), MSG_WAITALL, (struct sockaddr *) &serv_addr, &len))
        //printf("ip = %d\nserver_ip = %d\n", htonl(ip), serv_addr.sin_addr.s_addr);
        if(serv_addr.sin_addr.s_addr == inet_addr(ip)){
            break;
        }
    }
    printf("port = %d\n", *p_port);
    printf("Yeah! Get answer from server: ip = %s\n", inet_ntoa(serv_addr.sin_addr));
    close(sock_fd);
    return 0;
}

int SetConnection(){
    char buf[MAXLINE] = {0};
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
            return -1;
        }
        printf("connect_fd = %d\n", connect_fd);
    }
    return sock_fd;
}

int DoCommunication(char* buf, char *login){
    int sock_fd = SetConnection();
    if(sock_fd == -1){
        return 0;
    }

    printf("sock_fd = %d\nNow We have to get key\n", sock_fd);
    int key = GetKeyByDiffieHellman(sock_fd);
    printf("key = %d\nNow We have to be authorized\n", key);
    Send2Server(login, sock_fd, strlen(login));
    free(login);
    int n;

    int forked = fork();
    if(forked) {
        while (1) {
            n = read(STDIN_FILENO, buf, MAXLINE);

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
                break;
            }
            L:
            memset(buf, 0, MAXLINE);
        }
    } else{
        sock_fd_For_handler = -1;
        signal(SIGINT, sigint_handler);
        while (1) {
            if(RcvAndWrite(buf, sock_fd) == EXIT_CODE){
                break;
            }
        }
    }
    return 0;
}
