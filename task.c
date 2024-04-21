#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

// #ifdef MEMWATCH
// #include "memwatch.h"
// #endif

int access_count;
int deny_count;


void sigint_handler(int signal)
{
    if (signal == SIGINT)
    {
        int current_pid = getpid();
        printf("PID_%d: Process stopped\n", current_pid);
        FILE* stat_file = fopen("statistics.log", "a");
        fprintf(stat_file, "PID_%d: Access count: %d Deny count:%d\n", current_pid, access_count, deny_count);
        fclose(stat_file);
        exit(0);
    }
    return;
}

FILE* get_file_ptr(char* filename, char* mode)
{
    FILE* file;
    if (access(filename, F_OK) != 0)
    {
        file = fopen(filename, "w");
        if (file != NULL)
            fclose(file);
    }

    file = fopen(filename, mode);
    return file;
}

int try_to_lock_acquire(char* filename, int current_pid)
{
    FILE* file;
    while (access(filename, F_OK) == 0)
    {
        int pid_blocker = 0;
        file = fopen(filename, "r+");
        if (file == NULL)
            continue;

        fscanf(file, "%d", &pid_blocker);

        if (pid_blocker == 0)
        {
            fclose(file);
            break; // process can acquire
        }
        printf("PID_%d file locked by %d\n", current_pid, pid_blocker);
        fclose(file);
        sleep(1);
        deny_count++;
    }
    file = fopen(filename, "wx");
    if (file == NULL)
        return -1;
    fprintf(file, "%d", current_pid);
    fflush(file);
    fclose(file);
    access_count++;
    return 1;
}

int lock_release(char* filename, int current_pid)
{
    if (access(filename, F_OK) == 0)
    {
        FILE* file = fopen(filename, "r+"); // "r+" if other program acquired lock file
        if (file == NULL)
        {
            FILE* stat_file = fopen("statistics.log", "a");
            fprintf(stat_file, "PID_%d: Access count: %d Deny count:%d\n", current_pid, access_count, deny_count);
            return -1;  
        }
        int file_pid;
        fscanf(file, "%d", &file_pid);
        fclose(file);
        if (file_pid != current_pid)
        {
            printf("PID_%d Lock acquired by both %d and %d\n", current_pid, current_pid, file_pid);
            printf("PID_%d Skipping lock releasing\n", current_pid);
            FILE* stat_file = fopen("statistics.log", "a");
            fprintf(stat_file, "PID_%d: Access count: %d Deny count:%d\n", current_pid, access_count, deny_count);
            return -1;
        }
        else
        {
            remove(filename);
            printf("PID_%d: Lock released successfully\n", current_pid);
        }
    }
    else
        printf("PID_%d Other proccess released lock\n", current_pid);
    return 1;
}
// int get_file_pid_inside(char* filename, int current_pid)
// {
//     FILE* file = get_file_ptr(filename, "r+");
//     int content = 0;
//     if (fscanf(file, "%d", &content) == -1)
//         printf("PID_%d can`t read from %s\n", current_pid, filename);
    
//     fclose(file);
//     return content; // if content == 0 then the file is empty
// }

int main(int argc, char* argv[])
{
    int process_pid = getpid();
    printf("Task with PID=%d started\n", process_pid);
    signal(SIGINT, sigint_handler);
    int arg = 0;
    char* lock_filename = "myfile.lck";
    char* shared_filename = "myfile";

    // args parsing
    while ((arg = getopt(argc, argv, "f:")) != -1)
    {
        switch (arg)
        {
            case 'f':
                printf("PID_%d: Got filename %s\n", process_pid, optarg);

                lock_filename = (char*)malloc(strlen(optarg) + 1 + 4);
                strcpy(lock_filename, optarg);
                strcat(lock_filename, ".lck");
                shared_filename = (char*)malloc(strlen(optarg) + 1);
                strcpy(shared_filename, optarg);

                printf("PID_%d: Got shared file %s\n", process_pid, shared_filename);
                printf("PID_%d: Got lock file %s\n", process_pid, lock_filename);
                break;
        }
    }
    
    // int pid_in_lock = get_file_pid_inside(lock_filename, process_pid);
    // printf("PID_%d: Got lock file PID %d\n", process_pid, pid_in_lock);
    // matching pid
    // if (pid_in_lock == process_pid)
    // {
    //     printf("PID_%d: Lock file PID matches with process PID, reading shared file... %s\n", process_pid, shared_filename);
    //     FILE* shared_file = get_file_ptr(shared_file, "r"); 
    //     sleep(2); // emulates long reading operation
    //     fclose(shared_file);
    //     printf("PID_%d: Reading finished, lock releasing\n", process_pid);
    //     lock_release(lock_filename);
    //     printf("PID_%d: Lock released\n", process_pid);
    // }
    while (1)
    {      
        if (try_to_lock_acquire(lock_filename, process_pid) == 1)
        {
            printf("PID_%d: Lock acquired, reading shared file %s...\n", process_pid, shared_filename);

            FILE* shared_file = get_file_ptr(shared_filename, "r"); 
            sleep(1); // emulates long reading operation
            fclose(shared_file);
            printf("PID_%d: Reading finished, lock releasing\n", process_pid);
            if (lock_release(lock_filename, process_pid) == -1)
            {
                printf("PID_%d: Process stopped\n", process_pid);
                return -1;
            }
            else
                sleep(1);
        }
        else
        {
            continue; // 
        }
    } 
    return 1;
}