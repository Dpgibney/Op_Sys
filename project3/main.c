#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<stdint.h>
#define MAX_INPUT_SIZE 200

struct __attribute__((__packed__)) boot_sector_struct{
        uint16_t BPB_BytsPerSec;
        uint8_t BPB_SecPerClus;
        uint16_t BPB_RsvdSecCnt;
        uint8_t BPB_NumFATs;
        uint16_t BPB_RootEntCnt;
        uint16_t BPB_TotSec16;
        uint8_t BPB_Media;
        uint16_t BPB_FATSz16;
        uint16_t BPB_SecPerTrk;
        uint16_t BPB_NumHeads;
        uint32_t BPB_HiddSec;
        uint32_t BPB_TotSec32;
        uint32_t BPB_FATSz32;
        uint16_t BPB_ExtFlags;
        uint16_t BPB_FSVer;
        uint32_t BPB_RootClus;
        uint16_t BPB_FSInfo;
        uint16_t BPB_BkBootSec;
        uint8_t BPB_Reserved[12];
        uint8_t BS_DrvNum;
        uint8_t BS_Reserved1;
        uint8_t BS_BootSig;
        uint32_t BS_VolID;
        uint8_t BS_VolLab[11];
        uint8_t BS_FilSysType[8]; 
};

struct __attribute__((__packed__)) directory{
        char name[11];
        uint8_t attribute;
        uint8_t empty[8];
        uint16_t first_cluster_high;
        uint8_t empty2[4];
        uint16_t first_cluster_low;
        uint32_t size;     
};
int FATSz;
int TotSec;
int DataSec;
int CountofClusters;
int RootDirSectors;
int FirstDataSector;
int BPB_SecPerClus;
int bytes_per_logic;

unsigned int FATOffset(unsigned int N){
        return 4*N;
}

unsigned int FirstSectorofCluster(unsigned int N){
        return (((N-2)*BPB_SecPerClus)+FirstDataSector*bytes_per_logic);
}

void get_info(struct boot_sector_struct info, FILE *fptr){
        if(fseek(fptr,11,SEEK_SET)==0){
                fread(&(info),79,1,fptr);
                bytes_per_logic = info.BPB_BytsPerSec;

                //since only fat32 
                FATSz = info.BPB_FATSz32;
                TotSec = info.BPB_TotSec32;

                //Since for FAT32 it is always 0
                RootDirSectors = 0;

                FirstDataSector = info.BPB_RsvdSecCnt + (info.BPB_NumFATs * FATSz);
                DataSec = TotSec - FirstDataSector;
                CountofClusters = DataSec / info.BPB_SecPerClus; 
                BPB_SecPerClus = info.BPB_SecPerClus;

                printf("BPB_BytsPerSec:\t\t %u\n",info.BPB_BytsPerSec);
                printf("BPB_SecPerClus:\t\t %u\n",info.BPB_SecPerClus);
                printf("BPB_RsvdSecCnt:\t\t %u\n",info.BPB_RsvdSecCnt);
                printf("BPB_NumFATS:\t\t %u\n",info.BPB_NumFATs);
                printf("BPB_RootEntCnt:\t\t %u\n",info.BPB_RootEntCnt);
                printf("BPB_TotSec16:\t\t %u\n",info.BPB_TotSec16);
                printf("BPB_Media:\t\t %x\n",info.BPB_Media);
                printf("BPB_FATSz16:\t\t %u\n",info.BPB_FATSz16);
                printf("BPB_SecPerTrk:\t\t %u\n",info.BPB_SecPerTrk);
                printf("BPB_NumHeads:\t\t %u\n",info.BPB_NumHeads);
                printf("BPB_HiddSec:\t\t %u\n",info.BPB_HiddSec);
                printf("BPB_TotSec32:\t\t %u\n",info.BPB_TotSec32);
                printf("BPB_FATSz32:\t\t %u\n",info.BPB_FATSz32);
                printf("BPB_ExtFlags:\t\t %u\n",info.BPB_ExtFlags);
                printf("BPB_FSVer:\t\t %u\n",info.BPB_FSVer);
                printf("BPB_RootClus:\t\t %u\n",info.BPB_RootClus);
                printf("BPB_FSInfo:\t\t %u\n",info.BPB_FSInfo);
                printf("BPB_Reserved:\t\t %s\n",info.BPB_Reserved);
                printf("BS_DrvNum:\t\t %u\n",info.BS_DrvNum);
                printf("BS_Reserved1:\t\t %u\n",info.BS_Reserved1);
                printf("BS_BootSig:\t\t %u\n",info.BS_BootSig);
                printf("BS_VolID:\t\t %u\n",info.BS_VolID);
                printf("BS_VolLab:\t\t %s\n",info.BS_VolLab);
                printf("BS_FilSysType:\t\t %s\n",info.BS_FilSysType);
                printf("\n\n");

        }else{
                printf("failed to seek to parse boot sector\n");
        }
}

