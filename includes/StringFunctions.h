#include <sys/stat.h>
#include <bits/types/FILE.h>
#include <stdio.h>

#define MAXLINE   1024
//CP2Server <word1> <word2>
void GetNumWord(char *str, char *buf, int n);
int GetFileSize(int fd);
int GetFileSize(int fd);
int CopyFile(char *path_out, char *path_to);
int CP_CommandDetected(char *buf);