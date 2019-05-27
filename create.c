#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <grp.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/sysmacros.h>
#include <limits.h>
#include <ctype.h>

#include "create.h"
#include "mytar.h"
#include "binary.c"

void create(Tar *tar, Node *head)
{
   int fd_tar = 0;
   Node *ptr;
   struct stat stat_buf[sizeof(struct stat) + 1];
   char arr[512];

   fd_tar = open(tar->tar_file, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0666);

   ptr = head;
   while (ptr != NULL)
   {
      if (ptr->type == 0)
      {
         create_file(tar, fd_tar, ptr->pathname);
      }
      else if (ptr->type == 5)
      {
         if (ptr->pathname[strlen(ptr->pathname) - 1] != '.')
         {
            lstat(ptr->pathname, stat_buf);
            create_header(tar, stat_buf, ptr->pathname, '5');
            write(fd_tar, tar->headers[tar->num_headers], BUF_SIZE);
            tar->num_headers++;
         }
      }
      else if (ptr->type == 2)
      {
         lstat(ptr->pathname, stat_buf);
         create_header(tar, stat_buf, ptr->pathname, '2');
         write(fd_tar, tar->headers[tar->num_headers], BUF_SIZE);
         tar->num_headers++;
      }
      else
         fprintf(stderr, "unsupported file type\n");

      ptr = ptr->next;
   }
   /*end of archive need two empty blocks*/
   memset(arr, '\0', BUF_SIZE);
   write(fd_tar, arr, BUF_SIZE);
   write(fd_tar, arr, BUF_SIZE);

   
}

void create_file(Tar *tar, int fd_tar, char *name)
{
   int fd_file, size;
   struct stat f_stat[sizeof(struct stat) + 1];
   char buf[BUF_SIZE];
   /*create empty block of 512 bytes for file*/
   memset(buf, '\0', BUF_SIZE);

   fd_file = open(name, O_RDONLY);

   if (fd_file < 0)
   {
      fprintf(stderr, "file open error: %s\n", name);
      return;
   }

   if (fstat(fd_file, f_stat) != 0)
   {
      fprintf(stderr, "file stat error\n");
      return;
   }

   /*create a file header*/
   create_header(tar, f_stat, name, '0');

   if(write(fd_tar, tar->headers[tar->num_headers], BUF_SIZE) != BUF_SIZE)
      fprintf(stderr, "Block of 512 bytes not written\n");

   tar->num_headers++;

   /*write file in blocks of 512*/
   while ((size = read(fd_file, buf, BUF_SIZE)) != 0)
      if(write(fd_tar, buf, BUF_SIZE) != BUF_SIZE)
         fprintf(stderr, "Block of 512 not written\n");

   /*if verbose option, print files as added*/
   if (tar->options[V_FLAG] == 1)
      printf("%s\n", name);

   close(fd_file);

}

