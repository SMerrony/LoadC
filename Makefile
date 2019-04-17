CC = gcc
CFLAGS = -g -Wall -Wno-char-subscripts -ansi

adfl: main.o
	$(CC) $(CFLAGS) -o adfl main.o

main.o:	main.c aosvs_types.h aosvs_dump_fmt.h aosvs_fstat_packet.h
	$(CC) $(CFLAGS) -o main.o -c main.c

clean:
	rm -rf adfl.exe adfl *.o *.c~ *.h~

