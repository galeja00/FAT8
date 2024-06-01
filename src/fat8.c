#include <inttypes.h>
#include <stdlib.h>
#include "fat8.h"

//----------Bytes conv funcs-----------------

// count blocks/sectors for data cluster
uint8_t num_blocks_in_cluster(size_t size) {
    uint8_t result = 1;
    int headers_size = SIZE_BOOT + SIZE_FAT + SIZE_RD;
    size_t size_cluster_in_blocks = ((size - headers_size * SIZE_OF_BLOCK) / NUM_DATA_CLUSTERS) / SIZE_OF_BLOCK;
    while (result * 2 <= size_cluster_in_blocks) {
        result *= 2;
    }
    return result;
}

// convert 16b number to array of B
uint8_t* uint16_to_bytes(uint16_t val) {
    uint8_t* res = calloc(2, sizeof(uint8_t)); 
    res[0]= val & 0xFF;
    res[1] = (val >> 8) & 0xFF;
    return res;
}

// convert b number to array of B
uint8_t* uint32_to_bytes(uint32_t val) {
    uint8_t* res = calloc(4, sizeof(uint8_t)); 
    res[0] = val & 0xFF;
    res[1] = (val >> 8) & 0xFF;
    res[2] = (val >> 16) & 0xFF;
    res[3] = (val >> 24) & 0xFF;
    return res;
}

// convert array of B (size of 2) to 16 b number
uint16_t chars_to_unit16(uint8_t* arr) {
    uint16_t res;
    res = (arr[1] << 8) | arr[0];
    return res;
}

// convert array of B (size of 4) to 32 b number
uint32_t chars_to_unit32(uint8_t* arr) {
    uint32_t res;
    res = arr[3];
    res = (res << 8) | arr[2];
    res = (res << 8) | arr[1];
    res = (res << 8) | arr[0];
    return res;
}

//----------------------------------------------

// compare strings, if string are same return 0 else 1
int cmp_strings(char* name1, char* name2, int size) {
    for (int i = 0; i < size; i++) {
        if (name1[i] != name2[i]) {
            return 1;
        }
    }
    return 0;
}

// alligning inpoted fileName to FAT dir entry format
void align_fileName(char* alignName, char* fileName) {
    char fullName[11];
    size_t len = strlen(fileName);
    int i = 0;
    char name[8];
    char suffix[4];
    while (i < len && fileName[i] != '.' && i < 8) {
        name[i] = fileName[i];
        i++;
    } 
    while (i < 8) {
        name[i] = ' ';
        i++;
    }
    suffix[0] = fileName[len - 3];
    suffix[1] = fileName[len - 2];
    suffix[2] = fileName[len - 1];
    
    memcpy(fullName, name, 8);
    memcpy(fullName + 8, suffix, 3);  
    memcpy(alignName, fullName, 11);
}


void fs_format(drive* drive) {
    size_t size = drv_stat(drive);
    if (size * SIZE_OF_BLOCK < 264) {
        perror("Error: This drive - is to small for FAT8");
        exit(0);
    }
    uint8_t BPB_SecPerClus = num_blocks_in_cluster(size);
    uint8_t BPB_NumFATs = NUM_FATS;
    uint16_t BPB_BytsPerSec = SIZE_OF_BLOCK; 
    uint32_t BPB_TotSec32 = BPB_SecPerClus * NUM_DATA_CLUSTERS + SIZE_BOOT + SIZE_FAT + SIZE_RD;
    uint8_t BPB_FATSz8 = 1;
    char BS_FilSysType[8] = "FAT8    ";
    uint8_t* boot_sec = malloc(SIZE_OF_BLOCK * sizeof(uint8_t));
    uint8_t* coded_BPS = uint16_to_bytes(BPB_BytsPerSec);
    boot_sec[11] = coded_BPS[0];
    boot_sec[12] = coded_BPS[1];
    free(coded_BPS);
    boot_sec[13] = BPB_SecPerClus;
    boot_sec[14] = BPB_NumFATs;
    uint8_t* coded_TS32 = uint32_to_bytes(BPB_TotSec32);
    for (int i = 0; i < 4; i++) {
        boot_sec[15 + i] = coded_TS32[i];
    }
    free(coded_TS32);
    boot_sec[19] = BPB_FATSz8; 
    for (int i = 0; i < 8; i++) {
        boot_sec[48 + i] = BS_FilSysType[i]; 
    }
    drv_write(drive, 0, boot_sec);
    uint8_t* data = calloc(512, sizeof(uint8_t));
    for (int i = 0; i < SIZE_RD; i++) {
        drv_write(drive, ID_RD + i, data);
    }
    free(data);
    free(boot_sec);
}

