#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "data.h"

unsigned char fatType;
BOOTSECTOR* bootsector;
char* FAT;

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
            ret = DataRegion * bootsector->BPB.sectorsize + (cluster-2) * bootsector->BPB.sectorspercluster * bootsector->BPB.sectorsize;
            break;
        default:
            exit(1);
    }
    printf("Cluster %d has offset %d\n", cluster, ret);
    return ret;
}

DIRENTRY* readDirectoryEntry(unsigned short cluster) {
    DIRENTRY* directoryEntry = (DIRENTRY*)malloc(sizeof(DIRENTRY));

    return directoryEntry;
}

void printDirectoryEntry(DIRENTRY* directoryEntry) {
    char name[13];

    strncpy(name, directoryEntry->name, 8);

    if(directoryEntry->ext[0] != ' '){
        name[8] = '.';
        strncpy(&name[9], directoryEntry->ext, 3);
        name[13] = '\0';
    }else{
        name[8] = '\0';
    }

    // date
    printf("00.00.0000 ");

    // time
    printf("00:00:00 ");

    // directory or file
    printf(" <DIR> ");

    // file size
    printf("%10d ", 0);

    // file name
    printf("%s", name);

    printf("\n");
}

void listDirectory(DIRENTRY* directoryEntry) {
    printf("first cluster: %d\n", directoryEntry->firstcluser);
    printf("next cluster: %d\n", getnextcluster(directoryEntry->firstcluser));

    printDirectoryEntry(directoryEntry);
}

int main(int argc, char* argv[]) {

    int handle = open("BSA.img", O_RDONLY | O_BINARY);

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
    printf("now at %d\n", newPos);

    if((bytesRead = read(handle, FAT, FATSize)) != FATSize) {
        printf("Could not read %d Bytes(read %d)! errno: %d\n", FATSize, bytesRead, errno);
        exit(1);
    }

    unsigned int rootDirectoryStartPos = firstFATStartPos + (bootsector->BPB.numberofFATs * FATSize);

    printf("Root directory starting at byte %d\n", rootDirectoryStartPos);

    DIRENTRY* firstRootEntry = (DIRENTRY*)malloc(sizeof(DIRENTRY));
    newPos = lseek(handle, rootDirectoryStartPos, SEEK_SET);
    printf("now at %d\n", newPos);

    if((bytesRead = read(handle, firstRootEntry, sizeof(DIRENTRY))) != sizeof(DIRENTRY)) {
        printf("Could not read %d Bytes(read %d)! errno: %d\n", sizeof(DIRENTRY), bytesRead, errno);
        exit(1);
    }

    listDirectory(firstRootEntry);

	//getchar();

	return 0;
}
