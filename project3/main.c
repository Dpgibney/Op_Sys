#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<stdint.h>
#include<ctype.h>
#include<string.h>
#include<stdbool.h>
#define MAX_INPUT_SIZE 200
#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN    0x02
#define ATTR_SYSTEM    0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE   0x20
#define ATTR_LONG_NAME 0x0F
#define END_FAT        0x0FFFFFF8
#define TRUE_FAT_END   0x0FFFFFFF
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
        char name[8];
        char extention[3];
        uint8_t attribute;
        uint8_t empty[8];
        int16_t first_cluster_high;
        uint8_t empty2[4];
        uint16_t first_cluster_low;
        uint32_t size;     
};

struct __attribute__((__packed__)) long_name{
        uint8_t LDIR_Ord;
        char LDIR_Name1[10];
        uint8_t LDIR_Attr;
        uint8_t LDIR_Type;
        uint8_t LDIR_Chksum;
        char LDIR_Name2[12];
        uint16_t LDIR_FstClusLO;
        uint32_t LDIR_Name3;       
};
int FATSz;
int TotSec;
int DataSec;
int CountofClusters;
int RootDirSectors;
int FirstDataSector;
int BPB_SecPerClus;
int bytes_per_logic;

//struct for storing open files
struct openfiles{
	char* name;
	int mode;
	unsigned int first_cluster_num;
};

//just for compiling for the create function david will have to change this
unsigned int current_dir_fat;
unsigned int start_dir_fat;

unsigned int ThisFATSecNum(unsigned int N, struct boot_sector_struct* info){
        return (info->BPB_RsvdSecCnt+((4*N)/info->BPB_BytsPerSec))*512;
}

unsigned int ThisFATEntOffset(unsigned int N, struct boot_sector_struct* info){
        return (4*N)%info->BPB_BytsPerSec;
}

unsigned int FirstSectorofCluster(unsigned int N){
        return (((N-2)*BPB_SecPerClus)*bytes_per_logic+FirstDataSector*bytes_per_logic);
}

unsigned int find_empty_fat(unsigned int start_dir, struct boot_sector_struct* info, FILE* fptr){
	uint32_t dir_on = start_dir_fat;
	unsigned int tmp;
	do{
		fseek(fptr,dir_on,SEEK_SET);
		fread(&(tmp),4,1,fptr);
		if(tmp==0){
			return dir_on;
		}
		dir_on += 4;
	}while(true);
}


//TODO only checks against files not directories
unsigned int find_empty_cluster(unsigned int current_dir, struct boot_sector_struct* info, FILE* fptr, char* filename){
        uint32_t dir_on = current_dir_fat;
        uint32_t return_dir;
        unsigned int tmp;
        char tmp1[13];
        do{
        fseek(fptr,dir_on,SEEK_SET);
        fread(&(tmp),4,1,fptr);
        struct directory dir;
        //go to cluster and read out the files in it
        fseek(fptr,FirstSectorofCluster((dir_on-start_dir_fat)/4+2),SEEK_SET);
        return_dir = FirstSectorofCluster((dir_on-start_dir_fat)/4+2);
        int length = 8;
        
        //so that it will check the sector fully
        for(int i = 0; i < (info->BPB_BytsPerSec/32); i++){
                fread(&dir,32,1,fptr);
        
                for(int i = 0; i < 8; i++){
                    if(dir.name[i]==' '){
                        length = i;
                        break;
                    }
                }
                if(dir.attribute != ATTR_HIDDEN && dir.attribute != ATTR_DIRECTORY && dir.attribute != 0x80 && dir.attribute != ATTR_LONG_NAME){
                    memcpy(tmp1,dir.name,length);
                    if(dir.extention[0]==' '){
                        tmp1[length] = '\0';
                    }else{
                        tmp1[length] = '.';
                        memcpy(&tmp1[length+1],dir.extention,3);
                        tmp1[length+4] = '\0';
                    }
                    for(int i = 0; i < 12; i++){
                        tmp1[i] = tolower(tmp1[i]);
                    } 
                    if(strcmp(tmp1,filename)==0){
                       return -1;
                    }    
                }if(dir.attribute == ATTR_DIRECTORY){
                    memcpy(tmp1,dir.name,length);
                        tmp1[length] = '\0';
                    for(int i = 0; i < 12; i++){
                        tmp1[i] = tolower(tmp1[i]);
                    } 
                    if(strcmp(filename,tmp1)==0){
                          return -1;
                    }
                }
                if(dir.name[0]==0x00){
                    return return_dir;
                }
                return_dir += 32;
        }
        if(tmp < END_FAT){
            dir_on = start_dir_fat + (tmp*4-8);
        }
        }while(tmp < END_FAT);
        //if no empty cluster found make one
        uint32_t new_fat = find_empty_fat(dir_on, info, fptr);
        //printf("new fat %x\n",new_fat);
        uint32_t cluster_num = ((new_fat-start_dir_fat)/4+2);
        uint32_t first_cluster = FirstSectorofCluster(cluster_num);;
        fseek(fptr,dir_on,SEEK_SET);
        fwrite(&cluster_num,4,1,fptr);
        fseek(fptr,new_fat,SEEK_SET);
        //printf("new fat: %x\n",new_fat);
        uint32_t end_fat = 0xFFFFFFFF;
        fwrite(&end_fat,4,1,fptr);
        return first_cluster;
        
             
}