fat8* fs_open(drive* drive) {
    fat8* fs = malloc(sizeof(fat8));
    uint8_t* boot_sec = malloc(SIZE_OF_BLOCK * sizeof(uint8_t));
    drv_read(drive, 0, boot_sec); 
    uint8_t bytperchar[2];
    bytperchar[0] = boot_sec[11];
    bytperchar[1] = boot_sec[12]; 
    fs->drive = drive;
    fs->BPB_BytsPerSec = chars_to_unit16(bytperchar);
    fs->BPB_SecPerClus = boot_sec[13];
    fs->BPB_NumFATs = boot_sec[14];
    fs->BPB_TotSec32 =  chars_to_unit32(boot_sec + 15);
    fs->BPB_FATSz8 = boot_sec[19];
    for (int i = 0; i < 8; i++) {
        fs->BS_FilSysType[i]= boot_sec[48 + i];
    }
    fs->ClusterSize = fs->BPB_SecPerClus * fs->BPB_BytsPerSec;
    fs->FirstDataSector = FIRST_DATA_SEC;
    fs->DataSectors = fs->BPB_TotSec32 - FIRST_DATA_SEC;
    fs->CountOfClusters = fs->DataSectors / fs->DataSectors;
    free(boot_sec);
    return fs;
}

void fs_close(fat8* fs) {
    free(fs);
}

//---------------FAT FUNC---------------------

// return FAT table from file system
uint8_t* fat_get(fat8* fs) {
    uint8_t* buff = malloc(SIZE_OF_BLOCK * sizeof(uint8_t));
    drv_read(fs->drive, ID_FAT, buff);
    return buff;
}

// it will wrtite inoputed FAT to file system
void fat_write(fat8* fs, uint8_t* fat) {
    drv_write(fs->drive, ID_FAT, fat);
}

// dealocate all memory for FAT table
void fat_close(uint8_t* fat) {
    free(fat);
}

// it will find index in FAT table where is free cluster
int find_first_free_cluster(uint8_t* fat) {
    for (int i = FAT_FIRST; i < FAT_LAST + 1; i++) {
        if (fat[i] == FREE_CLUSTER) {
            return i;
        }
    }
    perror("Error: Disk is full");
    exit(1);
}

// FAT index 0 and 255 are unused becouse spacial values (FREE, EOC)
// it will find first sector from FAT index 
int sector_from_clsIndex(fat8* fs, uint8_t index) {
    return ((index - 1) * fs->BPB_SecPerClus) + fs->FirstDataSector;
}

// write cluster to FS drive
void write_cluster(fat8* fs, uint8_t* cluster, uint8_t clusterIndex) {
    int frst_sec = sector_from_clsIndex(fs, clusterIndex);
    uint8_t* buff = malloc(fs->BPB_BytsPerSec * sizeof(uint8_t));
    for(int i = 0; i < fs->BPB_SecPerClus; i++) {
        memcpy(buff, cluster + i * fs->BPB_BytsPerSec, fs->BPB_BytsPerSec);
        drv_write(fs->drive, frst_sec + i, buff);
    }
}

