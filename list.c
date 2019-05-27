#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "list.h"
#include "create.h"
#include "mytar.h"

typedef struct
{
   int flag;
   int index;
   int value;
} ModeChar;

static ModeChar MODECHRS[] = {
   {S_IRUSR, 1, 'r'}, {S_IWUSR, 2, 'w'}, {S_IXUSR, 3, 'x'},
   {S_IRGRP, 4, 'r'}, {S_IWGRP, 5, 'w'}, {S_IXGRP, 6, 'x'},
   {S_IROTH, 7, 'r'}, {S_IWOTH, 8, 'w'}, {S_IXOTH, 9, 'x'},
   {S_ISUID, 3, 's'}, {S_ISGID, 6, 's'}, {S_IFDIR, 0, 'd'},
   {0, 0, 0}
};

int given_names(char *name, int argc, char **argv)
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

void list(Tar *tar, char **argv, int argc)
{

   Header header;
   ModeChar *mode_chr;
   int tar_fd, file_size, size, i, mode, file_shift, boo = 0;
   char *end;
   char mode_str[11];
   char time_str[20];
   char name_str[H_NAME + H_PREFIX + 1];
   time_t mtime;

   tar_fd = open(tar->tar_file, O_RDONLY);

   if (tar_fd < 0)
      fprintf(stderr, "Cannnot open tar file: %s\n", tar->tar_file);

   /*read in block by block*/
   while ((size = read(tar_fd, &header, BUF_SIZE)) != 0)
   {
      /*reset the stuff*/
      boo = 0;
      memset(name_str, '\0', H_NAME + H_PREFIX + 1);

      /*validate the tar header*/
      if (header.name[0] == '\0')
         break;

      header.magic[H_MAGIC - 1] = '\0';

      if (strcmp(MAGIC_NUM, header.magic) !=0)
      {
         fprintf(stderr, "Header Error invalid magic: '%s'\n", header.magic);
         return;
      }

      /*concat prefix and name if there is a prefix*/
      if (strlen(header.prefix) != 0)
      {
         strncpy(name_str, header.prefix, 155);
         
         if (name_str[strlen(name_str - 1)] != '/')
            strcat(name_str, "/");

         strcat(name_str, header.name);
      }
      else
         strncpy(name_str, header.name, 100);

      /*if listed files given, read each header to see if need to list*/
      if (argc > 3)
      {
         if (given_names(name_str, argc, argv) == 1)
            boo = 1;
      }
      else
         boo = 1; /*otherwise list all*/

      /*move file pointer to next header*/
      file_size = strtol(header.size, &end, 8);
      file_shift = (int)ceil((double)file_size/BUF_SIZE);
      lseek(tar_fd, file_shift * BUF_SIZE, SEEK_CUR);

      /*if need to print, begin doing so*/
      if (boo)
      {
         /*if verbose option selscted*/
         if (tar->options[V_FLAG])
         {
            mode = strtol(header.mode, &end, 8);

            for (i = 0; i < 10; i++)
               mode_str[i] = '-';

            mode_str[i] = 0;

            for (mode_chr = MODECHRS; mode_chr->flag; mode_chr++)
            {
               if (mode & mode_chr->flag)
                  mode_str[mode_chr->index] = mode_chr->value;
            }

            if (header.typeflag[0] == '5')
               mode_str[0] = 'd';
            else if (header.typeflag[0] == '2')
               mode_str[0] = 'l';

            mtime = strtol(header.mtime, &end, 8);
            strftime(time_str, 19, "%Y-%m-%d %H:%M", localtime(&mtime));
            time_str[strlen(time_str) ] = '\0';

            printf("%s %s/%s %14u %s ", mode_str, header.uname, header.gname, file_size, time_str);
         }
         /*else just print file name*/
         if (strlen(name_str) > 100 && strlen(name_str) < 110)
            name_str[H_NAME] = '\0';

         printf("%s\n", name_str);
      }

   }
}


