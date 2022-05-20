#include "StringFunctions.h"
#include "Pam_Funcs.h"
#include "Log.h"
#include "Crypt.h"
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>

#define MAXLINE    1024
#define START_PORT 1e4

int udp_flag = 0;
struct sockaddr_in cli_addr;
int len = sizeof (cli_addr);
int exit_after_distributed;
int port = START_PORT;
unsigned char glob_key[MAXLINE];
unsigned char IV[MAXLINE];

int Send2Client(char *buf, int sock_fd, int n);
int Rcv(char *buf, int sock_fd);
int CP_FromClient(char *str, int sock_fd);
int DistributeBroadcastServer(); //С помощью бродкаста передавать данные сервера
int ConnectWithUser(); //Коннектится с юзером
int GetKeyByDiffieHellma(int sock_fd){
    // Bob
    int arr[3];
    Rcv((char*) arr, sock_fd);
    int p = arr[0];
    int g = arr[1];
    int A = arr[2];
    printf("p = %d, g = %d, A = %d\n", p, g, A);
    uint b = rand() % 100000;
    printf("randed %ud", b);
    uint B = 1;
    for (uint i = 0; i < b; ++i) {
        B = B * g % p;
    }
    Send2Client((char *) &B, sock_fd, sizeof (B));
    printf("B = %d\n", B);
    int key = 1;
    for (uint i = 0; i < b; ++i) {
        key = key * A % p;
    }
    return key;
}
int GetMessages(int pipe_write_fd, int acc_fd, int pipe_send_write, int bash_pid); //Получает сообщения и посылает их в pipe_get_fds
int SendMessages(int pipe_read_send_fd, int acc_fd); //из pipe_send_fds посылвет строки клиенту
int Authorize(int pipe_read_get_fd, int pipe_write_send_fd, char *login);
int DoBash(int pipe_read_get_fd, int pipe_write_send_fd);

int main(int argc, char **argv){
    init_log("./.log");
    int forked;

    if(argc == 2 && (!strcmp(argv[1], "UDP") || !strcmp(argv[1], "udp"))){
        printf("UDP mode\n");
        udp_flag = 1;
    } else{
        printf("TCP mode\n");
    }

    //if(DistributeBroadcastServer() < 0){
    //    log_error("DistributeBroadcastServer");
    //}

    log_info("Started work with client\n");
    int pipe_get_fds[2];
    int pipe_send_fds[2];
    if(pipe(pipe_get_fds)){
        perror("pipe_get error");
    }
    if(pipe(pipe_send_fds)){
        perror("pipe_send error");
    }

    int acc_fd = ConnectWithUser();
    if(acc_fd < 0){
        log_error("Cannot connect with user");
    }
    printf("Getting key..\n");
    srand(time(NULL));
    int key1 = GetKeyByDiffieHellma(acc_fd);
    int key2 = GetKeyByDiffieHellma(acc_fd);
    Make_keys(key1, key2, (char*)glob_key, (char*)IV);
    //printf("key = %s\n", glob_key);
    if((forked = fork()) < 0){
        log_perror("Fork error");
        exit(1);
    }
    if(!forked){
        char login[MAXLINE];
        /*read(pipe_get_fds[0], login, MAXLINE);
        if(Authorize(pipe_get_fds[0], pipe_send_fds[1], login) < 0){
            printf("Authorize error\n");
            log_error("Authorize error");
            write(pipe_send_fds[1], "Login exit\n", strlen("Login exit\n"));
            return 0;
        }
        printf("bash pid = %d\n", getpid());
         */
        if(DoBash(pipe_get_fds[0], pipe_send_fds[1]) < 0){
            log_error("Do Bash");
        }
        printf("killed\n");

    } else{

        if(acc_fd < 0){
            perror("ConnectWithUser == -1");
            exit(1);
        }
        int bash_pid = forked;
        if((forked = fork()) < 0){
            log_perror("Fork error");
            exit(1);
        }
        if(!forked) {
            if(SendMessages(pipe_send_fds[0], acc_fd) < 0){
                log_error("SendMessage");
            }
        } else{
            if(GetMessages(pipe_get_fds[1], acc_fd, pipe_send_fds[1], bash_pid) < 0){
                printf("GetMessage returned -1");
                log_error("GetMessage");
            }
        }
    }

}

int Send2Client(char *buf, int sock_fd, int n){
    int ret;
    if(udp_flag) {
        ret = sendto(sock_fd, buf, n, MSG_CONFIRM, (const struct sockaddr *) &cli_addr, sizeof cli_addr);
    }
    else{
        ret = write(sock_fd, buf, n);
    }
    return ret;
}

int Rcv(char *buf, int sock_fd){
    int n;
    if(udp_flag){
        n = recvfrom(sock_fd, buf, MAXLINE, MSG_WAITALL, (struct sockaddr *) &cli_addr, (socklen_t *)&len);
    } else{
        n = read(sock_fd, buf, MAXLINE);
    }
    return n;
}

