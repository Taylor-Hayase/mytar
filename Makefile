CC = gcc
CFLAGS = -Wall -ansi -pedantic -g -lm
MAIN = mytar
OBJS = mytar.o create.o list.o extract.o
all : $(MAIN)

$(MAIN) : $(OBJS) mytar.h
	$(CC) $(CFLAGS) -o $(MAIN) $(OBJS)

mytar.o : mytar.c mytar.h
	$(CC) $(CFLAGS) -c mytar.c

create.o : create.c binary.c create.h
	$(CC) $(CFLAGS) -c create.c

list.o : list.c list.h
	$(CC) $(CFLAGS) -c list.c

extract.o : extract.c binary.c extract.h
	$(CC) $(CFLAGS) -c extract.c

clean: 
	rm *.o $(MAIN) 
