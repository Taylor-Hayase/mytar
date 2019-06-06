# mytar
Simple C implementation of tar - written in collaboration with Ryan Premi

Complie with make

Usage: mytar [ctxv]f tarfile [path [...]]

Create (c): 
  This will create a new tar file, whose name is specified in the tarfile field. 
  The addition of v (verbose) will list the files as they are added to the tar file.
  
Table (t):
  This will list out all entries in the tar file given.
  If the verbose option is added, more in-depth information will be given;
  If specified files are given, then the table option will only list the files given, 
  and any decendants.
  
Extract (x):
  This will extract all files are directories from the given tar file.
  If any specified files are given, then extract will only extract those files and any directories needed to access those files.
  
This version of mytar only supports regular files, directories, and sym links
A tar file name will always need to be given, and a full pathname must be provided (for table and extract)
