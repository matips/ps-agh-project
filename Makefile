CC = gcc
CFLAGS = -Wall -g
DEPS = context.h
OBJ = context.o

hello: $(OBJ) hello.c
	$(CC) $(CFLAGS) hello.c `pkg-config fuse --cflags --libs` -o  $@ -I. $(OBJ)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

