#include "drv.h"
#include "fat8.h"

// testning function for generating data
void generate_abc_data(uint8_t *data, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        data[i] = 'a' + i / 64;  
    }
}

// console log of FAT table for chack
void print_fat(fat8* fs) {
    printf("----------------------------------\n");
    printf("FAT\n");
    uint8_t* fat = fat_get(fs);
    for (int i = FAT_FIRST; i < FAT_LAST; i++) {
        printf("%d, ", fat[i]);
    }
    fat_close(fat);
    printf("\n");
    printf("----------------------------------\n");
}

// po kazdym vypisu je print fat tabulky pro ukazaní zmen ve file systemu
int main() {
    char* nameDisk = "disk";
    drv_manufacture(nameDisk,1034);
    drive* d = drv_open(nameDisk);
    fs_format(d);
    fat8* fs = fs_open(d);
    // vypise zakladni informace o File systemu a Drivu
    printf("\n////FILE SYSTEM INFO////\n");
    printf("BPB_BytsPerSec: %d\n", fs->BPB_BytsPerSec);
    printf("BPB_SecPerClus: %d\n", fs->BPB_SecPerClus);
    printf("BPB_NumFats: %d\n", fs->BPB_NumFATs); 
    printf("BPB_TotSec: %d\n", fs->BPB_TotSec32); 
    printf("BPB_FATSz8: %d\n", fs->BPB_FATSz8); 
    printf("BS_FilSysType: %s\n", fs->BS_FilSysType);
    printf("Drive size: %ld\n", drv_stat(fs->drive) / SIZE_OF_BLOCK);
    printf("///////////////////////\n\n");
    
    int count = 4;
    char** fileNames = malloc(count * sizeof(char*));
    for (int i = 0; i < 10; i++) {
        fileNames[i] = malloc(SIZE_FILENAME + 2 * sizeof(char));
    }
    strcpy(fileNames[0], "test1.txt");
    strcpy(fileNames[1], "test2.txt");
    strcpy(fileNames[2], "test3567.txt");
    strcpy(fileNames[3], "test4667.txt");
    printf("----------------------------------\n");
    printf("Creating files\n");
    printf("----------------------------------\n");
    // vytvori dir entrys a vypise je
    for (int i = 0; i < count; i++) {
        file* fd = file_open(fs, fileNames[i]);
        printf("file { num: %d, name: %s } = created\n", i, fd->de->fileName);
        file_close(fd);
    }
    printf("----------------------------------\n");
    print_fat(fs);
    printf("Truncate files to bigger value\n");
    printf("----------------------------------\n");
    printf("name, exp size, size, cls\n");
    printf("----------------------------------\n");
    // pouzije truncate (zvetsi) a vypise hodnoty souboru  
    for (int i = 0; i < count; i++) {
        int size = rand() % (100000 + 1 - 2000) + 100;
        file* fd = file_open(fs, fileNames[i]);
        file_truncate(fd, size);
        printf("%s, %d, %d, %d\n", fd->de->fileName, size, fd->fileSize, (fd->fileSize / fs->ClusterSize) + 1);
        file_close(fd);
    }
    print_fat(fs);
    printf("----------------------------------\n");
    printf("Truncate files to smaller value\n");
    printf("----------------------------------\n");
    printf("name, exp size, size, cls\n");
    printf("----------------------------------\n");
    // pouzije truncate (zmensi) a vypise hodnoty souboru  
    for (int i = 0; i < count; i++) {
        int size2 = rand() % (10000 + 1 - 0) + 0;
        file* fd = file_open(fs, fileNames[i]);
        file_truncate(fd, size2);
        printf("%s, %d, %d, %d\n", fd->de->fileName, size2, fd->fileSize, (fd->fileSize / fs->ClusterSize) + 1);
        file_close(fd);
    }
    print_fat(fs);
    printf("----------------------------------\n");
    printf("Write and Read data\n");
    printf("----------------------------------\n");
    // zapise data a nasledne posune ukazatel na nulu a vypise je
    // TODO: potreba otestovat na vice moznosti 
    for (int i = 0; i < count; i++) {
        int size3 = rand() % (1000 + 1 - 1) + 1;    
        file* fd = file_open(fs, fileNames[i]);
        printf("name: %s\n", fd->de->fileName);
        uint8_t* w = calloc(size3, sizeof(uint8_t)); 
        generate_abc_data(w, size3);
        file_write(fd, w, size3);
        printf("data writed \n");
         printf("file offset: %d\n", fd->fileOffset);
        file_seek(fd, 0);
        printf("file offset after seek: %d\n", fd->fileOffset);
        uint8_t* r = calloc(size3, sizeof(uint8_t)); 
        file_read(fd, size3, r);
        printf("data readed: \n");
        printf("%s\n", r);
    }
    print_fat(fs);
    printf("----------------------------------\n");
    printf("All files in FS\n");
    printf("----------------------------------\n");
    int size;
    char** files = file_readdir(fs, &size);
    // vypise vsechny soubory ve root dir
    for (int i = 0; i < size; i++) {
        printf("%s\n", files[i]);
    }
    printf("----------------------------------\n");
    printf("Delete Files in FS (delete all files)\n");
    printf("----------------------------------\n");
    // Soubory kere odstranil vypise
    for (int i = 0; i < size; i++) {
        file_delete(fs, files[i]);
        printf("%s\n", files[i]);
    }
    print_fat(fs);
    free(files);
    fs_close(fs);
    drv_close(d);
    return 0;
}
