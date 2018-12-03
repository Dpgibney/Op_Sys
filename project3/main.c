#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#define MAX_INPUT_SIZE 200

struct BPB_2_0{
    unsigned short bytes_per_logic;
    char logial_sectors_per_cluster;
    unsigned short reserved_logical_sectors;
    char num_file_allocation_tables;
    unsigned short Max_num_fat12_16_root_entries;
    unsigned short total_logical_sectors;
    char media_descriptor; 
    unsigned short logic_sectors_fat12_16;
};

struct BPB_3_31{
   struct BPB_2_0 BPB_2;
   unsigned short Physical_sectors_per_track;
   unsigned short number_of_heads;
   unsigned int count_hidden_sectors;
   unsigned int total_logical_sectors;
};
struct boot_sector_struct{
   struct BPB_3_31 BPB_3;
   char drive_number;
   char reserved;
   char extended_boot_signature;
   unsigned int volume_id;
   char* volume_partition_label;
   char* file_system_type;
};

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
info.volume_partition_label = (char*)malloc(12);
info.file_system_type = (char*)malloc(9);


//check that file could open
//currently using binary mode
fptr = fopen(argv[1],"rb+");
if(fptr==NULL){
   printf("Opening file failed\n");
   return -1;
}

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

//close file
close(fptr);

}
