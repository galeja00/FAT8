
#ifndef DRV_H
#define DRV_H

#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <stdint.h>
#include <sys/mman.h>
#include <string.h>

#define SIZE_OF_BLOCK 512

// drive structure
typedef struct {
    char* path;
    int descriptor;
    unsigned int num_blocks;
} drive;

// create new drive(file) size * SIZE_OF_BLOCK on file(path to it)
void drv_manufacture(char* file, size_t size);
//  open drive for work (stat, read, write) and create new structure drive, after end of work with drive you need to use drv_close
drive* drv_open(char* file);
// return size of drive in Bytes
size_t drv_stat(drive* drive);
// return pointer to array his size is SIZE_OF_BLOCK(512 B), on position blockId
void drv_read(drive* drive, unsigned int blockId, uint8_t* buff);
// write to disk one block of data on position blockId
void drv_write(drive* drive, unsigned int blockId, void* data);
// close work with drive
void drv_close(drive* drive);



#endif 