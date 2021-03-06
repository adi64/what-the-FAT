#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "data.h"

// Unix / Windows interop
#ifndef O_BINARY
#define O_BINARY 0
#endif

/**
 * @brief Simple linked list for DIRENTRYs, used as queue
 */
typedef struct DirQueueItem_t {
    DIRENTRY* directoryEntry;
    struct DirQueueItem_t* next;
} DirQueueItem;

/**
 * @brief Global list head, only the 'next' pointer should be used!
 */
DirQueueItem* firstDirItem;

/**
 * @brief Pop list head and return entry
 * @return pointer to first item in queue
 */
DIRENTRY* dir_pop_front() {
    DirQueueItem* front = firstDirItem->next;
    DIRENTRY* ret = 0;
    if(firstDirItem->next) {
        ret = front->directoryEntry;
        firstDirItem->next = firstDirItem->next->next;
        free(front);
    }
    return ret;
}

/**
 * @brief Appends entry to the list tail
 * @param directoryEntry pointer to push back
 */
void* dir_push_back(DIRENTRY* directoryEntry) {
    DirQueueItem* newDirItem = (DirQueueItem*)malloc(sizeof(DirQueueItem));

    newDirItem->directoryEntry = directoryEntry;
    newDirItem->next = 0;

    DirQueueItem* lastItem = firstDirItem;

    while(lastItem && lastItem->next) {
        lastItem = lastItem->next;
    }

    lastItem->next = newDirItem;
}

/**
 * @brief Insert an entry after a given entry
 * @param entryBefore
 * @param newEntry
 */
void dir_insert(DIRENTRY* entryBefore, DIRENTRY* directoryEntry) {
    // find entry
    DirQueueItem* itemBefore = firstDirItem;

    while(itemBefore->directoryEntry != entryBefore && itemBefore->next) {
        itemBefore = itemBefore->next;
    }
    if(itemBefore->directoryEntry != entryBefore) {
        printf("could not find the given entry in the list!\n");
        return;
    }

    DirQueueItem* newDirItem = (DirQueueItem*)malloc(sizeof(DirQueueItem));

    newDirItem->directoryEntry = directoryEntry;
    newDirItem->next = itemBefore->next;
    itemBefore->next = newDirItem;
}

int handle;
unsigned char fatType;
BOOTSECTOR* bootsector;
char* FAT;
char dot[8] = {0x2E,0x20,0x20,0x20,0x20,0x20,0x20,0x20};
char dotdot[8] = {0x2E,0x2E,0x20,0x20,0x20,0x20,0x20,0x20};

/**
 * @brief print FAT volume information from bootsector
 * @param bootsector
 */
void printVolumeInformation(BOOTSECTOR* bootsector){
	if (!bootsector){
		printf("not a valid FAT bootsector!\n");
		return;
	}

	// append NULL byte to terminate string
	bootsector->EBPB.volumelabel[10] = '\0';
	bootsector->EBPB.fattype[7] = '\0';

        unsigned int numberofclusters = bootsector->BPB.numberofsectors / bootsector->BPB.sectorspercluster;
        if(numberofclusters < 4086) {
            fatType = 12;
        } else if(numberofclusters < 65526) {
            fatType = 16;
        } else {
            fatType = 32;
        }

	printf("Vendor: %s\n", bootsector->vendor);
	printf("Bios Parameter Block:\n");
	printf("  Sector size: %d\n", bootsector->BPB.sectorsize);
	printf("  Sectors per cluster: %d\n", bootsector->BPB.sectorspercluster);
	printf("  Reserved sectors: %d\n", bootsector->BPB.reservedsectors);
	printf("  Number of FATs: %d\n", bootsector->BPB.numberofFATs);
	printf("  Root entries: %d\n", bootsector->BPB.rootentries);
	printf("  Number of sectors: %d\n", bootsector->BPB.numberofsectors);
	printf("  Media type: %c\n", bootsector->BPB.mediatype);
	printf("  FAT sectors: %d\n", bootsector->BPB.FATsectors);
	printf("Sectors per Track: %d\n", bootsector->sectorspertrack);
	printf("Number of heads: %d\n", bootsector->numberofheads);
	printf("Hidden sectors: %d\n", bootsector->hiddensectors);
	printf("Total sectors: %d\n", bootsector->totalsectors);
	printf("Extended Bios Parameter Block\n");
	printf("  Drive number: %d\n", bootsector->EBPB.drivenumber);
	printf("  Reserved: %d\n", bootsector->EBPB.reserved);
	printf("  Signature: %d\n", bootsector->EBPB.signature);
	printf("  Serial number: %d\n", bootsector->EBPB.serialnumber);
	printf("  Volume label: %s\n", bootsector->EBPB.volumelabel);
	printf("  FAT type: %s\n", bootsector->EBPB.fattype);
	printf("  Bootcode: [not shown]\n");
	printf("  End of sector: %x\n", bootsector->EBPB.endofsector);
        printf("Total number of clusters: %d ( suggests FAT %d )\n", bootsector->BPB.numberofsectors / bootsector->BPB.sectorspercluster, fatType);
}