// zeroad all B in cluster and write to drive
void clean_cluster(fat8* fs, uint8_t clusterIndex) {
    uint8_t* zeros = calloc(fs->ClusterSize, sizeof(uint8_t));
    write_cluster(fs, zeros, clusterIndex);
}

// load actual cluster to file buffer
void load_curtenCluster_toBuff(file* f) {
    int frst_sec = sector_from_clsIndex(f->fs, f->curentCluster);
    uint8_t* data = malloc(SIZE_OF_BLOCK * sizeof(uint8_t));
    for(int i = 0; i < f->bufSize / f->fs->BPB_SecPerClus; i++) {
        drv_read(f->fs->drive, frst_sec + i, data);
        memcpy(f->buf + i * f->fs->BPB_BytsPerSec, data, f->fs->BPB_BytsPerSec);
    }
    free(data);
}


//--------------DIR ENTRY FUNC-------------------


// data must be on start of dir entry
char* get_dir_entry_name(char* data, char* buff) {
    if (buff == NULL) {
        perror("Error: fail to alocate memory");
        exit(1); 
    }
    int len = strlen(buff);
    buff[len] = 0x00;
    memcpy(buff, data, SIZE_FILENAME);
    return buff;
}


uint8_t count_dir_entrys(fat8* fs) {
    uint8_t count = 0;
    uint8_t* data = malloc(SIZE_OF_BLOCK * sizeof(uint8_t));
    for (int segment = 0; segment < SIZE_RD; segment++) {
        drv_read(fs->drive, ID_RD + segment, data);
        for (int entry = 0; entry < fs->BPB_BytsPerSec / SIZE_DIR_ENTRY; entry++) {
            char* name = malloc((SIZE_FILENAME + 1) * sizeof(char));
            get_dir_entry_name(data + entry * SIZE_DIR_ENTRY, name);
            if (name[0] != 0x00) { //name[0] == 0x00 -> is deleted dir_entry or empty
                count++;
            }
        }
    }
    free(data);
    return count;
}

dir_entry* get_dir_entry(char* data)  {
    dir_entry* entry = malloc(sizeof(dir_entry));
    if(entry == NULL) {
        perror("Error: fail to alocate memory");
        exit(1);
    }
    memcpy(entry->fileName, data, 11);
    entry->attr = data[11];
    entry->frsCluster = data[12];
    entry->fileSize = chars_to_unit32(data + 13);
    return entry; 
}

int pos_dir_entry(fat8* fs, char* fileName) {
    uint8_t* data = malloc(SIZE_OF_BLOCK * sizeof(uint8_t));
    for (int segment = 0; segment < SIZE_RD; segment++) {
        drv_read(fs->drive, ID_RD + segment, data);
        char* name = malloc((SIZE_FILENAME + 1) * sizeof(char));
        for (int entry = 0; entry < fs->BPB_BytsPerSec / SIZE_DIR_ENTRY; entry++){
            get_dir_entry_name(data + entry * SIZE_DIR_ENTRY, name);
            //printf("%s == %s\n", name, fileName);
            if (cmp_strings(name, fileName, SIZE_FILENAME) == 0) {
                free(data);
                return segment * (fs->BPB_BytsPerSec / SIZE_DIR_ENTRY) + entry;
            }
        }
        
    }
    free(data);
    return -1;
}

int freepos_dir_entry(fat8* fs) {
    uint8_t* data = malloc(SIZE_OF_BLOCK * sizeof(uint8_t));
    for (int segment = 0; segment < SIZE_RD; segment++) {
        drv_read(fs->drive, ID_RD + segment, data);
        for (int entry = 0; entry < fs->BPB_BytsPerSec / SIZE_DIR_ENTRY; entry++) {
            if (data[entry * SIZE_DIR_ENTRY] == 0x00) {
                free(data);
                return segment * (fs->BPB_BytsPerSec / SIZE_DIR_ENTRY) + entry;
            }
        }
        
    }
    free(data);
    return -1;
}

