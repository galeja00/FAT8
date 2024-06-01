#include <stdio.h>
#include <inttypes.h>

#ifndef FAT8_H
#define FAT8_H

#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20

// symptoms in FAT table for FREE cluster or EOC end of file
#define EOC 0xFF // 255 
#define FREE_CLUSTER 0x00 // 0
// size in sectors
#define SIZE_BOOT 1     //boot sector
#define SIZE_FAT 1      //FAT
#define SIZE_RD 16      //root directory
//
#define NUM_DATA_CLUSTERS 254   //max data clusters in file system
#define NUM_FATS 1   //number of fat tables in file system
//offset on drive in segments 
#define ID_BOOT 0    //boot dir
#define ID_FAT 1     //FAT
#define ID_RD 2      //root directory 
#define FIRST_DATA_SEC  18 //first sector 
//
#define SIZE_DIR_ENTRY  32 //aligning for dir_entrys in B
#define SIZE_FILENAME   11 //
//FAT indexis
#define FAT_FIRST  1   //first usable
#define FAT_LAST  254  //last usable

// including block device
#include "drv.h"

// structs for file system
typedef struct {
    drive* drive;
    // data v drivu             //offset(B)
    uint16_t BPB_BytsPerSec;    //11 -size of sector in B
    uint8_t  BPB_SecPerClus;    //13 -number of sectors in cluster    
    uint8_t  BPB_NumFATs;       //14 -number og FAT tables
    uint32_t BPB_TotSec32;      //15 -total number of sectors          
    uint8_t  BPB_FATSz8;        //19 -number of sectors occupated sectors by FAT 
    uint8_t  BS_FilSysType[8];  //48 -type of file system "FAT8    "

    // data odvezona
    uint32_t ClusterSize;
	uint32_t FirstDataSector;
	uint32_t DataSectors;
	uint32_t CountOfClusters;
} fat8;

typedef struct {                //offset(B)
    char    fileName[11];       //0
    uint8_t  attr;              //11 -attributs of file
    uint8_t  frsCluster;        //12 -first cluster
    uint32_t fileSize;          //13
    
    uint32_t fstClusterId;
} dir_entry;

// Try without dir entry
typedef struct {
    fat8 *fs;                   //file system
    dir_entry* de;              //dir entry of file
    uint32_t fileOffset;        //offset in file
    uint32_t fileSize;          //size in B
    uint8_t curentCluster;      //actual cluster whre fileoffset is
    uint32_t bufOffset;         //position in buffer
    uint32_t bufSize;           //num correct bytes in buffer
    uint8_t buf[];              //bufer for read/write
} file;

// format drive to default settings
// @drive - block devic (size of block 512B)
void fs_format(drive* drive);

// open drive type FAT8
fat8* fs_open(drive* drive);

//close work with file system
void fs_close(fat8* fs);

// open file with name fileName if existing, if not it will create new
file* file_open(fat8* fs, char* fileName); 

// increases of decreases to your input size
// @size - value of size
void file_truncate(file* fd, size_t size); 

// move fileOffset to position (counmt from start of file)
// @position - number from start of file
void file_seek(file* fd, int position);

// return file size in Bytes
size_t file_stat(file* fd);

// retunring fileOffset position
int file_tell(file* fd);

// read data from file from fileOffset position to size
// @result - data readed in functtion
void file_read(file* fd, int size, uint8_t* result);

// write data of size size if file si smaller the position in file + offset it will increases file size
void file_write(file* fd, uint8_t* data, int size);

// close work with file 
void file_close(file* fd);

// return array of all names in root directory size of array will be saved in size
char** file_readdir(fat8* fs, int* size);

// delete file when fileName == name
void file_delete(fat8* fs, char* name);


// for testing only
uint8_t* fat_get(fat8* fs);
void fat_close(uint8_t* fat);

#endif 