//ChkSum function from the fat write up provided
unsigned char ChkSum (unsigned char *name){
      short FcbNameLen;
      unsigned char Sum;
 
      Sum = 0;
      for(FcbNameLen=11; FcbNameLen!=0; FcbNameLen--){
          Sum = ((Sum & 1) ? 0x80 : 0) + (Sum >> 1) + *name++;
      }
      return (Sum);
}

void get_info(struct boot_sector_struct* info, FILE *fptr){
        if(fseek(fptr,11,SEEK_SET)==0){
                fread(&(*info),79,1,fptr);
                bytes_per_logic = info->BPB_BytsPerSec;

                //since only fat32 
                FATSz = info->BPB_FATSz32;
                TotSec = info->BPB_TotSec32;

                //Since for FAT32 it is always 0
                RootDirSectors = 0;

                FirstDataSector = info->BPB_RsvdSecCnt + (info->BPB_NumFATs * FATSz);
                DataSec = TotSec - FirstDataSector;
                CountofClusters = DataSec / info->BPB_SecPerClus; 
                BPB_SecPerClus = info->BPB_SecPerClus;
                start_dir_fat = ThisFATSecNum(2,info) + ThisFATEntOffset(2,info);
                current_dir_fat = ThisFATSecNum(2,info) + ThisFATEntOffset(2,info);

                //printf("BPB_BytsPerSec:\t\t %u\n",info->BPB_BytsPerSec);
                //printf("BPB_SecPerClus:\t\t %u\n",info->BPB_SecPerClus);
                //printf("BPB_RsvdSecCnt:\t\t %u\n",info->BPB_RsvdSecCnt);
                //printf("BPB_NumFATS:\t\t %u\n",info->BPB_NumFATs);
                //printf("BPB_RootEntCnt:\t\t %u\n",info->BPB_RootEntCnt);
                //printf("BPB_TotSec16:\t\t %u\n",info->BPB_TotSec16);
                //printf("BPB_Media:\t\t %x\n",info->BPB_Media);
                //printf("BPB_FATSz16:\t\t %u\n",info->BPB_FATSz16);
                //printf("BPB_SecPerTrk:\t\t %u\n",info->BPB_SecPerTrk);
                //printf("BPB_NumHeads:\t\t %u\n",info->BPB_NumHeads);
                //printf("BPB_HiddSec:\t\t %u\n",info->BPB_HiddSec);
                //printf("BPB_TotSec32:\t\t %u\n",info->BPB_TotSec32);
                //printf("BPB_FATSz32:\t\t %u\n",info->BPB_FATSz32);
                //printf("BPB_ExtFlags:\t\t %u\n",info->BPB_ExtFlags);
                //printf("BPB_FSVer:\t\t %u\n",info->BPB_FSVer);
                //printf("BPB_RootClus:\t\t %u\n",info->BPB_RootClus);
                //printf("BPB_FSInfo:\t\t %u\n",info->BPB_FSInfo);
                //printf("BPB_Reserved:\t\t %s\n",info->BPB_Reserved);
                //printf("BS_DrvNum:\t\t %u\n",info->BS_DrvNum);
                //printf("BS_Reserved1:\t\t %u\n",info->BS_Reserved1);
                //printf("BS_BootSig:\t\t %u\n",info->BS_BootSig);
                //printf("BS_VolID:\t\t %u\n",info->BS_VolID);
                //printf("BS_VolLab:\t\t %s\n",info->BS_VolLab);
                //printf("BS_FilSysType:\t\t %s\n",info->BS_FilSysType);
                //printf("\n\n");

        }else{
                printf("failed to seek to parse boot sector\n");
        }
}