dir_entry* search_dir_entry(fat8* fs, char* fileName)  {
    int pos = pos_dir_entry(fs, fileName);
    if (pos == -1) {
        return NULL;
    }
    int sec = ID_RD + ((pos * SIZE_DIR_ENTRY) / fs->BPB_BytsPerSec); 
    uint8_t* data = malloc(SIZE_OF_BLOCK * sizeof(uint8_t));
    drv_read(fs->drive, sec, data);
    int offset = pos * sec * fs->BPB_BytsPerSec / SIZE_DIR_ENTRY;
    dir_entry* res = get_dir_entry(data + offset);
    free(data);
    return res;
} 

void write_dir_entry(fat8* fs, dir_entry* entry) {
    int pos = freepos_dir_entry(fs);
    if (pos == -1) {
        perror("Error: root directory is full\n");
        exit(1);
    }
    int sec = ID_RD + ((pos * SIZE_DIR_ENTRY) / fs->BPB_BytsPerSec); 
    uint8_t* data = malloc(SIZE_OF_BLOCK * sizeof(uint8_t));
    drv_read(fs->drive, sec, data);
    int offset = pos * sec * fs->BPB_BytsPerSec / SIZE_DIR_ENTRY;
    for (int i = 0; i < SIZE_FILENAME; i++) {
        data[offset + i] = entry->fileName[i];
    }
    data[offset + 11] = entry->attr;
    data[offset + 12] = entry->frsCluster;
    uint8_t* size = uint16_to_bytes(entry->fileSize);
    for (int i = 0; i < 4; i++) {
        data[offset + i + 13] = size[i];
    }
    free(size);
    drv_write(fs->drive, sec, data);
}

dir_entry* create_dir_entry(fat8* fs, char* fileName) {
    uint8_t* fat = fat_get(fs);
    int freeClsIndex = find_first_free_cluster(fat);
    
    dir_entry* entry = malloc(sizeof(dir_entry));
    if (entry == NULL) {
        perror("Error: fail to allocate memory\n");
        exit(1);
    }
    memcpy(entry->fileName, fileName, SIZE_FILENAME);
    entry->attr = 0x00;
    entry->frsCluster = freeClsIndex;
    entry->fileSize = 0;
    fat[freeClsIndex] = EOC;
    drv_write(fs->drive, ID_FAT, fat);
    write_dir_entry(fs, entry);
    fat_close(fat);
    return entry;
}

void update_dir_entry(fat8* fs, dir_entry* de) {
    int pos = pos_dir_entry(fs, de->fileName);
    if (pos == -1) {
        printf("Error: this dir entry is not existing: %s\n" , de->fileName);
    }
    int sec = ID_RD + ((pos * SIZE_DIR_ENTRY) / fs->BPB_BytsPerSec); 
    uint8_t* data = malloc(SIZE_OF_BLOCK * sizeof(uint8_t));
    drv_read(fs->drive, sec, data);
    int offset = pos * sec * fs->BPB_BytsPerSec / SIZE_DIR_ENTRY;
    uint8_t* size = uint32_to_bytes(de->fileSize);
    for(int i = 0; i < 4; i++) {
        data[offset + i + 13] = size[i];
    } 
    data[offset + 12] = de->frsCluster;
    drv_write(fs->drive, sec, data);
    free(data);
}


void delete_dir_entry(fat8* fs, dir_entry* de) {
    int pos = pos_dir_entry(fs, de->fileName);
    if (pos == -1) {
        printf("Error: this dir entry is not existing: %s\n" , de->fileName);
    }
    int sec = ID_RD + ((pos * SIZE_DIR_ENTRY) / fs->BPB_BytsPerSec); 
    uint8_t* data = malloc(SIZE_OF_BLOCK * sizeof(uint8_t));
    drv_read(fs->drive, sec, data);
    int offset = pos * sec * fs->BPB_BytsPerSec / SIZE_DIR_ENTRY;
    data[offset] = 0x00;
    data[offset + 1] = 0x00;
    drv_write(fs->drive, sec, data);
    free(data);
}

