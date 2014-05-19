#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "data.h"

unsigned char fatType;

void printVolumeInformation(BOOTSECTOR* bootsector){
	if (!bootsector){
		printf("not a valid FAT bootsector!\n");
		return;
	}

	// append NULL byte to terminate string
	bootsector->EBPB.volumelabel[10] = '\0';
	bootsector->EBPB.fattype[7] = '\0';

        short numberofclusters = bootsector->BPB.numberofsectors / bootsector->BPB.sectorspercluster;
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

int main(int argc, char* argv[]) {

	int handle = open("BSA.img", O_RDONLY);

	if (handle == -1){
		printf("Can't open file! errno: %d\n", errno);
		exit(1);
	}

	BOOTSECTOR* bootsector = (BOOTSECTOR*)malloc(sizeof(char)* 512);

	if (read(handle, bootsector, 512) != 512) {
		printf("Could not read 512 Bytes! errno: %d\n", errno);
		exit(1);
	}

	printVolumeInformation(bootsector);

        unsigned int firstFATStartPos = bootsector->BPB.reservedsectors * bootsector->BPB.sectorsize;

        printf("First FAT starting at byte %d\n", firstFATStartPos);

        lseek(handle, firstFATStartPos, SEEK_SET);

        unsigned int rootDirectoryStartPos = firstFATStartPos + (bootsector->BPB.numberofFATs * bootsector->BPB.FATsectors * bootsector->BPB.sectorsize);

        printf("Root directory starting at byte %d\n", rootDirectoryStartPos);


	//getchar();

	return 0;
}
