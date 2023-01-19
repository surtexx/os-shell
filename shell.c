#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <readline/readline.h>
#define SIZE 1024

char ** words, **history;
char *command, *output, *p;
char c_path[SIZE];
int cmds_number, words_number, got_error, has_pipe;

void help()
{
	printf("--------------------------------------------------------\n\n");

    printf("history: show command history since running the shell\n");
	printf("ls: lists files and directories in your current directory \n");
	printf("cd: navigate to a path \n");
	printf("pwd: shows path to current directory \n");
	printf("cp: copies a file to another file \n");
    printf("touch: creates a new file \n");
	printf("makedir: creates a new directory \n");
    printf("rm: deletes a file \n");
	printf("rmdir: deletes a directory \n");
    printf("echo: prints a string passed as argument\n");
    printf("quit: you know what this does :) \n");

	printf("\n-----------------------------------------------------\n\n");
}

void hist()
{
    for(int i=0;i<cmds_number;i++)
        printf("%d-> %s\n", i,history[i]);
}

void cd(char* path)
{
    if (chdir(path))
    {
        printf("cd: no such file or directory: %s\n", path);
        got_error = 1;
    }
}

void pwd()
{
    pid_t pid = fork ();
    if (pid == 0)
    {
        char * argv[2];
        argv[0] = "/bin/pwd";
        argv[1] = NULL;
        if (!execvp(argv[0], argv)) {
            got_error = 1;
            printf("pwd: error locating current path\n");
        }
        kill(getpid(), 0);
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
    }
}

void ls()
{
    pid_t pid = fork ();
    if (pid == 0)
    {
        char *argv[2];
        argv[0] = "/bin/ls";
        argv[1] = NULL;
        execvp(argv[0], argv);
        kill(getpid(), 0);
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
    }
}

void touch(char* file)
{
    int fd = open(file, O_CREAT, 0644);
    if (fd != -1)
        printf("File created succesfully\n");
    else
    {
        got_error = 1;
        printf("touch: unable to create file\n");
    }
    close(fd);
}

void rm (char* file)
{
    char path[SIZE];
    if (getcwd(path, sizeof(path)) == NULL) 
    {
        got_error = 1;
        printf("rm: error locating current path\n");
    }
    strcat(path, "/");
    strcat(path, file);
    if(!remove(path))
        printf("File deleted succesfully\n");
    else
    {
        got_error = 1;
        printf("rm: unable to delete file\n");
    }
}

void cp(char* src, char* dest)
{
	char contents[SIZE*100];
	int fsrc, fdest;
	fsrc = open(src, O_RDONLY);
    fdest = open(dest, O_WRONLY | O_CREAT, 0666);
	if (fsrc < 0)
	{
        got_error = 1;
		printf("cp: source file doesn't exist\n");
        return;
	}

    if (fdest < 0)
	{
        got_error = 1;
		printf("cp: error opening destination file\n");
        return;
	}

	int nread = read(fsrc, contents, sizeof(contents));
    if(nread <0){
        got_error = 1;
        printf("cp: error reading contents of source file\n");
        return;
    }
	int nwrite = write(fdest, contents, nread);
    if(nwrite<0){
        got_error = 1;
        printf("cp: error writting contents to destination file\n");
        return;
    }
    printf("File copied succesfully\n");

	close(fsrc);
	close(fdest);
}

void my_mkdir(char* folder)
{
    char path[SIZE];
    if (getcwd(path, sizeof(path)) == NULL) 
    {
        got_error = 1;
        printf("mkdir: error locating current path\n");
    }

    strcat(path, "/");
    strcat(path, folder);
    pid_t pid = fork ();
    if (pid == 0)
    {
        char* argv[3];
        argv[0] = "/bin/mkdir";
        argv[1] = path;
        argv[2] = NULL;
        execvp(argv[0], argv);
        kill(getpid(), 0);
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
    }
}

void my_rmdir(char* folder)
{
    char path[SIZE];
    if (getcwd(path, sizeof(path)) == NULL) 
    {
        got_error = 1;
        printf("rmdir: error locating current path\n");
    }

    strcat(path, "/");
    strcat(path, folder);
    pid_t pid = fork ();
    if (pid == 0)
    {
        char* argv[3];
        argv[0] = "/bin/rmdir";
        argv[1] = path;
        argv[2] = NULL;
        execvp(argv[0], argv);
        kill(getpid(), 0);
    }
    else
    {
        int status;
        waitpid(pid, &status, 0);
    }
}

void echo()
{
    for (int i = 1; i < words_number; i ++)
        printf("%s ", words[i]);
    printf("\n");
}


