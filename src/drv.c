#include "drv.h"

#define SIZE_OF_BLOCK 512



void drv_manufacture(char* file, size_t size) {
    size_t file_size = size * SIZE_OF_BLOCK;
    int disk = open(file, O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    if (disk < 0) {
        perror("Error: unable to open file");
        exit(1);
    }

    if (ftruncate(disk, file_size) < 0) {
        perror("Error: in ftruncate size");
        close(disk);
        exit(2);
    }

    close(disk);
}

drive* drv_open(char* file) {
    int fd = open(file, O_RDWR);
    if (fd < 0) {
        perror("Error: Disk isn't inicializet.\n");
        exit(1);
    }

    drive* d = malloc(sizeof(drive));
    d->path = file;
    d->descriptor = fd;
    struct stat st;
    fstat(d->descriptor, &st);
    d->num_blocks = st.st_size / SIZE_OF_BLOCK;
    return d;
}

size_t drv_stat(drive* d) {
    return d->num_blocks * SIZE_OF_BLOCK;
}

void drv_read(drive* drive, unsigned int blockId, uint8_t* buff) {
    if (blockId >= drive->num_blocks) {
        perror("Error: blockId is out of range.\n");
        perror("function drv_read\n");
        exit(1);
    }

    long pagesize = sysconf(_SC_PAGE_SIZE);
    size_t page = (blockId * SIZE_OF_BLOCK) / pagesize;
    size_t offset =  page * pagesize;

    uint8_t* data = mmap(NULL, pagesize, PROT_READ, MAP_PRIVATE, drive->descriptor, offset);
    if (data == MAP_FAILED) {
        perror("Error: unable to map file to memory.\n");
        exit(1);
    } 

    size_t index = blockId * SIZE_OF_BLOCK - offset;
    memcpy(buff, data + index, SIZE_OF_BLOCK);

    munmap(data, pagesize);;
}

void drv_write(drive* drive, unsigned int blockId, void* data) {
    if (blockId >= drive->num_blocks) {
        perror("Error: blockId is out of range.\n");
        perror("function drv_write\n");
        exit(1);
    }

    long pagesize = sysconf(_SC_PAGE_SIZE);
    size_t page = (blockId * SIZE_OF_BLOCK) / pagesize;
    size_t offset =  page * pagesize;

    void* maped_d = mmap(NULL, pagesize, PROT_WRITE, MAP_SHARED, drive->descriptor, offset);
    if (maped_d == MAP_FAILED) {
        perror("Error: unable to map file to memory.\n");
        exit(1);
    } 

    size_t index = blockId * SIZE_OF_BLOCK - offset;
    memcpy(maped_d + index, data, SIZE_OF_BLOCK);

    munmap(maped_d, pagesize);
}

void drv_close(drive* drive) {
    close(drive->descriptor);
    free(drive);
}