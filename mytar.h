#ifndef MYTAR_H
#define MYTAR_H

/*the option flags*/
#define NUM_OPTS 6
#define C_FLAG 0
#define T_FLAG 1
#define X_FLAG 2
#define V_FLAG 3
#define S_FLAG 4
#define F_FLAG 5

#define BUF_SIZE 512 /*size of a block in tar*/
#define OCTAL 8 /*for octal number encoding*/
#define MAGIC_NUM "ustar" /*magic number*/

/*command string max size??*/
#define MAX_FILES 100

/*sizes for header fields*/
#define H_NAME 100
#define H_MODE 8
#define H_UID 8
#define H_GID 8
#define H_SIZE 12
#define H_MTIME 12
#define H_CHKSUM 8
#define H_TYPE 1
#define H_LNAME 100
#define H_MAGIC 6
#define H_VERSION 2
#define H_UNAME 32
#define H_GNAME 32
#define H_DEVMA 8
#define H_DEVMI 8
#define H_PREFIX 155


typedef struct Header /*everything for header w/appropriate sizes*/
{
   char name[H_NAME];
   char mode[H_MODE];
   char uid[H_UID];
   char gid[H_GID];
   char size[H_SIZE];
   char mtime[H_MTIME];
   char chksum[H_CHKSUM];
   char typeflag[H_TYPE];
   char linkname[H_LNAME];
   char magic[H_MAGIC];
   char version[H_VERSION];
   char uname[H_UNAME];
   char gname[H_GNAME];
   char devmajor[H_DEVMA];
   char devminor[H_DEVMI];
   char prefix[H_PREFIX];
   char padding[12];
} Header;

typedef struct Tar
{
   char *files[MAX_FILES]; /*files given at command line*/
   Header *headers[MAX_FILES]; /*headers for the files*/
   int num_files; /*how many files*/
   int num_headers; /*how many headers*/
   char *tar_file; /*given name for tarfile, from f option*/
   int options[NUM_OPTS]; /*options array, ctxvSf*/
   int verbose; /*verbose option set*/
} Tar;

typedef struct Node /*create linked list for structure of given files*/
{
   int type; /*0 = file, 2 = sym, 5 = directory*/
   char pathname[H_PREFIX + H_NAME + 2]; /*full pathname, len of name and prefix*/
   struct Node *next; 
} Node;

void open_err(char, int);

void usage();

void terminate(Tar*);

void read_in(Tar*, int, char**);

void traverse_files(char*, Node*, char*);

#endif
