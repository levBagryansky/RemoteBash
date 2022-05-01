#include <sys/stat.h>
#include <bits/types/FILE.h>
#include <stdio.h>

#define MAXLINE   1024
char *CP_Command = "CP2Server";

//CP2Server <word1> <word2>
void GetNumWord(char *str, char *buf, int n);
int GetFileSize(int fd);
int GetFileSize(int fd);
int CopyFile(char *path_out, char *path_to);