void ls(FILE *fptr, int N){
        //to hold file attributes
        struct directory dir;

        //go to cluster and read out the files in it
        fseek(fptr,FirstSectorofCluster(N),SEEK_SET);
        for(int i = 0; i < 16; i++){
                fread(&dir,32,1,fptr);
                dir.name[10] = '\0';
                printf("%s\n",dir.name);
        } 
}

int main(int argc,char *argv[]){
        char* input_raw = (char*)malloc(MAX_INPUT_SIZE*sizeof(char));
        char* input = (char*)malloc(MAX_INPUT_SIZE*sizeof(char));
        FILE *fptr;
        char* token;
        char* commands[5];
        int i = 0;

        //check that a file was passed as an argument
        if(argc!=2){
                printf("Improper usage: must pass image file as argument\n");
                return -1;
        }

        //boot sector struct
        struct boot_sector_struct info;

        //check that file could open
        //currently using binary mode
        fptr = fopen(argv[1],"rb+");
        if(fptr==NULL){
                printf("Opening file failed\n");
                return -1;
        }
        //parse boot sector
        get_info(info,fptr);

        //will grab newline as well be aware
        fgets(input_raw,MAX_INPUT_SIZE,stdin);
        sscanf(input_raw,"%s",input);

        //main shell like loop to ask user what to do
        while(strcmp(input,"exit")!=0){
                for(i=0; i < 5; i++){
                        commands[i] = 0;
                }
                i = 0;
                token = strtok(input, " ");
                while(token){
                        commands[i++] = token;
                        //printf("TOKEN %s\n", token);
                        token = strtok(NULL, " ");
                }
                /*for(i = 0; i < 5; i++){
                  printf("ARRAY:%s\n",commands[i]);
                  }*/
                if(strcmp(commands[0],"ls")==0){
                        printf("LS!!!\n");
                        //2 is a place holder sector to print out currently that is root
                        ls(fptr,2);
                }
                else if(strcmp(commands[0],"cd")==0){
                        printf("CD!!!\n");
                }
                else if(strcmp(commands[0],"size")==0){
                        printf("SIZE!!!\n");
                }
                else if(strcmp(commands[0],"creat")==0){
                        printf("creat!!!\n");
                }
                else if(strcmp(commands[0],"mkdir")==0){
                        printf("MKDIR!!!\n");
                }
                else if(strcmp(commands[0],"open")==0){
                        printf("open!!!\n");
                }
                else if(strcmp(commands[0],"close")==0){
                        printf("close!!!\n");
                }
                else if(strcmp(commands[0],"read")==0){
                        printf("READ!!!\n");
                }
                else if(strcmp(commands[0],"write")==0){
                        printf("WRITE!!!\n");
                }
                else if(strcmp(commands[0],"rm")==0){
                        printf("RM!!!\n");
                }
                else if(strcmp(commands[0],"rmdir")==0){
                        printf("RMDIR!!!\n");
                }

                //get new input from user
                fgets(input_raw,MAX_INPUT_SIZE,stdin);
                sscanf(input_raw,"%s",input);
        }
        //free memory here
        free(input);
        free(input_raw);

        //close file
        fclose(fptr);

}
