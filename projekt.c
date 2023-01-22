#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wait.h>

struct msgbuf
{
    long mtype;
    char mtext[1000];
    char mtext2[1000];
};

int getKey(char *name)
{
    FILE *fp = fopen("configFile.txt", "r");
    char line[50];

    char *array[2];
    int i = 0;
    while (fgets(line, 50, fp) != NULL)
    {

        char *ptr = strtok(line, " : ");
        if (strcmp(ptr, name) == 0)
        {
            while (ptr != NULL)
            {
                array[i++] = ptr;
                ptr = strtok(NULL, " : ");
            }
            break;
        }
    }
    fclose(fp);
    int key = atoi(array[1]);
    if (key == 0)
    {
        printf("Key error\n");
    }
    return key;
}

int spawn_proc(int in, int out, char *cm[])
{
    pid_t pid;

    if ((pid = fork()) == 0)
    {
        if (in != 0)
        {
            dup2(in, 0);
            close(in);
        }

        if (out != 1)
        {
            dup2(out, 1);
            close(out);
        }
        return execvp(cm[0], cm);
    }
    return pid;
}

int key;
int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        printf("Usage: %s <usr>", argv[0]);
    }

    key = getKey(argv[1]);

    // Tworzenie globalnej komunikacji
    int msgID;
    msgID = msgget(key, IPC_CREAT | 0666);
    if (msgID == -1)
    {
        perror("Error: msgget failed");
        exit(1);
    }

    int secondKey;
    int msgID2;
    char input[150];
    char commands[150];
    char mkfifoName[150];
    char *i;
    while (1)
    {
        struct msgbuf message;
        struct msgbuf message2;
        if (fork() == 0)
        {
            // Czytanie z lini polecen
            printf("actual user %s > ", argv[1]);
            // i = readline(" ");
            scanf("%s \"%[^\"]\" %s", input, commands, mkfifoName);

            secondKey = getKey(input);
            // Tworzenie kolejki komunikatów dla wywolanego polecenia
            msgID2 = msgget(secondKey, IPC_CREAT | 0666);
            if (msgID2 == -1)
            {
                perror("Error: msgget failed");
                exit(1);
            }

            // Tworzenie kolejki FIFO
            if (mkfifo(mkfifoName, 0666) == -1)
            {
                perror("Error: mkfifo failed");
                exit(1);
            }
            else
            {
                // Potomny czeka na powrót z FIFO
                // Macierzysty wysyła
                if (fork() == 0)
                {
                    int pdesk2;
                    pdesk2 = open(mkfifoName, O_RDONLY);
                    char buf[10000];

                    int n;
                    while((n = read(pdesk2,buf,sizeof(buf))) > 0)
                    {
                        printf("%s",buf);
                    }
                    close(pdesk2);
                    unlink(mkfifoName);
                    printf("odczytano %s", buf);
                }
                else
                {
                    // Wysyłanie wiadomosci
                    strcpy(message2.mtext, commands);
                    strcpy(message2.mtext2, mkfifoName);
                    message2.mtype = 1;
                    if (msgsnd(msgID2, &message2, sizeof(message2), 0) == -1)
                    {
                        perror("Error: msgsnd failed");
                        exit(1);
                    }
                }
            }
            
        }
        else
        {
            int rozmiar;
            // Odbior wyslanej wiadomoci
            rozmiar = msgrcv(msgID, &message, sizeof(message), 1, 0);
            if (rozmiar == -1)
            {
                perror("Error: msgrcv failed");
                exit(1);
            }
            else
            {
                printf("\nreceive %s\n", message.mtext);
                
                // Podzial komend i zapis ich do tablicy
                char *p = strtok(message.mtext, "|");
                char *q[10];
                int i = 0;

                while (p != NULL)
                {
                    q[i] = p;
                    p = strtok(NULL, "|");
                    i++;
                }
                int k = 0;
                char *cm[10][20];
                int cmdSize[10];

                while (k < i)
                {
                    char *temp = strtok(q[k], " ");
                    int j = 0;
                    while (temp != NULL)
                    {
                        cm[k][j] = temp;
                        temp = strtok(NULL, " ");
                        j++;
                    }
                    cmdSize[k] = j;
                    k++;
                }
                
                // Wykonywanie komend
                int fd[2],in = 0;
                if(fork() == 0)
                {
                    // Zapis przy pomocy kolejki FIFO
                    int pdesk;
                    pdesk = open(message.mtext2, O_WRONLY);
                    if (pdesk == -1)
                    {
                        perror("Otwarcie potoku do zapisu");
                        exit(1);
                    }

                    for (int ile = 0; ile < i - 1; ile++)
                    {
                        pipe(fd);
                        spawn_proc(in,fd[1],cm[ile]);
                        close(fd[1]);
                        in = fd[0];
                    }
                    dup2(pdesk,1);
                    if (in != 0)
                        dup2 (in, 0); 
                    
                    execvp(cm[i-1][0], cm[i-1]);
                }
                else
                {
                    wait(NULL);
                }
            }  
        }
    }
    msgctl(msgID, IPC_RMID, NULL);
    msgctl(msgID2, IPC_RMID, NULL);
}