void print_info(struct boot_sector_struct* info, FILE *fptr){
                printf("BPB_BytsPerSec:\t\t %u\n",info->BPB_BytsPerSec);
                printf("BPB_SecPerClus:\t\t %u\n",info->BPB_SecPerClus);
                printf("BPB_RsvdSecCnt:\t\t %u\n",info->BPB_RsvdSecCnt);
                printf("BPB_NumFATS:\t\t %u\n",info->BPB_NumFATs);
                printf("BPB_RootEntCnt:\t\t %u\n",info->BPB_RootEntCnt);
                printf("BPB_TotSec16:\t\t %u\n",info->BPB_TotSec16);
                printf("BPB_Media:\t\t %x\n",info->BPB_Media);
                printf("BPB_FATSz16:\t\t %u\n",info->BPB_FATSz16);
                printf("BPB_SecPerTrk:\t\t %u\n",info->BPB_SecPerTrk);
                printf("BPB_NumHeads:\t\t %u\n",info->BPB_NumHeads);
                printf("BPB_HiddSec:\t\t %u\n",info->BPB_HiddSec);
                printf("BPB_TotSec32:\t\t %u\n",info->BPB_TotSec32);
                printf("BPB_FATSz32:\t\t %u\n",info->BPB_FATSz32);
                printf("BPB_ExtFlags:\t\t %u\n",info->BPB_ExtFlags);
                printf("BPB_FSVer:\t\t %u\n",info->BPB_FSVer);
                printf("BPB_RootClus:\t\t %u\n",info->BPB_RootClus);
                printf("BPB_FSInfo:\t\t %u\n",info->BPB_FSInfo);
                printf("BPB_Reserved:\t\t %s\n",info->BPB_Reserved);
                printf("BS_DrvNum:\t\t %u\n",info->BS_DrvNum);
                printf("BS_Reserved1:\t\t %u\n",info->BS_Reserved1);
                printf("BS_BootSig:\t\t %u\n",info->BS_BootSig);
                printf("BS_VolID:\t\t %u\n",info->BS_VolID);
                printf("BS_VolLab:\t\t %s\n",info->BS_VolLab);
                printf("BS_FilSysType:\t\t %s\n",info->BS_FilSysType);
}

void ls(FILE *fptr, int N, struct boot_sector_struct* info){
	//to hold file attributes
	struct directory dir;

	//to hold file name and extention
	char tmp[13];
	int length = 8;

	//go to cluster and read out the files in it
	fseek(fptr,FirstSectorofCluster(N),SEEK_SET);

	//so that it will check the sector fully
	for(int i = 0; i < (info->BPB_BytsPerSec/32); i++){
		fread(&dir,32,1,fptr);
		for(int i = 0; i < 8; i++){
			if(dir.name[i]==' '){
				length = i;
				break;
			}
		}
		if(dir.attribute != 0x00 && dir.attribute != ATTR_HIDDEN && dir.attribute != ATTR_DIRECTORY && dir.attribute != 0x80 && dir.attribute != ATTR_LONG_NAME){
			memcpy(tmp,dir.name,length);
			if(dir.extention[0]==' '){
				tmp[length] = '\0';
			}else{
				tmp[length] = '.';
				memcpy(&tmp[length+1],dir.extention,3);
				tmp[length+4] = '\0';
			}
			for(int i = 0; i < 12; i++){
				tmp[i] = tolower(tmp[i]);
			} 
			printf("%s\n",tmp);
		}else if(dir.attribute == ATTR_DIRECTORY){
			memcpy(tmp,dir.name,length);
			for(int i = 0; i < length; i++){
				tmp[i] = tolower(tmp[i]);
			} 
			tmp[length] = '\0';
			printf("%s\n",tmp);
		}
	} 
}

int checkForFile(FILE *fptr, int N, struct boot_sector_struct* info, char* filename, unsigned int* super_tmp){
        //to hold file attributes
        struct directory dir;

        //to hold file name and extention
        char tmp[13];
        int length = 8;

        //go to cluster and read out the files in it
        fseek(fptr,FirstSectorofCluster(N),SEEK_SET);

        //so that it will check the sector fully
        //printf("what is n%d\n",N);
        for(int i = 0; i < (info->BPB_BytsPerSec/32); i++){
                fread(&dir,32,1,fptr);
                for(int i = 0; i < 8; i++){
                    if(dir.name[i]==' '){
                        length = i;
                        break;
                    }
                }
                if(dir.attribute != 0x02 && dir.attribute != 0x10 && dir.attribute != 0x80 && dir.attribute != 0x0F){
                    memcpy(tmp,dir.name,length);
                    if(dir.extention[0]==' '){
                        tmp[length] = '\0';
                    }else{
                        tmp[length] = '.';
                        memcpy(&tmp[length+1],dir.extention,3);
                        tmp[length+4] = '\0';
                    }
	
                    for(int i = 0; i < 12; i++){
                        tmp[i] = tolower(tmp[i]);
                    }
                    //printf("%s\n",tmp);
                    //printf("%x\n",dir.first_cluster_high);
                    //printf("%x\n",dir.first_cluster_low);
                }
		if(strcmp(tmp,filename)==0){
			*super_tmp = dir.first_cluster_high >> 8;
                    	*super_tmp += dir.first_cluster_low;
			return 1;
		}	
        }
	return 0;
}