/* FAT starts with 2nd cluster after #FAT-bits for signature
 * FAT1 contains the File Allocation Table as unsigned char*
 * works for FAT 12 & 16 only
 */
unsigned short getnextcluster(unsigned short cluster)
{
    char high=cluster%2;
    unsigned short ncluster;
    unsigned int offset;

    switch(fatType)
    {
        case 12:
            offset=3;
            offset += ((cluster/2)-1)*3;
            offset += high;
            memcpy(&ncluster,&FAT[ offset ],2); // copy relevant bytes 0-1 or 1-2
            ncluster &= (high)? 0xFFF0:0x0FFF; //mask low or high part
            ncluster >>= (high)?4:0; // modify relevant bits
            break;
        case 16:
            offset=3;
            offset += cluster*2;
            memcpy(&ncluster,&FAT[ offset ],2); // copy relevant bytes 0-1 or 1-2
            break;
        default:
            exit(1);
            break;
    }
    return ncluster;
}

/* return fileoffset of a cluster
 * required variables: sectorssize; physical unit
 *                     sectorspercluster; number of sectors per cluster
 *                                                                    FAT (sectors)                                       RootDir (Bytes)
 *                     Dataregion = ( reservedsectors (0xe) + numberofFATs(0x10)*sectorsperFAT(0x16) ) * sectorsize + RootEntries(0x11) * 32 ; Start of Data Clusters
 *
 */
unsigned int getclusteroffset(unsigned short cluster)
{
    unsigned int ret=0;

    unsigned int DataRegion = (bootsector->BPB.reservedsectors + bootsector->BPB.numberofFATs * bootsector->BPB.FATsectors) *
                                    bootsector->BPB.sectorsize + bootsector->BPB.rootentries * 32;

    switch(fatType)
    {
        case 12:
        case 16:
            if(cluster == 0) {
                ret = (bootsector->BPB.reservedsectors + bootsector->BPB.numberofFATs * bootsector->BPB.FATsectors) * bootsector->BPB.sectorsize;
            }else{
                ret = DataRegion /* * bootsector->BPB.sectorsize */ + (cluster-2) * bootsector->BPB.sectorspercluster * bootsector->BPB.sectorsize;
            }
            break;
        default:
            exit(1);
    }
    //printf("Cluster %d has offset %d\n", cluster, ret);
    return ret;
}

/**
 * @brief Allocates 32 bytes on the heap, reads next DIRENTRY (from current position).
 * @return pointer to a DIRENTRY structure on the heap
 */
DIRENTRY* readDirectoryEntry() {
    DIRENTRY* directoryEntry = (DIRENTRY*)malloc(sizeof(DIRENTRY));

    //printf("reading from offset %d...\n", tell(handle));
    unsigned int bytesRead;
    if((bytesRead = read(handle, directoryEntry, sizeof(DIRENTRY))) != sizeof(DIRENTRY)) {
        printf("Could not read %d Bytes(read %d)! errno: %d\n", sizeof(DIRENTRY), bytesRead, errno);
        exit(1);
    }

    if(directoryEntry->name[0] == '\0') {
        return 0;
    }

    return directoryEntry;
}