int CP_FromClient(char *str, int sock_fd){
    char path_to[MAXLINE];
    GetNumWord(str, path_to, 2);
    int path_to_fd = open(path_to, O_WRONLY | O_TRUNC | O_CREAT, 0666);
    if(path_to_fd == -1){
        perror("Open error");
        //Send2Client(&no_error, sock_fd, 1);
        return -1;
    }
    char buf[MAXLINE];
    printf("Sended no_error\n");
    //if(Send2Client(&no_error, sock_fd, 1) < 0){
    //    return -1;
   // }
    int num_of_rcvs;
    if(udp_flag){
        TRY(recvfrom(sock_fd, &num_of_rcvs, sizeof (int), MSG_WAITALL, (struct sockaddr *) &cli_addr, (socklen_t *)&len))
    } else{
        TRY(read(sock_fd,  &num_of_rcvs, sizeof (int)))
    }
    printf("num_of_rcvs = %d\n", num_of_rcvs);

    int n;
    for (int i = 0; i < num_of_rcvs; ++i) {
        TRY((n = Rcv(buf, sock_fd)))
        TRY(write(path_to_fd, buf, n))
    }
    close(path_to_fd);

    char *str_copied = "Copied\n";
    write(STDOUT_FILENO, str_copied, 7);
    return 0;
}

//С помощью бродкаста передавать данные сервера
int DistributeBroadcastServer(){
    int sock_fd_rcv, sock_fd_snd;
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    memset(&cli_addr, 0, sizeof(cli_addr));

    sock_fd_rcv = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock_fd_rcv < 0){
        log_perror("socket creation failed");
        perror("socket creation failed");
        exit(1);
    }

    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(START_PORT);

    int a = 1;
    TRY(setsockopt(sock_fd_rcv, SOL_SOCKET, SO_BROADCAST, &a, sizeof(a)))
    TRY(bind(sock_fd_rcv, (struct sockaddr*) &serv_addr, sizeof(serv_addr)))
    printf("Broadcast connecting..\n");

    socklen_t len = sizeof(cli_addr);
    TRY(recvfrom(sock_fd_rcv, &exit_after_distributed, sizeof(exit_after_distributed), MSG_WAITALL,
                 (struct sockaddr *) &cli_addr, &len))
    printf("Yeah!, got message from broadcast, ip = %s, port = %d\n",
               inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

    TRY((sock_fd_snd = socket(AF_INET, SOCK_DGRAM, 0)))
    port++;
    printf("port = %d\n", port);
    TRY(sendto(sock_fd_snd, &port, sizeof(port), MSG_CONFIRM, (const struct sockaddr *) &cli_addr, sizeof cli_addr))
    close(sock_fd_snd);
    close(sock_fd_rcv);
    log_info("Distributed server ip by broadcast\n");
    return 0;
}