//TODO create only will find a free sector in the current block need to fix that
//also fix create with file extentions
void create(char* filename, struct boot_sector_struct* info, FILE* fptr, unsigned int empty_cluster_num){
	if(empty_cluster_num == -1){printf("error file already exists\n");}
	else{
		//printf("CREATE DOES NOT COMPLETELY WORK YET");
                //First this sets up the file to be added to the directory
                struct directory dir;
                char filestart[9];
                for(int i = 0; i < 8; i++){
                        filestart[i] = ' ';
                }
                char fileexten[4];
                for(int i = 0; i < 3; i++){
                        fileexten[i] = ' ';
                }
                int i = 0;
                int j = 0;
                bool has_exten = false;
                while(filename[i]!='\0'){
                        if(filename[i]=='.'){
                                memcpy(filestart,filename,i);
                                j = i;
                                has_exten = true;
                        }
                        i++;
                        if(filename[i]=='\0'){
                                memcpy(filestart,filename,i);
                        }
                }
                if(has_exten){
                        memcpy(fileexten,&filename[j+1],i-j);
                }

	//	struct directory dir;
	//	char filestart[9];
	//	for(int i = 0; i < 8; i++){
        //                if(i<strlen(filename)){
	//		     filestart[i] = filename[i];
        //                }else{
	//		     filestart[i] = ' ';
        //                }
	//	}
	//	char fileexten[4];
	//	for(int i = 0; i < 3; i++){
	//		fileexten[i] = ' ';
	//	}
                //finds empty fat to set the cluster for the new file
		uint32_t empty_fat = find_empty_fat(start_dir_fat,info,fptr);
		dir.first_cluster_high = empty_fat >> 16;
		dir.first_cluster_low = (empty_fat << 16) >> 16;
		dir.size = 0;
		for(int i = 0; i < 8; i++){
			dir.name[i] = filestart[i];
		}
		for(int i = 0; i < 3; i++){
			dir.extention[i] = fileexten[i];
		}
		dir.attribute = ATTR_ARCHIVE;
		fseek(fptr,empty_cluster_num,SEEK_SET);
                //setting up the long name to be added in front of the actual directory entry
                //this will NOT accept long names correctly it is only implemented to get ls in 
                //an actual file manager to work
		struct long_name longname;
		longname.LDIR_Ord = 0x40;
		for(int i = 0; i < 10; i+=2){
			longname.LDIR_Name1[i] = filename[i/2];
			longname.LDIR_Name1[i+1] = 0x00;
		}
		longname.LDIR_Attr = ATTR_LONG_NAME;
		longname.LDIR_Chksum = ChkSum(filename);
		for(int i = 0; i < 12; i+=2){
			longname.LDIR_Name2[i] = filename[5+i/2];
			longname.LDIR_Name2[i+1] = 0x00;
		}
		longname.LDIR_FstClusLO = 0;
		longname.LDIR_Name3 = 0xFFFFFFFF;
                //write the long name and then file to the fat32 system
		fwrite(&longname,sizeof(struct long_name),1,fptr);
		fwrite(&dir,sizeof(struct directory),1,fptr);
                //puts eof value in the fat table
		fseek(fptr,empty_fat,SEEK_SET);
		uint32_t eof = 0xFFFFFF0F;
		fwrite(&eof,sizeof(uint32_t),1,fptr);
	}
}

