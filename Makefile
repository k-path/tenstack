CC = gcc
CFLAGS = -Wall -Iinclude
SRCDIR = src

tap: $(SRCDIR)/tap.c $(SRCDIR)/utils.c 
	$(CC) $(CFLAGS) $^ -o $@   
  # gcc -Wall -Iinclude src/tap.c -o tap

clean:
	rm -f tap *.o