void create_header(Tar *tar, struct stat *st, char *name, char type)
{
   Header *head = tar->headers[tar->num_headers] = calloc(1, BUF_SIZE);
   struct passwd *user;
   struct group *group;
   char buf[101], pre[157], post[102];
   int i = 0, x = 0;

   memset(pre, '\0', 157);
   memset(post, '\0', 102);

   /*for header, if name is longer than 100, place first part in prefix
    * go backwards from the end and look for slash or begining*/
   if (strlen(name) > 100)
   {
      for (i = strlen(name) - 1; i > 0; i--)
      {
         if (name[i] == '/' && i != (strlen(name) - 1))
            break;
      }
      /*i is now at the index before the name*/
      if (i != 0)
      {
         /*if there is no prefix, don't come in here*/
         while (x < i)
         {
            pre[x] = name[x];
            x++;
         }
         strcat(pre, "\0");
      }

      /*post is the name of the file only, if no prefix*/
      x = 0;
      i++;
      while (i < strlen(name))
      {
         post[x] = name[i];
         x++;
         i++;
      }
      strcat(post, "\0");
   }
   else
      strcpy(post, name);
   /*prefix may be an empty string*/

   strncpy(head->prefix, pre, H_PREFIX);
   /*post will always have something*/
   strncpy(head->name, post, H_NAME);

   if (octal_in(head->mode, H_MODE, (st->st_mode & (S_IRWXU | S_IRWXG | S_IRWXO))) != 0)
      usage(); /*set permissions*/

   if (octal_in(head->uid, H_UID, st->st_uid) != 0 )
      fprintf(stderr, "uid write to header failed\n");

   if (octal_in(head->gid, H_GID, st->st_gid) != 0)
      fprintf(stderr, "gid write to header failed\n"); /*keep group id*/

   /*specify header size*/
   if (type == '5' || type == '2') 
      sprintf(head->size, "%011o",  0);
   else
      if (octal_in(head->size, H_SIZE, (unsigned int)st->st_size) != 0)
         fprintf(stderr, "size write to header failed\n");

   if (octal_in(head->mtime, H_MTIME, st->st_mtime) != 0)
      fprintf(stderr, "mtime write to header failed\n"); /*mod time*/

   if (type == '5') /*typeflag for dir*/
      *(head->typeflag) = '5';
   else if (type == '2') /*typeflag for sym link*/
      *(head->typeflag) = '2';
   else if (type == '0') /*typeflag for regular files*/
      *(head->typeflag) = '0';

   if (*(head->typeflag) == '2') /*if sym link, get value of link*/
   {
      /*get just the file name still have issues*/
      readlink(name, buf, 100);
      strncpy(head->linkname, buf, H_LNAME);
      head->linkname[H_LNAME - 1] = '\0';
   }

   strcpy(head->magic, MAGIC_NUM); /*magic number, null terminated*/
   head->magic[H_MAGIC - 1] = '\0';

   *(head->version) = '0'; /*version is '00'*/
   *(head->version + 1) = '0';

   user = getpwuid(getuid());
   strncpy(head->uname, user->pw_name, 31);/*user name, null terminater*/
   head->uname[H_UNAME - 1] = '\0';

   group = getgrgid(getgid());
   strncpy(head->gname, group->gr_name, 31);/*group name, null terminated*/
   head->gname[H_GNAME - 1] = '\0';

  /* sprintf(head->devmajor, "%7o", major(st->st_dev));
   head->devmajor[H_DEVMA - 1] = '\0';

   sprintf(head->devminor, "%7o", minor(st->st_dev));
   head->devminor[H_DEVMI - 1] = '\0';*/

   /*chksum, sum of all characters*/
   if (octal_in(head->chksum, H_CHKSUM, calc_chksum(head)) != 0)
      fprintf(stderr, "chksum write to header failed\n");
}

unsigned int calc_chksum(struct Header *header)
{
   /*add th eascii value of all characers in header,
    * replace chksum part with spaces*/
   unsigned int result;
   unsigned int i = 0;
   result = 0;

   while (i < H_NAME)
   {
      result = result + (unsigned char)header->name[i];
      i++;
   }

   i = 0;
   while (i < H_MODE)
   {
      result = result + (unsigned char)header->mode[i];
      i++;
   }

   i = 0;
   while (i < H_UID)
   {
      result = result + (unsigned char)header->uid[i];
      i++;
   }

   i = 0;
   while (i < H_GID)
   {
      result = result + (unsigned char)header->gid[i];
      i++;
   }

   i = 0;
   while (i < H_SIZE)
   {
      result = result + (unsigned char)header->size[i];
      i++;
   }

   i = 0;
   while (i < H_MTIME)
   {
      result = result + (unsigned char)header->mtime[i];
      i++;
   }

   result = result + (unsigned int)header->typeflag[0]; 

   i = 0;
   while (i < H_LNAME)
   {
      result = result + (unsigned char)header->linkname[i];
      i++;
   }

   i = 0;
   while (i < H_MAGIC)
   {
      result = result + (unsigned char)header->magic[i];
      i++;
   }

   result = result + (unsigned int)header->version[0];
   result = result + (unsigned int)header->version[1];

   i = 0;
   while (i < H_UNAME)
   {
      result = result + (unsigned char)header->uname[i];
      i++;
   }

   i = 0;
   while (i < H_GNAME)
   {
      result = result + (unsigned char)header->gname[i];
      i++;
   }

  /* i = 0;
   while (i < H_DEVMA)
   {
      result = result + (unsigned char)header->devmajor[i];
      i++;
   }

   i = 0;
   while (i < H_DEVMI)
   {
      result = result + (unsigned char)header->devminor[i];
      i++;
   }
*/
   i = 0;
   while (i < H_PREFIX)
   {
      result = result + (unsigned char)header->prefix[i];
      i++;
   }

   result += 32 * 8;

   return result;
}