//TODO currently it is recognized as a directory but doesnt seem to know what to do about that
void mkdir(char* filename, struct boot_sector_struct* info, FILE* fptr, unsigned int empty_cluster_num){
	if(empty_cluster_num == -1){printf("error file already exists\n");}
	else{
		//printf("mkdir DOES NOT COMPLETELY WORK YET");
                //First this sets up the directory to be added to the directory
                struct directory dir;
                char filestart[9];
                for(int i = 0; i < 8; i++){
                        if(i<strlen(filename)){
                             filestart[i] = filename[i];
                        }else{
                             filestart[i] = ' ';
                        }
                }
                char fileexten[4];
                for(int i = 0; i < 3; i++){
                        fileexten[i] = ' ';
                }

                //finds empty fat to set the cluster for the new file
		uint32_t empty_fat = find_empty_fat(start_dir_fat,info,fptr);
                uint32_t empty_cluster = (empty_fat-start_dir_fat)/4+2;
		dir.first_cluster_high = empty_cluster >> 16;
		dir.first_cluster_low = (empty_cluster << 16) >> 16;
		dir.size = 0;
                //printf("empty_fat: %x cluster_high %x cluster_low %x\n",empty_fat,dir.first_cluster_high,dir.first_cluster_low);
		for(int i = 0; i < 8; i++){
			dir.name[i] = filestart[i];
		}
		for(int i = 0; i < 3; i++){
			dir.extention[i] = fileexten[i];
		}
		dir.attribute = ATTR_DIRECTORY;
		fseek(fptr,empty_cluster_num,SEEK_SET);
                //setting up the long name to be added in front of the actual directory entry
                //this will NOT accept long names correctly it is only implemented to get ls in 
                ////an actual file manager to work
		//struct long_name longname;
		//longname.LDIR_Ord = 0x40;
		//for(int i = 0; i < 10; i+=2){
		//	longname.LDIR_Name1[i] = filename[i/2];
		//	longname.LDIR_Name1[i+1] = 0x00;
		//}
		//longname.LDIR_Attr = ATTR_LONG_NAME;
		//longname.LDIR_Chksum = ChkSum(filename);
		//for(int i = 0; i < 12; i+=2){
		//	longname.LDIR_Name2[i] = filename[5+i/2];
		//	longname.LDIR_Name2[i+1] = 0x00;
		//}
		//longname.LDIR_FstClusLO = 0;
		//longname.LDIR_Name3 = 0xFFFFFFFF;
                //write the long name and then file to the fat32 system
		//fwrite(&longname,sizeof(struct long_name),1,fptr);
		fwrite(&dir,sizeof(struct directory),1,fptr);
                //puts eof value in the fat table
		fseek(fptr,empty_fat,SEEK_SET);
		uint32_t eof = 0x0FFFFFFF;
		fwrite(&eof,sizeof(uint32_t),1,fptr);

                //write the . and .. files to the new directory
		fseek(fptr,FirstSectorofCluster((empty_fat-start_dir_fat)/4+2),SEEK_SET);
                //printf("first sector %x\n",FirstSectorofCluster((empty_fat-start_dir_fat)/4+2));
                //for . file
                dir.name[0] = '.';
                for(int i = 1; i < 8; i++){
                    dir.name[i] = ' ';
                }
		fwrite(&dir,sizeof(struct directory),1,fptr);
                uint32_t current_cluster = (current_dir_fat-start_dir_fat)/4+2;
		dir.first_cluster_high = current_cluster >> 16;
		dir.first_cluster_low = (current_cluster << 16) >> 16;
                dir.name[1] = '.';
		fwrite(&dir,sizeof(struct directory),1,fptr);
                 
	}
}

void openfile(char* filename, int mode, struct openfiles** of, struct boot_sector_struct* info, FILE* fptr, int *filecount){
	if(mode == 0){
		printf("Invalid mode\n");
	}
	else{
	uint32_t dir_on = current_dir_fat;
	uint32_t tmp;
	unsigned int super_tmp;
	unsigned int super_tmp2;
	int open = 0;
	int i = 0;
	struct directory dirtmp;
		do{
		fread(&dirtmp,32,1,fptr);
             	fread(&(tmp),4,1,fptr);
                //printf("Current Directory: %x\n",dir_on);
		//TODO make sure file isnt read only if mode is 1 or 2
                //currently assuming all in the same fat
                if(checkForFile(fptr,((dir_on-start_dir_fat)/4+2),info, filename, &super_tmp)==1){
			//for loop to check if its in of open =1
			for(i = 0; i < *filecount; i++){
				if(super_tmp == (*of)[i].first_cluster_num){
					printf("File is already open\n");
					return;
				}

			}
			super_tmp2 = dirtmp.first_cluster_high >> 8;
			super_tmp2 += dirtmp.first_cluster_low;
			if(super_tmp2 & 0x01 == 0x01 && mode == 2){
			}
			else{
				printf("Error: file is read-only\n");
				break;
			}
			if(open == 0){
				//create new entry
				struct openfiles *oftmp = malloc((*filecount+1)*sizeof(struct openfiles));
				char * tmp_name = (char*)malloc(sizeof(char)); 
				for(i = 0; i < *filecount; i++){
					oftmp[i].name = (*of)[i].name;
					oftmp[i].mode = (*of)[i].mode;
					oftmp[i].first_cluster_num = (*of)[i].first_cluster_num;
				}
				oftmp[*filecount].name = filename;
				oftmp[*filecount].mode = mode;
				oftmp[*filecount].first_cluster_num = super_tmp;
				//for(i = 0; i < *filecount; i++){
				//	free((*of)[i].name);
				//}
				free(*of);
				*of = oftmp;
				*filecount = *filecount + 1;
			}
		}
		else{
			printf("File does not exist\n");
		}
                if(tmp < 0x0FFFFFF8){
               		dir_on = start_dir_fat + (tmp*4-8);
           	}
         	}while(tmp < 0x0FFFFFF8);
	}
	
}
void closefile(unsigned int super_tmp, struct openfiles** of, int * filecount){
	struct openfiles *oftmp = malloc((*filecount-1)*sizeof(struct openfiles));
        int j = 0;
	int i = 0;
	for(i = 0; i < *filecount; i++){
		if(super_tmp != (*of)[i].first_cluster_num){
                	oftmp[j].mode = (*of)[i].mode;
           		oftmp[j].first_cluster_num = (*of)[i].first_cluster_num;
			j++;
		}
        }
	free(*of);
        *of = oftmp;
        *filecount = *filecount - 1;

}
unsigned int cd(unsigned int current_dir, struct boot_sector_struct* info, FILE* fptr, char* directory){
	uint32_t dir_on = current_dir_fat;
	unsigned int tmp;
	char tmp1[9];
	do{
                //set file ptr to the first fat entry
		fseek(fptr,dir_on,SEEK_SET);
		fread(&(tmp),4,1,fptr);
		struct directory dir;

		//go to cluster and read out the files in it
		fseek(fptr,FirstSectorofCluster((dir_on-start_dir_fat)/4+2),SEEK_SET);
		int length = 8;
		//so that it will check the sector fully
		for(int i = 0; i < (info->BPB_BytsPerSec/32); i++){
			fread(&dir,32,1,fptr);
			for(int i = 0; i < 8; i++){
				if(dir.name[i] == ' '){
					length = i;
					break; 
				}              
			}
			if(dir.attribute == ATTR_DIRECTORY){
				memcpy(tmp1,dir.name,length);
				for(int i = 0; i < length; i++){
					tmp1[i] = tolower(tmp1[i]);
				}
				tmp1[length] = '\0';
			}if(strcmp(tmp1,directory)==0){
				unsigned int super_tmp = dir.first_cluster_high >> 16;
                                //printf("low %x high %x\n",dir.first_cluster_low,dir.first_cluster_high);
				super_tmp += dir.first_cluster_low;
				if(super_tmp == 0){super_tmp=2;}
				super_tmp = ThisFATSecNum(super_tmp,info) + ThisFATEntOffset(super_tmp,info);
				return super_tmp;
			}
		}
		if(tmp < END_FAT){
			dir_on = start_dir_fat + (tmp*4-8);
		}
	}while(tmp < END_FAT);
	printf("didnt find the file\n");
	return current_dir_fat;

}


