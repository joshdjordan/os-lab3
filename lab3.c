#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define INODE_SIZE 56
#define LIST_SIZE 128
#define BLOCK_SIZE 1024

typedef struct {
    char name[16];          // file name
    int size;               // file size (in number of blocks)
    int blockPointer[8];    // direct block pointers
    int used;               // 0 => inode is free; 1 => in use
} inode;

typedef struct {
    char instrucT[10];
    char fileName[10];
    int value;
} cmd;

void create(char name[16], int size) {
    inode node;
    int availSpace = 0;
    int freeBlockList[LIST_SIZE];
    int availNodeInd = -1;

    // step 1: check to see if there is enough available space on the disk to write file
    FILE *disk = fopen("mydisk0", "rb+");
    if (disk == NULL) {
        perror("error opening file for create");
        exit(1);
    }

    fread(freeBlockList, sizeof(int), LIST_SIZE, disk);

    // setting available space on disk
    for (int i = 0; i < LIST_SIZE; i++) {
        if (freeBlockList[i] == 0) {
            availSpace++;
        }
    }

    // checks if enough space on disk
    if (availSpace < size) {
        perror("Insufficient available space on disk");
        fclose(disk);
        exit(1);
    }

    // step 2: search for an available inode to use to take our file
    for (int i = LIST_SIZE + 1; i < BLOCK_SIZE; i += INODE_SIZE) {
        fseek(disk, i, SEEK_SET);
        fread(&node, sizeof(inode), 1, disk);

        if (node.used == 0) {
            availNodeInd = i;
            break;
        }
    }

    if (availNodeInd == -1) {
        perror("No free inodes locatedn");
        fclose(disk);
        exit(1);
    }

    // setting the inode as 1 for being used
    node.used = 1;
    strcpy(node.name, name);
    node.size = size;

    // step 3: allocate the data block via freeBlockList
    for (int i = 0; i < size; i++) {
        int blockInd = -1;
        for (int j = 1; j < LIST_SIZE; j++) {
            blockInd = j;
            freeBlockList[j] = 1;
            node.blockPointer[i] = blockInd;
            break;
        }

        // checks after the inner loop completes if the server was unable to allocate
        if (blockInd == -1) {
            perror("Unable to allocate data block");
            fclose(disk);
            exit(1);
        }
    }

    // step 4: write out data to the disk
    fseek(disk, 0, SEEK_SET);
    fwrite(freeBlockList, sizeof(int), LIST_SIZE, disk);

    fseek(disk, LIST_SIZE + availNodeInd * sizeof(inode), SEEK_SET);
    fwrite(&node, sizeof(inode), 1, disk);

    fclose(disk);
}

void delete(char name[16]) {
    inode node;
    int freeBlockList[LIST_SIZE];

    FILE *disk = fopen("mydisk0", "rb+");
    if (disk == NULL) {
        perror("error opening file to delete");
        exit(1);
    }
    
    // step 1: search for the file to be deleted
    for (int i = LIST_SIZE + 1; i < BLOCK_SIZE; i += INODE_SIZE) {
        fseek(disk, i, SEEK_SET);
        fread(&node, sizeof(inode), 1, disk);

        if (node.used == 1 && !strcmp(node.name, name)) {
            // step 2: setting the FBL
            fseek(disk, 0, SEEK_SET);
            fread(freeBlockList, sizeof(int), LIST_SIZE, disk);

            for (int j = 0; j < node.size; j++) {
                freeBlockList[node.blockPointer[j]] = 0;
            }

            // step 3: setting use field
            node.used = 0;

            // step 4: writing data back to disk
            fseek(disk, 0, SEEK_SET);
            fwrite(freeBlockList, sizeof(int), LIST_SIZE, disk);

            fseek(disk, i, SEEK_SET);
            fwrite(&node, sizeof(inode), 1, disk);

            break;
        }
    }
    fclose(disk);
}

void read(char name[16], int blockNum, char buffer[1024]) {
    inode node;
    int iter = 0;

    FILE *disk = fopen("mydisk0", "rb+");
    if (disk == NULL) {
        perror("error opening file to read");
        exit(1);
    }
    
    // step 1: search for the file to be read
    for (int i = LIST_SIZE + 1; i < BLOCK_SIZE; i += INODE_SIZE) {
        fseek(disk, i, SEEK_SET);
        fread(&node, sizeof(inode), 1, disk);

        if (node.used == 1 && !strcmp(node.name, name)) {
            // step 2:
            if (blockNum >= node.size) {
                perror("Incorrect size.\n");
                fclose(disk);
                exit(1);
            }
            int addr = node.blockPointer[blockNum];
            fseek(disk, addr * BLOCK_SIZE, SEEK_SET);
            fread(buffer, sizeof(char), BLOCK_SIZE, disk);
        }
    }
}