//Коннектится с юзером
int ConnectWithUser(){
    struct sockaddr_in serv_addr;
    int sock_fd;
    if(udp_flag){
        sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    }
    else {
        sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    }
    if (sock_fd == -1) {
        perror("sock_fd == -1, exit");
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    //serv_addr.sin_addr.s_addr = inet_addr("192.168.1.153");
    serv_addr.sin_port = htons(START_PORT);

    bind(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    printf("binded\n");
    if(udp_flag){
        char buf[MAXLINE] = {0};
        int len = sizeof(cli_addr);
        memset(&cli_addr, 0, sizeof(cli_addr));
        int n = recvfrom(sock_fd, (char *) buf, MAXLINE,
                         MSG_WAITALL, (struct sockaddr *) &cli_addr, (socklen_t *) &len);
        write(STDOUT_FILENO, buf, n);
        char* str = "Got answer from server\n";
        int sended = sendto(sock_fd, str, strlen(str), MSG_CONFIRM, (const struct sockaddr *) &cli_addr, sizeof cli_addr);
        printf("sended = %d\n", sended);
        if(sended == -1){
            perror("sendto error");
        }
        return sock_fd;
    }
    listen(sock_fd, 2);
    printf("listened\n");
    int acc_fd = accept(sock_fd, NULL, NULL);
    printf("accepted\n");
    log_info("Connected with user, ip = %s\n", inet_ntoa(cli_addr.sin_addr));
    return acc_fd;
}

//Получает сообщения и посылает их в pipe_get_fds
int GetMessages(int pipe_write_fd, int acc_fd, int pipe_send_write, int bash_pid) {
    unsigned char encrypt_buf[MAXLINE] = {0};
    unsigned char buf[MAXLINE] = {0};
    while (1){
        int n = Rcv((char*)encrypt_buf, acc_fd);
        printf("recved: %s\nn = %d\n", encrypt_buf, n);
        decrypt(encrypt_buf, n, glob_key, IV, buf);

        for (size_t i = 0; i < strlen((char*)buf); i++){
            printf("%zu - (%d) %c\n", i, buf[i], buf[i]);
        }

        char * space = strchr((char*)buf, '\n');
        int true_len = space - (char*)buf + 1;

        for (size_t i = true_len; i < strlen(buf); i++){
            buf[i] = '\0';
        }

        for (size_t i = 0; i < strlen((char*)buf); i++){
            printf("%zu - (%d) %c\n", i, buf[i], buf[i]);
        }

        if(n < 0){
            log_perror("Rcv error");
            return -1;
        }
        if(!strcmp((char*)buf, "exit\n")){
            if(write(pipe_write_fd, buf, true_len) < 0 || write(pipe_send_write, buf, n) < 0)
                log_perror("exit write error");
            write(STDOUT_FILENO, buf, n);
            printf("GetMessages Exits\n");
            log_info("Exited\n");
            return 0;
        }
        if(n == 1 && *buf == SIGINT){
            char ctrl_c = 67;
            write(pipe_write_fd, &ctrl_c, 1);
            //kill(bash_pid, SIGINT);
            //printf("Killed sigint to bash with pid %d\n", bash_pid);
            return 0;
        }
        printf("buf = %s\n", buf);
        write(STDOUT_FILENO, buf, true_len);
        if(CP_CommandDetected((char*)buf)){
            if(CP_FromClient((char*)buf, acc_fd) < 0) {
                log_error("CP_FromClient error");
            }
        }else {
            if(write(pipe_write_fd, buf, true_len) < 0){
                log_perror("Write error");
            }
        }
        memset(buf, 0, n);
        memset(encrypt_buf, 0, n);

    }
}

//из pipe_send_fds посылвет строки клиенту
int SendMessages(int pipe_read_send_fd, int acc_fd){
    char str[MAXLINE];
    char encrypt_buf[MAXLINE];
    while (1) {
        int n = read(pipe_read_send_fd, str, MAXLINE);
        if(n < 0){
            log_perror("read error");
            perror("read error");
        }
        int siph_n = encrypt((unsigned char *)str, n, glob_key, IV, (unsigned char*)encrypt_buf);
        if(udp_flag){
            TRY(sendto(acc_fd, encrypt_buf, siph_n, MSG_CONFIRM, (const struct sockaddr *) &cli_addr, sizeof cli_addr))
        }else {
            TRY(write(acc_fd, encrypt_buf, siph_n))
        }
        if(!strcmp(str, "exit\n")){
            printf("SendMessages Exits\n");
            return 0;
        }
        printf("Send %s\n", str);
        TRY(write(STDOUT_FILENO, str, n))
        memset(str, 0, MAXLINE);
        memset(encrypt_buf, 0, MAXLINE);
    }
}

int Authorize(int pipe_read_get_fd, int pipe_write_send_fd, char *login){
    int clone_stdin = dup(STDIN_FILENO);
    int clone_stdout = dup(STDOUT_FILENO);
    TRY(dup2(pipe_read_get_fd, STDIN_FILENO))
    TRY(dup2(pipe_write_send_fd, STDOUT_FILENO))
    TRY(dup2(pipe_write_send_fd, STDERR_FILENO))

    int logined;
    struct passwd *info;
    info = getpwnam(login);
    for (int i = 0; i < 2; ++i) {
        logined = login_into_user(login);
        if(logined == 0){
            break;
        }
        if(logined < 0){
            TRY(dup2(clone_stdin, STDIN_FILENO))
            TRY(dup2(clone_stdout, STDOUT_FILENO))
            return -1;
        }
    }

    if(logined != 0){
        TRY(dup2(clone_stdin, STDIN_FILENO))
        TRY(dup2(clone_stdout, STDOUT_FILENO))
        return -1;
    }

    log_info("user \"%s\" logged in\n", login);
    setgid(info -> pw_gid);
    setuid(info -> pw_uid);
    TRY(dup2(clone_stdin, STDIN_FILENO))
    TRY(dup2(clone_stdout, STDOUT_FILENO))
    return 0;
}

int DoBash(int pipe_read_get_fd, int pipe_write_send_fd){
    int clone_stdin = dup(STDIN_FILENO);
    int clone_stdout = dup(STDOUT_FILENO);
    TRY(dup2(pipe_read_get_fd, STDIN_FILENO))
    TRY(dup2(pipe_write_send_fd, STDOUT_FILENO))
    TRY(dup2(pipe_write_send_fd, STDERR_FILENO))
    char *bash_args[] = {"sh", NULL};
    TRY(execvp("sh", bash_args))
    TRY(dup2(clone_stdin, STDIN_FILENO))
    TRY(dup2(clone_stdout, STDOUT_FILENO))
    return -1;
}