void size(unsigned int current_dir, struct boot_sector_struct* info, FILE* fptr, char* directory){
	//same as cd except upon file match use Dir_Size
	uint32_t dir_on = current_dir_fat;
	unsigned int tmp;
	char tmp1[13];
	do{
		fseek(fptr,dir_on,SEEK_SET);
		fread(&(tmp),4,1,fptr);
		struct directory dir;
		//go to cluster and read out the files in it
		fseek(fptr,FirstSectorofCluster((dir_on-start_dir_fat)/4+2),SEEK_SET);
		int length = 8;
		//so that it will check the sector fully
		for(int i = 0; i < (info->BPB_BytsPerSec/32); i++){
			fread(&dir,32,1,fptr);
			for(int i = 0; i < 8; i++){
				if(dir.name[i] == ' '){
					length = i;
					break; 
				}              
			}
			if(dir.attribute != ATTR_DIRECTORY){
				memcpy(tmp1,dir.name,length);
				for(int i = 0; i < length; i++){
					tmp1[i] = tolower(tmp1[i]);
				}
				tmp1[length] = '\0';
				//printf("%s\n",tmp1);
			}if(strcmp(tmp1,directory)==0){
				unsigned int super_tmp = dir.size;
				printf("file size %d\n",super_tmp);
				return;
			}
		}
		if(tmp < END_FAT){
			dir_on = start_dir_fat + (tmp*4-8);
		}
	}while(tmp < END_FAT);
	printf("didnt find the file\n");
}

uint32_t byteOffsetofCluster(struct boot_sector_struct *bs, uint32_t cluster_num){
	return FirstSectorofCluster(cluster_num)* bs->BPB_BytsPerSec;
}

uint32_t getFatEntry(struct boot_sector_struct * bs, uint32_t cluster_num) {
    
    }


int getFileSizeInClusters(struct boot_sector_struct * bs, uint32_t firstCluster) {
    int size = 1;
    firstCluster = (int) getFatEntry(bs, firstCluster);

    while((firstCluster = (firstCluster & TRUE_FAT_END)) < END_FAT) {
       
        size++;
        firstCluster = getFatEntry(bs, firstCluster);
    }
    return size;
}