void write(char name[16], int blockNum, char buffer[1024]) {
    inode node;

    FILE *disk = fopen("mydisk0", "rb+");
    if (disk == NULL) {
        perror("error opening file to write");
        exit(1);
    }

    // step 1: search for the file to be written
    for (int i = LIST_SIZE + 1; i < BLOCK_SIZE; i += INODE_SIZE) {
        fseek(disk, i, SEEK_SET);
        fread(&node, sizeof(inode), 1, disk);

        if (node.used == 1 && !strcmp(node.name, name)) {
            // step 2: write buffer to the correct block in the disk
            if (blockNum >= node.size) {
                perror("Incorrect size");
                fclose(disk);
                exit(1);
            }

            int addr = node.blockPointer[blockNum];
            fseek(disk, addr * BLOCK_SIZE, SEEK_SET);
            fwrite(buffer, sizeof(char), BLOCK_SIZE, disk);
        }
    }
}

void ls() {
    inode node;

    FILE *disk = fopen("mydisk0", "rb+");
    if (disk == NULL) {
        perror("error opening file for ls");
        exit(1);
    }

    printf("\n----- FILES ON DISK -----\n");
    printf("Name\t\tSize\n");

    for (int i = LIST_SIZE + 1; i < BLOCK_SIZE; i += INODE_SIZE) {
        fseek(disk, i, SEEK_SET);
        fread(&node, INODE_SIZE, 1, disk);

        if (node.used == 1) {
            printf("%s\t\t%d\n", node.name, node.size);
        }
    }
    printf("-------------------------\n\n");
}

int main() {
    cmd instructions[50];
    char info[100];
    const char *delim = " ";
    char dummyBuffer[1024];
    char buffer[1024];
    char diskName[20];
    int infoLength;

    memset(dummyBuffer, '1', sizeof(dummyBuffer));

    FILE *test = fopen("sample-test.txt", "r");
    if (test == NULL) {
        perror("error opening file to test");
        exit(1);
    }

    int tokIter = 0;
    while (fgets(info, sizeof(info), test) && tokIter < 50) {
        infoLength = strlen(info);
        if (infoLength > 0 && info[infoLength - 1] == '\n') {
            info[infoLength - 1] = '\0'; // null terminating new line to prevent final quote to be on new line
        }

        char *token = strtok(info, delim);

        if (token != NULL) {
            strncpy(instructions[tokIter].instrucT, token, sizeof(instructions[tokIter].instrucT) - 1);
            instructions[tokIter].instrucT[strlen(instructions[tokIter].instrucT)] = '\0'; // null termination
            token = strtok(NULL, delim);
        }

        if (token != NULL) {
            strncpy(instructions[tokIter].fileName, token, sizeof(instructions[tokIter].fileName) - 1);
            instructions[tokIter].fileName[strlen(instructions[tokIter].fileName)] = '\0'; // nulltermination
            token = strtok(NULL, delim);
        }

        if (token != NULL) {
            instructions[tokIter].value = (int)strtol(token, NULL, 10);
            token = strtok(NULL, delim);
        }
        tokIter++;
    }
    fclose(test);

    for (int i = 0; i < tokIter; i++) {
        if (!strcmp(instructions[i].instrucT, "C")) {
            create(instructions[i].fileName, instructions[i].value);

            printf("\n----- Created File -----\n");
            printf("   Name\t\tSize\n");
            printf("  %s\t %d\n", instructions[i].fileName, instructions[i].value);
            printf("------------------------\n\n");
        }

        else if (!strcmp(instructions[i].instrucT, "D")) {
            delete(instructions[i].fileName);

            printf("\n------ Deleted File ------\n");
            printf("    File Name: %s\n", instructions[i].fileName);
            printf("--------------------------\n\n");
        }

        else if (!strcmp(instructions[i].instrucT, "W")) {
            write(instructions[i].fileName, instructions[i].value, dummyBuffer);

            printf("\n------ Wrote File ------\n");
            printf("  Name\t   Block Number\n");
            printf(" %s\t\t%d\n", instructions[i].fileName, instructions[i].value);
            printf("-------------------------\n\n");
        }

        else if (!strcmp(instructions[i].instrucT, "R")) {
            read(instructions[i].fileName, instructions[i].value, buffer);

            printf("\n------ Read File ------\n");
            printf("  Name\t   Block Number\n");
            printf(" %s\t%d\n", instructions[i].fileName, instructions[i].value);
            printf("-----------------------\n\n");
        }

        else if (!strcmp(instructions[i].instrucT, "L")) {
            ls();
        }
    }
}