//---------------------------------------------------


file* create_new_file(fat8* fs, char* fileName) {
    dir_entry* entry = create_dir_entry(fs, fileName);
    size_t bufferSize = fs->ClusterSize * sizeof(char);
    file* f = malloc(sizeof(file) + bufferSize * sizeof(char));
    if (f == NULL) {
        perror("Error: fail to allocate memory\n");
        exit(1);
    }
    f->fs = fs;
    f->fileOffset = 0;
    f->fileSize = 0;
    f->bufOffset = 0;
    f->bufSize = 0;
    f->de = entry;
    f->curentCluster = entry->frsCluster;
    clean_cluster(fs, f->curentCluster);
    return f;
}

file* get_file(fat8* fs, dir_entry* entry) {
    int bufferSize = fs->ClusterSize;
    file* fd = malloc(bufferSize * sizeof(char) + sizeof(file));
    if (fd == NULL) {
        perror("Error: Fail to allocate memory\n");
        exit(1);
    }
    fd->fileOffset = 0;
    fd->fileSize = entry->fileSize;
    fd->curentCluster = entry->frsCluster;
    fd->de = entry;
    fd->bufSize = 0;
    fd->bufOffset = 0;
    fd->fs = fs;
}

file* file_open(fat8* fs, char* fileName) {
    char* alignName = calloc(SIZE_FILENAME, sizeof(char));
    align_fileName(alignName, fileName);
    dir_entry* entry = search_dir_entry(fs, alignName);
    if (entry == NULL) {
        free(entry);
        file* file = create_new_file(fs, alignName);
        return file;
    }
    file* file = get_file(fs, entry);
    return file;
}

void file_truncate(file* fd, size_t size) {
    int ClusterSize = fd->fs->ClusterSize;
    int oldNumCls = fd->fileSize / ClusterSize;
    int newNumCls = size / ClusterSize;
    uint8_t* fat = fat_get(fd->fs);
    // to smaller number of Clusters then before
    if (size < fd->fileSize && newNumCls < oldNumCls) {
        int dealocateCls = oldNumCls - newNumCls;
        uint8_t acCluster = fd->de->frsCluster;
        int pos = 0;
        while (oldNumCls >= pos) {
            uint8_t saveCls = fat[acCluster];
            if (pos == newNumCls) {
                fat[acCluster] = EOC;
            } else if (pos > newNumCls) {
                fat[acCluster] = FREE_CLUSTER;
            }
            acCluster = saveCls;
            pos++;
        }
    // to biger num of Clusters
    } else if (newNumCls > oldNumCls) {
        int alocateCls = newNumCls - oldNumCls;
        uint8_t acCluster = fd->curentCluster;
        while(fat[acCluster] != EOC) {
            acCluster = fat[acCluster];
        }
        for (int i = 0; i < alocateCls; i++) {
            uint8_t cls = find_first_free_cluster(fat);
            clean_cluster(fd->fs, cls);
            fat[acCluster] = cls;
            acCluster = cls;
            fat[acCluster] = EOC;
            fat_write(fd->fs, fat);
        }
    }
    fd->fileSize = size;
    fd->de->fileSize = size;
    update_dir_entry(fd->fs, fd->de);
    fat_write(fd->fs, fat);
    fat_close(fat);
}
 
void file_seek(file* fd, int position) {
    if (position > fd->fileSize) {
        perror("Error: to position out of range of file");
        exit(1);
    }
    fd->fileOffset = position;
    fd->bufOffset = fd->fileOffset / fd->fs->ClusterSize;
    fd->curentCluster = fd->de->frsCluster;
    uint8_t* fat = fat_get(fd->fs);
    for (int i = 0; i < position / fd->fs->CountOfClusters; i++) {
        fd->curentCluster = fat[fd->curentCluster];
    }
    fat_close(fat);
}