void readfile(char* filename, struct openfiles **of, struct boot_sector_struct *bs, struct boot_sector_struct* info, int mode, int *filecount, FILE* fptr, int *filesize, unsigned int cluster_num, int OFFSET, int SIZE){
	struct directory dir;

	//currentCluster = first_num_cluster;	
	uint32_t readClusterOffset = (OFFSET/bs-> BPB_BytsPerSec);
	uint32_t posToCluster = OFFSET % bs -> BPB_BytsPerSec;
	//uint32_t fileSize_Clusters = getFileSizeInClusters(bs, firstCluster);
	//uint32_t remainingClusters = fileSizeInClusters - readClusterOffset;


	printf("fread> pos: %d, readClusterOffset: %d, posRelativeToCluster: %d\n",OFFSET, readClusterOffset, posToCluster);
        int x;
	for(x = 0; x < readClusterOffset; x++){
           //     currentCluster = getFatEntry(bs, currentCluster);
	}
	//to hold file name and extention
	char cluster_hold[1000];     
//currently assuming all in the same fat
		/*
		do{
                        //find the cluster value and store into char array
			fseek(fptr,dir_on,SEEK_SET);
                        fread(&(tmp),4,1,fptr);
                        printf("Current Directory: %x\n",dir_on);
                        //currently assuming all in the same fat
                            //ls call(cluster num)
			    cluster_num = dir_on-start_dir_fat/4+2
			//    fread(&(tmp), 4, 1, fptr);	
                            if(tmp < 0x0FFFFFF8){
                                dir_on = start_dir_fat + (tmp*4-8);
                            }
                        }while(tmp < 0x0FFFFFF8);
		*/
//read the SIZE bytes starting at OFFSET 
//Edge cases: OFFSET > sizeof(FILENAME), print error
		uint32_t dataReadSoFar = SIZE;
		uint32_t dataLeft = SIZE;
		uint32_t fileReadOffset;
		uint32_t startReadPos;

	uint32_t dir_on = current_dir_fat;
	uint32_t tmp;
	unsigned int super_tmp;

	//char cluster_hold* = malloc((*filecount+1)*sizeof(struct openfiles));

	//try and find file

	//check if the file has read permissions (R, RW, WR)
	if ((mode != 2) && (mode != 3)){}
	else{
	
	//check if the file has been opened(and the file is not a directory)
	
	if(checkForFile(fptr,((dir_on-start_dir_fat)/4+2),info, filename, &super_tmp)==1){
			//for loop to check if its in of open =1
			for(int i = 0; i < *filecount; i++){
				if(super_tmp == (*of)[i].first_cluster_num){
					printf("File is already open\n");
					return;
				}

			}
	}
	//read the file
	//find the cluster start pos
////////////////////////////uint32_t startReadpos = byteOffsetOfCluster(bs, first_cluster_num) 

	//cluster_first_num?

	//read the SIZE bytes starting at OFFSET 
	
			//super_tmp = *of.first_cluster_num;

				/*
				//create new entry
				struct openfiles *oftmp = malloc((*filecount+1)*sizeof(struct openfiles));
				for(i = 0; i < *filecount; i++){
					oftmp[i].mode = (*of)[i].mode;
					oftmp[i].first_cluster_num = (*of)[i].first_cluster_num;
				}
				oftmp[*filecount].mode = mode;
				oftmp[*filecount].first_cluster_num = super_tmp;
				free(*of);
				*of = oftmp;
				*filecount = *filecount + 1;
				*/
		}

//Edge cases: 
if(OFFSET >= *filesize){
	printf ("Error: Edge Case\n");
}//Edge cases: OFFSET > sizeof(FILENAME), print erroruint32_t index;

if(SIZE > *filesize){
	printf ("Error: Not enough space in file\n");
} //OFFSET > sizeof(FILENAME), print error 
//SIZE > sizeof(FILENAME), print entire file
//OFFSET + SIZE > sizeof(FILENAME) -prints sizeof(FILENAME) -OFFSET bytes

//off_bytes/bytes_per_sec = cluster offset
//offset % bytes_per_sec = byte offset within cluster
}