void execute(char ** words, int nr_args)
{
    if (!strcmp(words[0], "help"))
    {
        if (nr_args != 1)
        {
            got_error = 1;
            printf("help: invalid number of parameters\n");
            return;
        }
        help();
    }
    else if (!strcmp(words[0], "history"))
    {
        if (nr_args != 1)
        {
            got_error = 1;
            printf("history: invalid number of parameters\n");
            return;
        }
        hist();
    }
    else if (!strcmp(words[0], "cd"))
    {
        if (nr_args > 2 && !has_pipe)
        {
            got_error = 1;
            printf("cd: invalid number of parameters\n");
            return;
        }
        if(has_pipe == 1)
        {
            if(nr_args == 2)cd(words[1]);
            else cd("..");
            has_pipe = 2;
        }
        else if(has_pipe == 0)
        {
            if(nr_args == 2)cd(words[1]);
            else cd("..");
        }
    }
    else if (!strcmp(words[0], "pwd"))
    {
        if (nr_args != 1)
        {
            got_error = 1;
            printf("pwd: invalid number of parameters\n");
            return;
        }
        pwd();
    }
    else if(!strcmp(words[0], "ls"))
    {
        if (nr_args != 1)
        {
            got_error = 1;
            printf("ls: invalid number of parameters\n");
            return;
        }
        ls();
    }
    else if (!strcmp(words[0], "touch"))
    {
        if(nr_args != 2 && !has_pipe)
        {
            got_error = 1;
            printf("touch: invalid number of parameters\n");
            return;
        }
        if(has_pipe == 1)
        {
            touch(words[1]);
            has_pipe = 2;
        }
        if(has_pipe == 0)
            touch(words[1]);
    }
    else if (!strcmp(words[0], "cp"))
    {
        if (nr_args != 3 && !has_pipe)
        {
            got_error = 1;
            printf("cp: invalid number of parameters\n");
            return;
        }
        if(has_pipe == 1)
        {   
            cp(words[1], words[2]);
            has_pipe = 2;
        }
        if(has_pipe == 0)
            cp(words[1], words[2]);
    }
    else if (!strcmp(words[0], "mkdir"))
    {
        if(nr_args != 2 && !has_pipe)
        {
            got_error = 1;
            printf("mkdir: invalid number of parameters\n");
            return;
        }
        if(has_pipe == 1){
            my_mkdir(words[1]);
            has_pipe = 2;
        }
        if(has_pipe == 0)
            my_mkdir(words[1]);
    }
    else if (!strcmp(words[0], "rmdir"))
    {
        if(nr_args != 2 && !has_pipe)
        {
            got_error = 1;
            printf("rmdir: invalid number of parameters\n");
            return;
        }
        if(has_pipe == 1)
        {
            my_rmdir(words[1]);
            has_pipe = 2;
        }
        else if(has_pipe == 0)
            my_rmdir(words[1]);
    }
    else if (!strcmp(words[0], "rm"))
    {
        if(nr_args != 2 && !has_pipe)
        {
            got_error = 1;
            printf("rm: invalid number of parameters\n");
            return;
        }
        if(has_pipe == 1)
        {
            rm(words[1]);
            has_pipe = 2;
        }
        else if(has_pipe == 0)
            rm(words[1]);
    }

    else if (!strcmp(words[0], "echo"))
    {
        echo();
    }

    else if (!strcmp(words[0], "quit"))
    {
        exit(0);
    }
    else
    {
        printf("Command '%s' not found\n", words[0]);
        got_error = 1;
        return;
    }
}
int main()
{
    history  = malloc(SIZE * sizeof(char*));
    printf("Welcome to the shell! Use help to discover the commands.\n");
    while(1)
    {
        has_pipe = 0;
        if (getcwd(c_path, sizeof(c_path)) != NULL) 
        {
            char* msg;
            msg = malloc(SIZE * sizeof(char));
            sprintf(msg,"%s $: ", c_path);
            command = readline(msg);
        } 
        else 
            printf("unable to locate current path\n");
        if(!strcmp(command, ""))
        {
            printf("\n");
            continue;
        }
        history[cmds_number] = malloc(SIZE * sizeof(char));
        strcpy(history[cmds_number], command);
        cmds_number++;
        words = malloc(SIZE * sizeof(char*));
        words_number = 0;
        p = strtok(command, " ");
        while(p != NULL)
        {
            char* cpy = malloc(SIZE * sizeof(char));
            strcpy(cpy, p);

            if (!strcmp(cpy, "|"))
            {
                has_pipe = 1;
                if (words_number > 0)
                    execute(words, words_number);

                p = strtok(NULL, " ");
                if (p == NULL)
                {
                    printf("pipe: invalid number of operands\n");
                    continue;
                }
                strcpy(cpy, p);
                words[0] = cpy;

                words_number = 2;
                execute(words, words_number);

                got_error = 0;
                words_number = 0;
            }
            else if (!strcmp(cpy, "||"))
            {
                if(words_number > 0)
                    execute(words, words_number);
                got_error = 0;
                words_number = 0;
            }
            else if (!strcmp(cpy, "&&"))
            {
                execute(words, words_number);
                if(got_error)
                    break;
                words_number = 0;
            }
            else words[words_number ++] = cpy;

            p = strtok(NULL, " ");
        }

        if (!got_error)
            execute(words, words_number);
        else
            got_error = 0;
    }
    return 0;
}