/**
 * @brief isDirectory
 * @param directoryEntry
 * @return 1 if directoryEntry is a directory, 0 otherwise
 */
int isDirectory(DIRENTRY* directoryEntry) {
    if(!directoryEntry){
        return 0;
    }

    return ((directoryEntry->attr & (1<<4)) > 0);
}

/**
 * @brief Formats a date entry like 'DD.MM.YYYY'
 * @param date is in FAT format
 * @param buf is a buffer >= 11 bytes which will hold the formatted string
 */
void formatDirectoryEntryDate(unsigned short date, char* buf) {
    unsigned short year = 1980 + (date >> 9);
    unsigned short month = (date & 0x1E0) >> 5;
    unsigned short day = (date & 0x1F);
    sprintf(buf, "%02d.%02d.%d", day, month, year);
}

/**
 * @brief Formats a time entry like 'HH:MM:SS' (24 hour format)
 * @param time is in FAT format
 * @param buf is a buffer >= 9 bytes
 */
void formatDirectoryEntryTime(unsigned short time, char* buf) {
    // Bytes 15 - 11, hours 0 - 23
    unsigned short hour = time >> 11;
    // Bytes 10 - 05, minutes 0 - 59
    unsigned short minute = (time & 0x7E0) >> 5;
    // Bytes 04 - 00, seconds 0 - 29 ( times two, only even seconds)
    unsigned short second = (time & 0x1F) * 2;
    sprintf(buf, "%02d:%02d:%02d", hour, minute, second);
}

/**
 * @brief Formats directory entry like 'file.ext'
 * @param directoryEntry
 * @param buf is a buffer >= 13 bytes
 */
void formatDirectoryEntryName(DIRENTRY* directoryEntry, char* buf) {
    strncpy(buf, directoryEntry->name, 8);

    if(directoryEntry->ext[0] != ' '){

        // find end of file name
        unsigned short i = 7;
        while(i > 0 && buf[i] == 0x20) { --i; }

        // append '.' and file extension
        buf[i+1] = '.';
        strncpy(&buf[i+2], directoryEntry->ext, 3);
        buf[i+5] = '\0';
    }else{
        // terminate as early as possible
        unsigned short i;
        for(i = 1; i < 8 && buf[i] != 0x20; i++) ;
        buf[i] = '\0';
    }
}
/**
 * @brief parentDirectory
 * @param currentDirectoryEntry
 * @return parent DIRENTRY*
 */
DIRENTRY* parentDirectory(DIRENTRY* currentDirectoryEntry) {
    lseek(handle, getclusteroffset(currentDirectoryEntry->firstcluser), SEEK_SET);

    DIRENTRY* directoryEntry = 0;
    DIRENTRY* parentDirectoryEntry = 0;

    // read current directory, find parent entry ('..')
    while(directoryEntry = readDirectoryEntry()) {
        if(memcmp(directoryEntry->name, dotdot, 8) == 0) {
            parentDirectoryEntry = directoryEntry;
            break;
        }
    }

    return parentDirectoryEntry;
}

/**
 * @brief Retrieves the current folder's name (handy if you only have a '.' entry at hand)
 * @param currentDirectoryEntry
 * @param buf is a buffer big enough to hold the folder name
 */
void currentFolderName(DIRENTRY* currentDirectoryEntry, char* buf) {

    DIRENTRY* directoryEntry;
    DIRENTRY* parentDirectoryEntry = 0;

    parentDirectoryEntry = parentDirectory(currentDirectoryEntry);

    // read parent directory, find entry that matches current (original) folder's first cluster
    lseek(handle, getclusteroffset(parentDirectoryEntry->firstcluser), SEEK_SET);

    while(directoryEntry = readDirectoryEntry()) {
        if(directoryEntry->firstcluser == currentDirectoryEntry->firstcluser) {
            formatDirectoryEntryName(directoryEntry, buf);
            return;
        }
    }
}

