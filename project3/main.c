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
//make it accept arguments for where to look making it do equivilent of ls . for now
void ls(struct boot_sector_struct info, FILE *fptr){
     unsigned long tmp = 0;
     fseek(fptr,tmp,SEEK_SET);
     //char* tmp = (char*)malloc(info.BPB_3.BPB_2.logical_sectors_per_cluster);
     unsigned int location = 0;
     fread(&location,4,1,fptr);
     printf("%x\n",location);
      
}
unsigned int FATOffset(unsigned int N){
    return 4*N;
}
unsigned int FirstSectorofCluster(unsigned int N){
    printf("%d\n",((N-2)*BPB_SecPerClus)+FirstDataSector);
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

        FirstDataSector = info.BPB_RsvdSecCnt + (info.BPB_NumFATs * FATSz);// + RootDirSectors;
        DataSec = TotSec - FirstDataSector;
        CountofClusters = DataSec / info.BPB_SecPerClus; 
        BPB_SecPerClus = info.BPB_SecPerClus;
        
        printf("bytes per logic: %u\n",info.BPB_BytsPerSec);
        printf("logical sectors per cluster: %u\n",info.BPB_SecPerClus);
        printf("reserved_logical_sectors: %u\n",info.BPB_RsvdSecCnt);
        printf("number of file allocation tables: %u\n",info.BPB_NumFATs);
        printf("max number of fat12/16 root entries: %u\n",info.BPB_RootEntCnt);
        printf("total logical sectors: %u\n",info.BPB_TotSec16);
        printf("media descriptors: %x\n",info.BPB_Media);
        printf("logical sectors fat12/16: %u\n",info.BPB_FATSz16);
        printf("physical sectors per track: %u\n",info.BPB_SecPerTrk);
        printf("number of heads: %u\n",info.BPB_NumHeads);
        printf("count hidden sectors: %u\n",info.BPB_HiddSec);
        printf("total logical sectors: %u\n",info.BPB_TotSec32);
        printf("logic sectors per file table: %u\n",info.BPB_FATSz32);
        printf("drive description: %u\n",info.BPB_ExtFlags);
        printf("version: %u\n",info.BPB_FSVer);
        printf("cluster number root dir start: %u\n",info.BPB_RootClus);
        printf("first logic sector: %u\n",info.BPB_FSInfo);
        printf("reserved: %s\n",info.BPB_Reserved);
        printf("cf_24: %u\n",info.BS_DrvNum);
        printf("cf_25: %u\n",info.BS_Reserved1);
        printf("cf_26: %u\n",info.BS_BootSig);
        printf("cf_27: %u\n",info.BS_VolID);
        printf("cf_2b: %s\n",info.BS_VolLab);
        printf("cf_36: %s\n",info.BS_FilSysType);
        
     }
}
int main(int argc,char *argv[]){
char* input_raw = (char*)malloc(MAX_INPUT_SIZE*sizeof(char));
char* input = (char*)malloc(MAX_INPUT_SIZE*sizeof(char));
FILE *fptr;
char* token;
char* commands[5];
int i = 0;
printf("%s\n",argv[1]);
printf("%d\n",argc);

//check that a file was passed as an argument
if(argc!=2){
   printf("Improper usage: must pass image file as argument\n");
   return -1;
}

//creating struct to hold info about filesystem one extra space for endline;
struct boot_sector_struct info;
//info.BPB_Reserved = (char*)malloc(12);
//info.BS_VolLab = (char*)malloc(11);
//info.BS_FilSysType = (char*)malloc(8);


//check that file could open
//currently using binary mode
fptr = fopen(argv[1],"rb+");
if(fptr==NULL){
   printf("Opening file failed\n");
   return -1;
}
get_info(info,fptr);
printf("count of cluster %d\n",CountofClusters);

//will grab newline as well be aware
fgets(input_raw,MAX_INPUT_SIZE,stdin);
sscanf(input_raw,"%s",input);

//main shell like loop to ask user what to do
while(strcmp(input,"exit")!=0){
printf("%s\n",input_raw);
ls(info,fptr);

//get new input from user
fgets(input_raw,MAX_INPUT_SIZE,stdin);
sscanf(input_raw,"%s",input);
	printf("%s\n",input_raw);
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
                struct directory dir;
                fseek(fptr,FirstSectorofCluster(2),SEEK_SET);
                for(int i = 0; i < 16; i++){
                fread(&dir,32,1,fptr);
                dir.name[10] = '\0';
                printf("%s\n",dir.name);
                }
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

//TODO free malloc'd memory in struct
//free(info.BPB_Reserved);
//free(info.BS_VolLab);
//free(info.BS_FilSysType);

//close file
fclose(fptr);

}
