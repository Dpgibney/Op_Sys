#include<stdio.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<stdint.h>
#define MAX_INPUT_SIZE 200

struct BPB_2_0{
    uint16_t bytes_per_logic;
    uint8_t logical_sectors_per_cluster;
    uint16_t reserved_logical_sectors;
    uint8_t num_file_allocation_tables;
    uint16_t max_num_fat12_16_root_entries;
    uint16_t total_logical_sectors;
    uint8_t media_descriptor; 
    uint16_t logic_sectors_fat12_16;
};

struct BPB_3_31{
   struct BPB_2_0 BPB_2;
   uint16_t physical_sectors_per_track;
   uint16_t number_of_heads;
   uint32_t count_hidden_sectors;
   uint32_t total_logical_sectors;
};
struct boot_sector_struct{
   struct BPB_3_31 BPB_3;
   uint32_t logic_sectors_per_file_table;
   uint16_t drive_description;
   uint16_t version;
   uint32_t cluster_num_root_dir_start;
   uint16_t first_logic_sector;
   char* filename_of_weird_thing;
   uint8_t cf_24;
   uint8_t cf_25;
   uint8_t cf_26;
   uint32_t cf_27;
   char* cf_2b;
   char* cf_36;
};

void get_info(struct boot_sector_struct info, FILE *fptr){
     if(fseek(fptr,11,SEEK_SET)==0){
        fread(&(info.BPB_3.BPB_2.bytes_per_logic),2,1,fptr);
        fread(&(info.BPB_3.BPB_2.logical_sectors_per_cluster),1,1,fptr);
        fread(&(info.BPB_3.BPB_2.reserved_logical_sectors),2,1,fptr);
        fread(&(info.BPB_3.BPB_2.num_file_allocation_tables),1,1,fptr);
        fread(&(info.BPB_3.BPB_2.max_num_fat12_16_root_entries),2,1,fptr);
        fread(&(info.BPB_3.BPB_2.total_logical_sectors),2,1,fptr);
        fread(&(info.BPB_3.BPB_2.media_descriptor),1,1,fptr);
        fread(&(info.BPB_3.BPB_2.logic_sectors_fat12_16),2,1,fptr);
        fseek(fptr,24,SEEK_SET);
        fread(&(info.BPB_3.physical_sectors_per_track),2,1,fptr);
        fread(&(info.BPB_3.number_of_heads),2,1,fptr);
        fread(&(info.BPB_3.count_hidden_sectors),4,1,fptr);
        fread(&(info.BPB_3.total_logical_sectors),4,1,fptr);
        fseek(fptr,36,SEEK_SET);
        fread(&(info.logic_sectors_per_file_table),4,1,fptr);
        fread(&(info.drive_description),2,1,fptr);
        fread(&(info.version),2,1,fptr);
        fread(&(info.cluster_num_root_dir_start),4,1,fptr);
        fread(&(info.first_logic_sector),2,1,fptr);
        fread((info.filename_of_weird_thing),12,1,fptr);
        fread(&(info.cf_24),1,1,fptr);
        fread(&(info.cf_25),1,1,fptr);
        fread(&(info.cf_26),1,1,fptr);
        fread(&(info.cf_27),4,1,fptr);
        fread((info.cf_2b),11,1,fptr);
        fread(&(info.BPB_3.physical_sectors_per_track),2,1,fptr);
        fread((info.cf_36),8,1,fptr);
        printf("BPB 2.0\n");
        printf("bytes per logic: %u\n",info.BPB_3.BPB_2.bytes_per_logic);
        printf("logical sectors per cluster: %u\n",info.BPB_3.BPB_2.logical_sectors_per_cluster);
        printf("reserved_logical_sectors: %u\n",info.BPB_3.BPB_2.reserved_logical_sectors);
        printf("number of file allocation tables: %u\n",info.BPB_3.BPB_2.num_file_allocation_tables);
        printf("max number of fat12/16 root entries: %u\n",info.BPB_3.BPB_2.max_num_fat12_16_root_entries);
        printf("total logical sectors: %u\n",info.BPB_3.BPB_2.total_logical_sectors);
        printf("media descriptors: %x\n",info.BPB_3.BPB_2.media_descriptor);
        printf("logical sectors fat12/16: %u\n",info.BPB_3.BPB_2.logic_sectors_fat12_16);
        printf("BPB 3.0\n");
        printf("physical sectors per track: %u\n",info.BPB_3.physical_sectors_per_track);
        printf("number of heads: %u\n",info.BPB_3.number_of_heads);
        printf("count hidden sectors: %u\n",info.BPB_3.count_hidden_sectors);
        printf("total logical sectors: %u\n",info.BPB_3.total_logical_sectors);
        printf("FAT32 boot\n");
        printf("logic sectors per file table: %u\n",info.logic_sectors_per_file_table);
        printf("drive description: %u\n",info.drive_description);
        printf("version: %u\n",info.version);
        printf("cluster number root dir start: %u\n",info.cluster_num_root_dir_start);
        printf("first logic sector: %u\n",info.first_logic_sector);
        printf("reserved: %s\n",info.filename_of_weird_thing);
        printf("cf_24: %u\n",info.cf_24);
        printf("cf_25: %u\n",info.cf_25);
        printf("cf_26: %u\n",info.cf_26);
        printf("cf_27: %u\n",info.cf_27);
        printf("cf_2b: %s\n",info.cf_2b);
        printf("cf_36: %s\n",info.cf_36);
        
     }
}
int main(int argc,char *argv[]){
char* input_raw = (char*)malloc(MAX_INPUT_SIZE*sizeof(char));
char* input = (char*)malloc(MAX_INPUT_SIZE*sizeof(char));
FILE *fptr;
printf("%s\n",argv[1]);
printf("%d\n",argc);

//check that a file was passed as an argument
if(argc!=2){
   printf("Improper usage: must pass image file as argument\n");
   return -1;
}

//creating struct to hold info about filesystem one extra space for endline;
struct boot_sector_struct info;
info.filename_of_weird_thing = (char*)malloc(12);
info.cf_2b = (char*)malloc(11);
info.cf_36 = (char*)malloc(8);


//check that file could open
//currently using binary mode
fptr = fopen(argv[1],"rb+");
if(fptr==NULL){
   printf("Opening file failed\n");
   return -1;
}
get_info(info,fptr);
//will grab newline as well be aware
fgets(input_raw,MAX_INPUT_SIZE,stdin);
sscanf(input_raw,"%s",input);

//main shell like loop to ask user what to do
while(strcmp(input,"exit")!=0){
printf("%s\n",input_raw);


//get new input from user
fgets(input_raw,MAX_INPUT_SIZE,stdin);
sscanf(input_raw,"%s",input);
}
//free memory here
free(input);
free(input_raw);

//TODO free malloc'd memory in struct
free(info.filename_of_weird_thing);
free(info.cf_2b);
free(info.cf_36);

//close file
fclose(fptr);

}
