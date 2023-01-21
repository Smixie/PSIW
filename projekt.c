#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <unistd.h>

struct msgbuf
{
    long mtype;
    char mtext[500];
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
        return 1;
    }

    int secondKey;
    int msgID2;
    char input[150];
    char commands[150];

    while (1)
    {
        struct msgbuf message;
        
        if(fork() == 0)
        {   
            struct msgbuf message2;
            printf("actual user %s > ",argv[1]);
            scanf("%s %s",input,commands);
            printf("%s" , commands);

            secondKey = getKey(input);
            msgID2 = msgget(secondKey,IPC_CREAT | 0666);
            if(msgID2 == -1){
                perror("Error: msgget failed");
                return 1;
            }
            
            strcpy(message2.mtext,"wass");
            msgsnd(msgID2,&message2,sizeof(message2),0);
        }
        else{
            msgrcv(msgID,&message,sizeof(message),2,0);
            printf("receive %s",message.mtext);
        }
        
   }
    
}