#include <stdio.h>
#include <string.h>

typedef struct {
    char name[16];          //file name
    int size;               // file size (in number of blocks)
    int blockPointers[8];   // direct block pointers
    int used;               // 0 => inode is free; 1 => in use
} inode;

int create(char name[16], int size) {
    inode node;
    int availSpace = 0;
    int freeBlockList[128];
    int availNodeInd = -1;

    // step 1: check to see if there is enough available space on the disk to write file
    FILE *disk = fopen("mydisk0", "rb+");
    fread(freeBlockList, sizeof(int), 128, disk);

    // setting available space on disk
    for (int i = 0; i < 128; i++) {
        if (freeBlockList[i] == 0) {
            availSpace++;
        }
    }

    // checks if enough space on disk
    if (availSpace < size) {
        perror("Insufficient available space on disk.\n");
        close(disk);

        return -1;
    }

    // step 2: search for an available inode to use to take our file
    
}

int main() {

}