/**
 * @brief Retrieves the absolute path of a given directory
 * @param directoryEntry
 * @param buf is a buffer big enough to hold the absolute path
 */
void absoluteDirectoryPath(DIRENTRY* directoryEntry, char* buf) {
    // if there is no parent directory, we are at root level
    DIRENTRY* parent = parentDirectory(directoryEntry);
    if(parent == 0) {
        sprintf(buf, "\\");
        return;
    }

    // if we are not at root level, fire up the recursion
    absoluteDirectoryPath(parent, buf);

    // concatenate the path until here and this directory's name
    unsigned short stringLength = strlen(buf);
    char dirname[512];
    currentFolderName(directoryEntry, dirname);
    sprintf(&buf[stringLength], "\\%s", dirname);
}

/**
 * @brief Prints a directory entry in a format similar to 'dir'
 * @param directoryEntry
 */
void printDirectoryEntry(DIRENTRY* directoryEntry) {

    // date
    char changedate[11];
    formatDirectoryEntryDate(directoryEntry->changedate, changedate);
    printf("%s ", changedate);

    // time
    char changetime[9];
    formatDirectoryEntryTime(directoryEntry->changetime, changetime);
    printf("%s ", changetime);

    // directory or file
    if(isDirectory(directoryEntry)) {
        printf(" <DIR> ");
    }else{
        printf("       ");
    }

    // file size
    printf("%10d ", directoryEntry->size);

    // file name
    char name[14];
    formatDirectoryEntryName(directoryEntry, name);
    printf("%-12s  ", name);

    /*
    // cluster dev info
    unsigned short firstcluster = directoryEntry->firstcluser;
    unsigned short nextcluster = getnextcluster(firstcluster);

    printf("Cluster(s) %d", directoryEntry->firstcluser);
    if(nextcluster < CLUSTER_LAST_MIN) {
           printf(" -> %d -> ...", nextcluster);
    }
    */

    printf("\n");
}

void handleLFN(DIRENTRY* directoryEntry, char* LFNBuffer) {
    DIRENTRY_V* lfnEntry = (DIRENTRY_V*)directoryEntry;

    int sequenceNumber = lfnEntry->sequence_number & 0x5;
    int lastSequenceNumber = (lfnEntry->sequence_number & 0x6) == 0x6;
    printf("Sequence number: %d\n", sequenceNumber);
    if(lastSequenceNumber) {
        printf("this is the last sequence number.\n");
    }

    int offset = sequenceNumber * 13;

    if(offset > 255) {
        printf("offset %d > 255!\n", offset);
        return;
    }

    memcpy(&LFNBuffer[offset], lfnEntry->name_0, 10);
    memcpy(&LFNBuffer[offset+10], lfnEntry->name_1, 12);
    memcpy(&LFNBuffer[offset+22], lfnEntry->name_2, 4);

    printf("LFN so far: %s\n", LFNBuffer);
}

/**
 * @brief Reads a folder listing, prints all items and adds all subdirectories to the global queue
 * @param offset where the first directory entry of the folder structure begins
 */
