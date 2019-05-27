#ifndef CREATE_H
#define CREATE_H

#include "mytar.h"

struct stat;

void create_header(Tar*, struct stat*, char*, char);

void create_file(Tar*, int, char*);

/*void create_dir(Tar*, DIR*, char*, int);*/

void create(Tar*, Node*);

unsigned int calc_chksum(struct Header*);

#endif
