#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

#define SHELL_MAX_INPUT 1024
#define MAX_ARGS 100

void parse_input(char *input, char **args)
{
    int i = 0;
    char *token = strtok(input, " \t\n");
    while (token != NULL && i < MAX_ARGS - 1)
    {
        args[i++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[i] = NULL;
}

void cmd_pwd()
{
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL)
        printf("%s\n", cwd);
    else
        perror("pwd");
}

void cmd_ls()
{
    DIR *dir = opendir(".");
    struct dirent *entry;

    if (dir == NULL)
    {
        perror("ls");
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
            printf("%s  ", entry->d_name);
    }

    printf("\n");
    closedir(dir);
}

void cmd_cd(char **args)
{
    if (args[1] == NULL)
    {
        printf("cd: missing argument\n");
        return;
    }

    if (chdir(args[1]) != 0)
        perror("cd");
}

void cmd_mkdir(char **args)
{
    if (args[1] == NULL)
    {
        printf("mkdir: missing directory name\n");
        return;
    }

    if (mkdir(args[1], 0777) != 0)
        perror("mkdir");
}

void cmd_touch(char **args)
{
    if (args[1] == NULL)
    {
        printf("touch: missing file name\n");
        return;
    }

    int fd = open(args[1], O_CREAT | O_WRONLY, 0666);
    if (fd < 0)
        perror("touch");
    else
        close(fd);
}

void cmd_rm(char **args)
{
    if (args[1] == NULL)
    {
        printf("rm: missing file name\n");
        return;
    }

    if (remove(args[1]) != 0)
        perror("rm");
}

void cmd_cp(char **args)
{
    if (args[1] == NULL || args[2] == NULL)
    {
        printf("cp: source or destination missing\n");
        return;
    }

    FILE *src = fopen(args[1], "rb");
    if (src == NULL)
    {
        perror("cp");
        return;
    }

    FILE *dst = fopen(args[2], "wb");
    if (dst == NULL)
    {
        fclose(src);
        perror("cp");
        return;
    }

    int ch;
    while ((ch = fgetc(src)) != EOF)
        fputc(ch, dst);

    fclose(src);
    fclose(dst);
}

void cmd_mv(char **args)
{
    if (args[1] == NULL || args[2] == NULL)
    {
        printf("mv: source or destination missing\n");
        return;
    }

    if (rename(args[1], args[2]) != 0)
        perror("mv");
}

void cmd_cat(char **args)
{
    if (args[1] == NULL)
    {
        printf("cat: missing file name\n");
        return;
    }

    FILE *fp = fopen(args[1], "r");
    if (fp == NULL)
    {
        perror("cat");
        return;
    }

    int ch;
    while ((ch = fgetc(fp)) != EOF)
        putchar(ch);

    fclose(fp);
}

void cmd_echo(char **args)
{
    int i = 1;
    while (args[i] != NULL)
    {
        printf("%s", args[i]);
        if (args[i + 1] != NULL)
            printf(" ");
        i++;
    }
    printf("\n");
}

void execute_external(char **args)
{
    pid_t pid = fork();

    if (pid == 0)
    {
        execvp(args[0], args);
        perror("exec");
        exit(1);
    }
    else if (pid > 0)
    {
        wait(NULL);
    }
    else
    {
        perror("fork");
    }
}

void execute_background(char **args)
{
    pid_t pid = fork();

    if (pid == 0)
    {
        execvp(args[0], args);
        perror("exec");
        exit(1);
    }
    else if (pid > 0)
    {
        printf("Process running in background\n");
    }
    else
    {
        perror("fork");
    }
}

int main()
{
    char input[SHELL_MAX_INPUT];
    char *args[MAX_ARGS];

    while (1)
    {
        printf("Mehedi@Terminal> ");

        if (fgets(input, sizeof(input), stdin) == NULL)
            break;

        if (strlen(input) == 1)
            continue;

        parse_input(input, args);

        if (args[0] == NULL)
            continue;

        int bg = 0;
        int i = 0;

        while (args[i] != NULL)
        {
            if (strcmp(args[i], "&") == 0)
            {
                bg = 1;
                args[i] = NULL;
                break;
            }
            i++;
        }

        if (strcmp(args[0], "exit") == 0)
            break;
        else if (strcmp(args[0], "pwd") == 0)
            cmd_pwd();
        else if (strcmp(args[0], "ls") == 0)
            cmd_ls();
        else if (strcmp(args[0], "cd") == 0)
            cmd_cd(args);
        else if (strcmp(args[0], "mkdir") == 0)
            cmd_mkdir(args);
        else if (strcmp(args[0], "touch") == 0)
            cmd_touch(args);
        else if (strcmp(args[0], "rm") == 0)
            cmd_rm(args);
        else if (strcmp(args[0], "cp") == 0)
            cmd_cp(args);
        else if (strcmp(args[0], "mv") == 0)
            cmd_mv(args);
        else if (strcmp(args[0], "cat") == 0)
            cmd_cat(args);
        else if (strcmp(args[0], "echo") == 0)
            cmd_echo(args);
        else
        {
            if (bg)
                execute_background(args);
            else
                execute_external(args);
        }
    }

    return 0;
}