size_t file_stat(file* fd) {
    return fd->fileSize;
}

int file_tell(file* fd) {
   return fd->fileOffset;
}

// not enough tested on varient of inputs possible bugs 
void file_read(file* fd, int size, uint8_t* result) {
    uint8_t* fat = fat_get(fd->fs);
    if (fd->bufSize == 0) {
        load_curtenCluster_toBuff(fd);
    }
    //printf("siz: %d, of: %d, fof: %d\n", fd->bufSize, fd->bufOffset, fd->fileOffset);
    int read = 0;
    while(read < size) {
        if (fd->bufOffset >= fd->bufSize) {
            if (fd->bufOffset == fd->fs->ClusterSize) {
                fd->bufOffset = 0;
                fd->curentCluster = fat[fd->curentCluster];
            }
            load_curtenCluster_toBuff(fd);
            fd->bufSize = fd->fs->ClusterSize;
        }
        result[read] = fd->buf[fd->bufOffset];
        read++;
        fd->bufOffset++;
    }

}

// not enough tested on varient of inputs possible bugs 
void file_write(file* fd, uint8_t* data, int size) {
    uint8_t* fat = fat_get(fd->fs);
    if (fd->fileOffset + size < fd->fileSize) {
        file_truncate(fd, fd->fileSize + size);
    }
    
    int writeIndex = 0;
    load_curtenCluster_toBuff(fd);
    fd->bufOffset = fd->fileOffset % fd->fs->ClusterSize;
    //printf("oof: %d, size: %d\n", fd->bufOffset, fd->bufSize);
    while (writeIndex < size) {
        if (fd->bufOffset >= fd->fs->ClusterSize) {
            write_cluster(fd->fs, fd->buf, fd->curentCluster);
            fd->curentCluster = fat[fd->curentCluster];
            load_curtenCluster_toBuff(fd);
            fd->bufOffset = 0;
            fd->bufSize = 0;
        }
        fd->buf[fd->bufOffset] = data[writeIndex];
        fd->bufOffset++;
        fd->fileOffset++;
        writeIndex++;
    }
    write_cluster(fd->fs, fd->buf, fd->curentCluster);
    update_dir_entry(fd->fs, fd->de);
}

void file_close(file* fd) {
    free(fd);
}

// size is for return num of returned fileNames in returned vector
char** file_readdir(fat8* fs, int* size) {
    uint8_t count = count_dir_entrys(fs);
    char** fileNames = malloc(sizeof(char*) * count);
    if (fileNames == NULL) {
        perror("Error: faild to alocate memory\n");
        exit(1);
    }
    for (int i = 0; i < count; i++) {
        fileNames[i] = malloc(SIZE_FILENAME  * sizeof(char));
        if (fileNames[i] == NULL) {
            perror("Error: faild to alocate memory\n");
            exit(1);
        }
    }
    int index = 0;
    uint8_t* data = malloc(SIZE_OF_BLOCK * sizeof(char));
    for (int segment = 0; segment < SIZE_RD; segment++) {
        drv_read(fs->drive, ID_RD + segment, data);
        for (int entry = 0; entry < fs->BPB_BytsPerSec / SIZE_DIR_ENTRY; entry++) {
            char name[SIZE_FILENAME + 1];
            get_dir_entry_name(data + entry * SIZE_DIR_ENTRY, name);
            if (name[0] != 0x00) {
                memcpy(fileNames[index], name, SIZE_FILENAME);
                index++;
            }
        }
    }
    free(data);
    *size = count;
    return fileNames;
}

void file_delete(fat8* fs, char* name) {   
    file* f = file_open(fs, name);
    file_truncate(f, 0);
    uint8_t* fat = fat_get(fs);
    fat[f->de->frsCluster] = FREE_CLUSTER;
    delete_dir_entry(fs, f->de);
    fat_write(fs, fat);
    file_close(f);
    fat_close(fat);
}