#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/sysmacros.h>
#include <dirent.h>

#include "mytar.h"
#include "create.h"
#include "list.h"
#include "extract.h"

#define MIN_ARGS 3
#define TAR_IND 2

int main(int argc, char **argv)
{

   Tar *tar = calloc(1, sizeof(Tar));
   int i, fd, tar_fd;
   Node *head = calloc(1, sizeof(Node));
   Node *ptr;
   char *path;/* = calloc(255, sizeof(char));*/
   struct stat *statbuf = calloc(1, sizeof(struct stat));

   /*read in the options*/
   read_in(tar, argc, argv);

   if (tar->options[T_FLAG])
   {
      if ((tar_fd = open(argv[TAR_IND], O_RDONLY)) < 0)
         usage();

      list(tar, argv, argc);

      free(tar->tar_file);
      free(tar);
      free(head);
      free(statbuf);
      return 0;
   }

   if (tar->options[X_FLAG])
   {
      if (open(argv[TAR_IND], O_RDONLY) < 0)
         usage();
      if (strstr(argv[TAR_IND], ".tar") == NULL)
         usage();

      extract(tar, argv, argc);

      free(statbuf);
      free(head);
      free(tar->tar_file);
      free(tar);
      return 0;
   }

   /*else is create option*/
   path = calloc(255, sizeof(char));

   if (argc > MIN_ARGS)
   {
      /*pathnames are given*/
      for (i = 0; i < argc - MIN_ARGS; i++)
      {
         strcpy(path, argv[MIN_ARGS + i]);

         if ((fd = open(path, O_RDONLY)) < 0)
         {
            fprintf(stderr, "File error: %s\n", path);
            path = getcwd(path, 255);
            continue;
         }
         traverse_files(path, head, NULL);
      }
   }

   ptr = head;
   lstat(head->pathname, statbuf);

   if (tar->options[C_FLAG])
   {
      create(tar, head);
   }

   while (head != NULL)
   {
      ptr = head;
      head = head->next;
      free(ptr);
   }

   free(tar->tar_file);
   free(tar->files[0]);
   for (i = 0; i < tar->num_headers; i++)
      free(tar->headers[i]);
   free(head);
   free(statbuf);
   free(path);
   
   free(tar);
   return 0;
}

void usage()
{
   fprintf(stderr, "Usage: mytar [ctxvS]f tarfile [ path [ ... ] ]\n");
   exit(EXIT_FAILURE);
}

void read_in(Tar *tar, int argc, char **argv)
{
   int i = 0, valid_opts = 0;
   char str[10];

   if (argc < MIN_ARGS)
      usage();

   strcpy(str, argv[1]);

   while (str[i] != '\0')
   {
      if (str[i] == 'c') /*create new tar*/
         tar->options[C_FLAG] = 1;
      else if (str[i] == 't') /*table of contents*/
         tar->options[T_FLAG] = 1;
      else if (str[i] == 'x') /*extract from new archive*/
         tar->options[X_FLAG] = 1;
      else if (str[i] == 'v')/*verbose*/
      {
         tar->options[V_FLAG] = 1;
         tar->verbose = 1;
      }
      else if (str[i] == 'S') /*strict*/
         tar->options[S_FLAG] = 1;
      else if (str[i] == 'f') /*output filename*/
         tar->options[F_FLAG] = 1;
      else
         usage();

      i++;
   }

   if (tar->options[F_FLAG])
   {
      tar->tar_file = calloc(strlen(argv[TAR_IND]) + 1, sizeof(char));
      strcpy(tar->tar_file, argv[TAR_IND]);
   }
   else
      usage();

   if (tar->tar_file == NULL)
      usage();

   for (i = 0; i < 3; i ++) /*check for a c, t, or x*/
   {
      if (tar->options[i])
         valid_opts++;
   }

   if (valid_opts != 1) /*need only one c, t or x*/
      usage();

   if (tar->options[C_FLAG]) /*if given c_flag, then need files to be tar'ed*/
   {
      for (i = TAR_IND + 1; i < argc; i++)
      {
         tar->files[tar->num_files] = calloc(strlen(argv[i]) + 1, sizeof(char));
         strcpy(tar->files[tar->num_files], argv[i]);
         tar->num_files++;
      }
   }
   /*given tar file*/
   strcpy(tar->tar_file, argv[TAR_IND]);
}

void traverse_files(char *file, Node *head, char *path)
{
   Node *ptr, *new;
   struct stat *buff = calloc(1, sizeof(struct stat));
   DIR *dir;
   struct dirent *d_files;
   char *full_name = calloc(255, sizeof(char));
  
   if (path != NULL)
      strcat(full_name, path);

   strcat(full_name, file);

   if(lstat(full_name, buff) != 0)
      fprintf(stderr, "STAT FAIL\n");

   ptr = head;

   if (strlen(head->pathname) == 0)
   {
      if (S_ISREG(buff->st_mode))
         head->type = 0;
      else if (S_ISDIR(buff->st_mode))
         head->type = 5;
      else if (S_ISLNK(buff->st_mode))
         head->type = 2;
      else
         fprintf(stderr, "Unsupported file type\n");

      strncpy(head->pathname, full_name, 255);
      head->pathname[256] = '\0';
      head->next = NULL;

      ptr = head;
   }
   else
   {
      new = calloc(1, sizeof(Node));

      if (S_ISREG(buff->st_mode))
         new->type = 0;
      else if (S_ISDIR(buff->st_mode))
         new->type = 5;
      else if (S_ISLNK(buff->st_mode))
         new->type = 2;
      else
         fprintf(stderr, "File Error\n");

      strncpy(new->pathname, full_name, 255);

      if (full_name[strlen(full_name) - 1] != '.' && new->type == 5)
         strcat(new->pathname, "/");

      new->pathname[256] = '\0';
      
      while (ptr->next != NULL)
         ptr = ptr->next;

      ptr->next = new;
      ptr = new;
   }

   /*recursively call on directories*/
   if (S_ISDIR(buff->st_mode) && (strcmp(file, ".") != 0) && (strcmp(file, "..") != 0))
   {
      dir = opendir(full_name);
      if (dir)
      {
         while((d_files = readdir(dir)) != NULL)
         {
            if (full_name[strlen(full_name) - 1] != '/')
               strcat(full_name, "/");
            traverse_files(d_files->d_name, head, full_name);
         }
         closedir(dir);
      }
   }
   free(buff);
   free(full_name);

}