void listDirectory(unsigned int offset) {

    // do not read into next cluster
    long maxOffset = offset + (bootsector->BPB.sectorspercluster * bootsector->BPB.sectorsize);
    long newOffset = offset;
    lseek(handle, newOffset, SEEK_SET);

    // the reference point in the list where we want to add the subdirectories
    // by default, we want to add subdirectories to the top so that we get depth-first search
    DIRENTRY* referencePoint = firstDirItem->directoryEntry;

    // this is where we read into
    DIRENTRY* directoryEntry;

    // in case we have to handle VFAT / LFN entries, prepare the buffer
    char LFN[260];

    while((newOffset < maxOffset) && (directoryEntry = readDirectoryEntry())) {
        // we need to preserve the offset in case any operations move the file pointer
        //newOffset = tell(handle);
        newOffset = lseek(handle, 0, SEEK_CUR);

        if(memcmp(directoryEntry->name, dot, 8) == 0) {
            char buf2[1024];
            absoluteDirectoryPath(directoryEntry, buf2);
            printf("Directory of %s\n", buf2);
        }

        if(directoryEntry->attr == DIRENTRY_ATTR_VFAT) {
            printf("this is a VFAT entry!\n");
            handleLFN(directoryEntry, LFN);
        }else{
            printDirectoryEntry(directoryEntry);
        }

        if(isDirectory(directoryEntry)) {
            // add subdirectories to the queue
            // please ignore the current directory entry ('.') and parent ('..')
            if((memcmp(directoryEntry->name, dot, 8) != 0) && (memcmp(directoryEntry->name, dotdot, 8) != 0)) {
                dir_insert(referencePoint, directoryEntry);
                referencePoint = directoryEntry;
                //dir_push_back(directoryEntry);
            }
        }

        lseek(handle, newOffset, SEEK_SET);
    }

}

/**
 * @brief Recursively lists all directories in the global queue and empties that queue
 */
void list_recursive() {
    DIRENTRY* directoryEntry;

    unsigned int nextCluster;
    while((directoryEntry = dir_pop_front())) {

        nextCluster = directoryEntry->firstcluser;
        do{
            listDirectory(getclusteroffset(nextCluster));
        }while((nextCluster = getnextcluster(nextCluster)) < CLUSTER_LAST_MIN);
        printf("\n");
    }
}

int main(int argc, char* argv[]) {

    // initialize list
    firstDirItem = (DirQueueItem*)malloc(sizeof(DirQueueItem));
    firstDirItem->directoryEntry = 0;
    firstDirItem->next = 0;

    char filename[1024] = "BSA.img";
    if(argc != 2) {
        printf("Usage: %s filename\n", argv[0]);
        printf("I will pick file '%s' for you.\n", filename);
    }else{
        strncpy(filename, argv[1], 1023);
    }

    handle = open(filename, O_RDONLY | O_BINARY);

	if (handle == -1){
		printf("Can't open file! errno: %d\n", errno);
		exit(1);
	}

    bootsector = (BOOTSECTOR*)malloc(sizeof(char)* 512);

    unsigned int bytesRead;
    if ((bytesRead = read(handle, bootsector, 512)) != 512) {
        printf("Could not read 512 Bytes (read %d)! errno: %d\n", bytesRead, errno);
		exit(1);
	}

	printVolumeInformation(bootsector);

    unsigned int firstFATStartPos = bootsector->BPB.reservedsectors * bootsector->BPB.sectorsize;
    unsigned int FATSize = bootsector->BPB.FATsectors * bootsector->BPB.sectorsize;

    FAT = (char*)malloc(sizeof(char) * FATSize);

    printf("First FAT starting at byte %d, length %d\n", firstFATStartPos, FATSize);

    unsigned int newPos;
    newPos = lseek(handle, firstFATStartPos, SEEK_SET);

    if((bytesRead = read(handle, FAT, FATSize)) != FATSize) {
        printf("Could not read %d Bytes(read %d)! errno: %d\n", FATSize, bytesRead, errno);
        exit(1);
    }

    unsigned int rootDirectoryStartPos = firstFATStartPos + (bootsector->BPB.numberofFATs * FATSize);
    unsigned int rootDirectoryStartCluster = bootsector->BPB.reservedsectors + bootsector->BPB.FATsectors * bootsector->BPB.numberofFATs;


    printf("Root directory starting at cluster %d / byte %d\n", rootDirectoryStartCluster, rootDirectoryStartPos);

    newPos = lseek(handle, rootDirectoryStartPos, SEEK_SET);

    printf("\n");

    // list root directory and add subdirectories to global queue
    listDirectory(rootDirectoryStartPos);
    printf("\n");

    // recursively list all directories inside the global queue
    list_recursive();

	return 0;
}
