#define _BSD_SOURCE
#define SIX_C_SOURCE >= 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <utime.h>
#include <dirent.h>

#include "list.h"
#include "create.h"
#include "mytar.h"


int named(char *name, int argc, char **argv)
{
   int i;
   int found = 0;

   for (i = 3; i < argc; i++)
   {
      if (strcmp(name, argv[i]) == 0)
      {
         found = 1;
         break;
      }
      else if ((argv[i][strlen(argv[i]) - 1] == '/') && (strstr(name, argv[i]) != NULL))
      {
         found = 1;
         break;
      }
   }
   return found;
}


void extract(Tar *tar, char **argv, int argc)
{

   Header header;
   int tar_fd, fd, file_size, size, i, mode, file_blocks, boo = 0;
   DIR *dir = NULL;
   char *end;
   char buffer[100];
   char block_buf[BUF_SIZE];
   char name_str[H_NAME + H_PREFIX + 2];
   time_t mtime;
   struct stat sb;
   struct utimbuf u_time;

   tar_fd = open(tar->tar_file, O_RDONLY);

   if (tar_fd < 0)
      fprintf(stderr, "failed to open tar file\n");;

   /*read in blocks of 512 bytes*/
   while ((size = read(tar_fd, &header, BUF_SIZE)) != 0)
   {
      /*reset the stuff*/
      boo = 0;
      memset(name_str, '\0', H_NAME + H_PREFIX + 1);
      mode = 0;
      mtime = 0;

      /*validate header*/
      if (header.name[0] == '\0')
         break;

      header.magic[H_MAGIC - 1] = '\0';

      if (strcmp(MAGIC_NUM, header.magic) !=0)
      {
         fprintf(stderr, "Header Error invalid magic: '%s'\n", header.magic);
         return;
      }

      /*if prefix present, attach it*/
      if (strlen(header.prefix) != 0)
      {
         strncpy(name_str, header.prefix, 155);
         
         if (name_str[strlen(name_str - 1)] != '/')
            strcat(name_str, "/");

         strcat(name_str, header.name);
      }
      else
         strncpy(name_str, header.name, 100);

      mode = strtol(header.mode, &end, 8);

      mtime = strtol(header.mtime, &end, 8);

      /*if given specific files, only extract those*/
      if (argc > 3)
      {
         if (named(name_str, argc, argv) == 1)
            boo = 1;
      }
      else
         boo = 1; /*otherwise extract all*/

      file_size = strtol(header.size, &end, 8);
      file_blocks = (int)ceil((double)file_size/BUF_SIZE);

      if (boo)
      {
         /*check if all directories are present if given a specific file*/
         for (i = 0; i < strlen(name_str); i++)
         {
            if (name_str[i] == '/')
            {
               strncpy(buffer, name_str, i + 1);
               buffer[i+1] = '\0';
               if ((dir = opendir((buffer)))  == NULL)
               {
                  if (mkdir(buffer, 0777) != 0)
                     fprintf(stderr, "failed to make dir %s \n", buffer);
               }
            }
         }
         /*either open current directory*/
         if (name_str[strlen(name_str) - 1] == '/')
         {
            if (mkdir(name_str, (S_IRWXU | S_IRWXG | S_IRWXO)) != 0)
               dir = opendir(name_str);
         }
         /*or add a symlink*/
         else if (header.typeflag[0] == '2')
            symlink(header.linkname, name_str);
         /*or create a file*/
         else
         {
            fd = open(name_str, O_RDWR | O_CREAT, (S_IRWXU | S_IRWXG | S_IRWXO));

            if(fd ==-1)
               fprintf(stderr, "open/create failed\n");
            if (stat(name_str, &sb) != 0)
               fprintf(stderr, "header stat failed: name_str = %s\n",name_str);

            u_time.modtime = mtime;
            u_time.actime = sb.st_atime;
         
            /*restore mod time*/
            if (utime(name_str, &u_time) != 0)
               fprintf(stderr, "utime change failed\n");

            /*read in each file and header and create new file*/
            i = 0;
            while (file_blocks > 0 || file_size > 0)
            {
               if (read(tar_fd, block_buf, BUF_SIZE) != BUF_SIZE)
                  fprintf(stderr, "readin from file failed\n");

               if (file_size < 512)
               {
                  if (write(fd, block_buf, file_size) != file_size)
                     fprintf(stderr, "write to file failed size: %d\n", file_size);
                  file_size = file_size - file_size;
               }
               else
               {
                  if (write(fd, block_buf, BUF_SIZE) != BUF_SIZE)
                     fprintf(stderr, "write to file failed size: 512\n");
                  file_size = file_size - BUF_SIZE;
               }

               file_blocks--;
               i++;
            }
            /*restore permissions*/
            chmod(name_str, mode & (S_IRWXU | S_IRWXG | S_IRWXO));
            close(fd);
         }
      }
      else /*go to next header if not a listed file, and not a extract all*/
         lseek(tar_fd, file_blocks * BUF_SIZE, SEEK_CUR);

   }

}
