/*
 * data.h
 *
 * FAT12 structures and constants
 *
 * Bernhard Rabe 
 *
 * 20.11.2006
 */

#ifndef __DATA_H
#define __DATA_H

#include <wchar.h>

/* FAT struture 
 *  BOOTSECTOR
 *  1st FAT
 *  additional FATs 
 *  Directory
 *  Data 
 */


#define DIRENTRY_LAST	0x00
#define DIRENTRY_MASKE5 0x05
#define DIRENTRY_CURENT 0x2E
#define DIRENTRY_PARENT 0x2E2E
#define DIRENTRY_EMPTY	0xE5

#define DIRENTRY_ATTR_READONLY  0x1
#define DIRENTRY_ATTR_HIDDEN    0x2
#define DIRENTRY_ATTR_SYSTEM    0x4
#define DIRENTRY_ATTR_VOLUME    0x8
#define DIRENTRY_ATTR_DIR       0x10
#define DIRENTRY_ATTR_ARCHIVE   0x20
#define DIRENTRY_ATTR_RESERVED1 0x40
#define DIRENTRY_ATTR_RESERVED2 0x80
#define DIRENTRY_ATTR_VFAT      0x0F//DIRENTRY_ATTR_VOLUME|DIRENTRY_ATTR_SYSTEM|DIRENTRY_ATTR_HIDDEN|DIRENTRY_ATTR_READONLY 

//macros
#define CLUSTER_FREE            0x000
#define CLUSTER_RESERVED_MIN(b) ((1<<(b))-16)   //   0xFF0
#define CLUSTER_RESERVED_MAX(b) ((1<<(b))-10)   //0xFF6
#define CLUSTER_BAD(b)          ((1<<(b))-9)    //0xFF7
#define CLUSTER_LAST_MIN(b)     ((1<<(b))-8)    //0xFF8
#define CLUSTER_LAST_MAX(b)     ((1<<(b))-1)    //0xFFF
//#define CLUSTER_LAST_MIN        0xFF8
//#define CLUSTER_LAST_MAX        0xFFF


#define VFAT_END 0x0000
#define VFAT_SEQUENCE_END(v)  (v)>>6  //check the 6th bit
#define VFAT_SEQUENCE_NUMBER(v) (v)&0x1F //mask all bits above 5th

#define IS_DIR(attr)		  (((attr)&DIRENTRY_ATTR_DIR)==DIRENTRY_ATTR_DIR)
#define IS_VFAT(attr)		  (((attr)&DIRENTRY_ATTR_VFAT)==DIRENTRY_ATTR_VFAT)	



#define FAT12_SIGNATURE  "FAT12   "
#define FAT16_SIGNATURE  "FAT16   "
#define FAT32_SIGNATURE  "FAT32   "
#define FAT12_BITS 12
#define FAT16_BITS 16
#define FAT32_BITS 32

#ifdef _WIN32
#define strdup _strdup
#endif




#pragma pack(1) //FORCE alignment 1 byte alignment
typedef struct _BOOTSEC_T{
    char bootroutine[3]; //0
    char vendor[8];      //3
    struct _BPB{   //packed BIOS Parameters
        unsigned short sectorsize; // 0b
        unsigned char sectorspercluster; //0d
        unsigned short reservedsectors; //0e
        unsigned char numberofFATs; //10
        unsigned short rootentries; //11
        unsigned short numberofsectors; //13
        unsigned char mediatype; //15
        unsigned short FATsectors; //16       
    } BPB;
    unsigned short sectorspertrack; //18
    unsigned short numberofheads; //1a
    unsigned int hiddensectors; //1c
    unsigned int totalsectors; // 20
    struct _EBPB{
        unsigned char drivenumber; //24
        unsigned char reserved; //25
        unsigned char signature; // 26
        unsigned int serialnumber; // 27
        unsigned char volumelabel[11]; // 2b
        unsigned char fattype[8] ; //36 
        unsigned char bootcode[448]; //3e
        unsigned short endofsector; // 1fe  0x55 0xaa       
    } EBPB;
} BOOTSECTOR;  //512 Byte


typedef struct _DIRENTRY_T{
    unsigned char       name[8]; //0x0
    unsigned char		ext[3];  //0x8
    unsigned char       attr;//0x0b
    char				reserved;//0x0c
    unsigned char		createmillis;//0x0d
    unsigned short      createtime;//0x0e
    unsigned short      createdate;//0x10
    unsigned short      lastaccessdate;//0x12
    unsigned short      EAindex;//0x14  highbytes of cluster for FAT32
    unsigned short      changetime;//0x16
    unsigned short      changedate;////0x18
    unsigned short      firstcluser;//0x1a
    unsigned int        size;//0x1c
} DIRENTRY; //32 Byte


typedef struct _DIRENTRY_V{
	unsigned char		sequence_number; //0x0
	wchar_t				name_0[5]; //0x1
	unsigned char		attr; //allways 0xF  0xb
	unsigned char		reserved; //0x0   0xc
	unsigned char		checksum; //0xd
	wchar_t				name_1[6]; // 0xe
	unsigned short		firstcluster; //0x1a allways 0
	wchar_t				name_2[2]; //0x1c
} DIRENTRY_V; //32 Bytes
#pragma pack()





//DIRENTRY dot   = {".       ","   ",DIRENTRY_ATTR_DIR,0,0,0,0,0,0,0,0,-1,0};
//DIRENTRY dotdot= {"..      ","   ",DIRENTRY_ATTR_DIR,0,0,0,0,0,0,0,0,-1,0};
//DIRENTRY empty = {{0xE5,32,32,32,32,32,32,32},"  ",0,0,0,0,0,0,0,0,0,0,0};
//DIRENTRY last  = {{0x0,32,32,32,32,32,32,32} ,"  ",0,0,0,0,0,0,0,0,0,0,0};
#endif