int main(int argc,char *argv[]){
        char* input_raw = (char*)malloc(MAX_INPUT_SIZE*sizeof(char));
        char* input = (char*)malloc(MAX_INPUT_SIZE*sizeof(char));
        FILE *fptr;
        char* token;
        char* commands[5];
        int i = 0;
	struct openfiles * openfs = NULL;
	int mode = 0;
	int filecount = 0;

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
        get_info(&info,fptr);

        //will grab newline as well be aware
        fgets(input_raw,MAX_INPUT_SIZE,stdin);
        sscanf(input_raw,"%s",input);
        input_raw[strlen(input_raw)-1] = '\0';

        //main shell like loop to ask user what to do
        while(strcmp(input,"exit")!=0){
                for(int i=0; i < 5; i++){
                        commands[i] = 0;
                }
                i = 0;
                token = strtok(input_raw, " ");
                while(token!=NULL){
                        commands[i++] = token;
                        //printf("TOKEN %s\n", token);
                        token = strtok(NULL, " ");
                }
                for(int i = 0; i < 5; i++){
                  //printf("ARRAY:%s\n",commands[i]);
                  }
                if(strcmp(commands[0],"ls")==0){
                        uint32_t tmp;
                        uint32_t dir_on = current_dir_fat;
                        do{
                        fseek(fptr,dir_on,SEEK_SET);
                        fread(&(tmp),4,1,fptr);
                            ls(fptr,((dir_on-start_dir_fat)/4+2),&info);
                            if(tmp < END_FAT){
                                dir_on = start_dir_fat + (tmp*4-8);
                            }
                        //printf("dir on %x\n",dir_on);
                        }while(tmp < END_FAT);
                }
                else if(strcmp(commands[0],"cd")==0){
                        if(commands[1]!=NULL){
                             current_dir_fat = cd(current_dir_fat, &info, fptr, commands[1]);
                             //printf("current_dir_fat %x\n",current_dir_fat);
                        }else{
                             printf("Error: cd needs a directory name\n");
                        }
                }
                else if(strcmp(commands[0],"info")==0){
                        print_info(&info,fptr);        
                }
                else if(strcmp(commands[0],"size")==0){
                        if(commands[1] != NULL){
			    size(current_dir_fat, &info, fptr, commands[1]);
			}else{
				printf("Error: cd needs a directory name\n");
			}
		}
                else if(strcmp(commands[0],"creat")==0){
                        //printf("creat!!!\n");
                        if(commands[1]==NULL){
                              printf("Must enter a filename\n");
                        }else{
                              unsigned int empty = find_empty_cluster(current_dir_fat,&info,fptr, commands[1]);
                              //printf("empty: %x",empty);
                              create(commands[1],&info,fptr,empty);
                                               
                        }
                }
                else if(strcmp(commands[0],"open")==0){
                        //printf("open!!!\n");
			if(commands[1]==NULL){
				printf("Must enter a filename\n");
			}
			else{
				if(commands[2]==NULL){
					printf("Must enter a mode\n");
				}
				else if(commands[3]==NULL){
					//printf("OPEN func\n");
					if(strcmp(commands[2],"w")==0){
						mode = 1;
					}
					else if(strcmp(commands[2], "r")==0){
						mode = 2;
					}
					else if(strcmp(commands[2], "rw")==0){
						mode = 3;
					}
					else if(strcmp(commands[2], "wr")==0){
						mode = 3;
					}
					else{
						mode = 0;
					}
					//printf("Mode: %d\n", mode);
					openfile(commands[1], mode, &openfs, &info, fptr, &filecount);
					/*printf("FILE COUNT: %d", filecount);
					int i = 0;
					for(i=0; i < filecount; i++){
						printf("mode %d, cluster %d\n", openfs[i].mode, openfs[i].first_cluster_num);
					}*/
				}
			}
                }
                else if(strcmp(commands[0],"close")==0){
                        //printf("close!!!\n");
			if(commands[1]==NULL){
				printf("Must enter a filename\n");
			}
			else{
				uint32_t dir_on = current_dir_fat;
				unsigned int super_tmp;
				uint32_t tmp;
				int open = 0;
				do{
					fseek(fptr,dir_on,SEEK_SET);
					fread(&(tmp),4,1,fptr);
					if(checkForFile(fptr,((dir_on-start_dir_fat)/4+2),&info, commands[1], &super_tmp)!=1){
						printf("ERROR: file is not in current directory\n");
					}
					else{
						if(filecount == 0){
							printf("Error: No open files\n");
						}
						else{
						for(i=0; i < filecount; i++){
							if(super_tmp == openfs[i].first_cluster_num){
								open = 1;
							}
						}
						if(open == 0){
							printf("Error: File is not open\n");
						}
						else{
							closefile(super_tmp, &openfs, &filecount);
							break;
						}
					}

					}
				
					if(tmp < 0X0FFFFFF8){
						dir_on = start_dir_fat + (tmp*4-8);
					}
				}while(tmp < 0X0FFFFFF8);
				/*int i = 0;
                                for(i=0; i < filecount; i++){
                               		printf("mode %d, cluster %d\n", openfs[i].mode, openfs[i].first_cluster_num);
                               	}*/

			}
		}
		else if(strcmp(commands[0],"mkdir")==0){
                        if(commands[1]==NULL){
                                printf("Must enter a filename\n");
                        }else{
			unsigned int empty = find_empty_cluster(current_dir_fat,&info,fptr, commands[1]);
		          	//printf("empty: %x",empty);
		          	mkdir(commands[1],&info,fptr,empty);
		          	//printf("MKDIR!!!\n");
                        }
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
		input_raw[strlen(input_raw)-1] = '\0';
	}
	//free memory here
	free(input);
	free(input_raw);
	free(openfs);

	//close file
	fclose(fptr);


}
