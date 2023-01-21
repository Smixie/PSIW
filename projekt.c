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

struct msgbuf
{
    long mtype;
    char mtext[500];
    char mtext2[500];
};

struct command
{
    char *arg[20];
};

int getKey(char *name)
{
    FILE* fp = fopen("configFile.txt","r");
    char line[50];
    
    char *array[2];
    int i=0;
    while (fgets(line, 50, fp)!= NULL){

        char * ptr = strtok(line," : ");
        if(strcmp(ptr,name) == 0){ 
            while(ptr != NULL)
            {
                array[i++] = ptr;
                ptr = strtok(NULL," : ");            
            }
            break;
        } 
    }
    fclose(fp);
    int key = atoi(array[1]);
    if(key == 0)
    {
        printf("It is not int\n");
    }
    return key;
}

int key;
int main(int argc, char* argv[])
{
    
    if(argc < 2)
    {
        printf("Usage: %s <usr>",argv[0]);
    }
    

    key = getKey(argv[1]);

    int msgID;
    
    msgID = msgget(key,IPC_CREAT | 0666);
    if(msgID == -1){
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
        if(fork() == 0)
        {   
            printf("actual user %s > ",argv[1]);
            //i = readline(" ");
            scanf("%s \"%[^\"]\" %s" ,input,commands,mkfifoName);

            secondKey = getKey(input);
            msgID2 = msgget(secondKey,IPC_CREAT | 0666);
            if(msgID2 == -1){
                perror("Error: msgget failed");
                exit(1);
            }
            
            if(mkfifo(mkfifoName,0666) == -1){
                perror("Error: mkfifo failed");
                exit(1);
            }else
            {
                strcpy(message2.mtext,commands);
                strcpy(message2.mtext2,mkfifoName);
                message2.mtype = 1;
                if(msgsnd(msgID2,&message2,sizeof(message2),0) == -1)
                {
                    perror("Error: msgsnd failed");
                    exit(1);
                }
            }
        }
        else{
            int rozmiar;
            rozmiar = msgrcv(msgID,&message,sizeof(message),1,0);
            if(rozmiar == -1)
            {
                perror("Error: msgrcv failed");
                exit(1);
            }
            else
            {
                // int pdesk;
                // pdesk = open(message.mtext2,O_WRONLY);
                // if(pdesk == -1)
                // {
                //     perror("Otwarcie potoku do zapisu");
                //     exit(1);
                // }
                printf("\nreceive %s\n %s\n",message.mtext,message.mtext2);
                struct command cmd[2];

                char *p = strtok(message.mtext,"|");
                char *q[10];
                int i=0;
                while( p != NULL)
                {
                    q[i] = p;
                    p = strtok(NULL,"|");
                    i++;
                }
                int k=0;
                char *cm[10][20];
                while(k<i)
                {
                    
                    char *temp = strtok(q[k]," ");
                    int j=0;
                    while( temp != NULL)
                    {    
                        cm[k][j] = temp;
                        temp = strtok(NULL," ");
                    }
                    k++;
                }
                for(i=0; i<k;i++){
                    for(int j=0; j<sizeof(cm[i]);i++){
                        printf("%s",cm[i][j]);
                    }
                }
            }

            
        }
        
   }
    msgctl(msgID,IPC_RMID,NULL);
    msgctl(msgID2,IPC_RMID,NULL);
}
