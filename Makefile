CC = gcc
CFLAGS = -Wall -Werror -Iinclude -pthread
# CFLAGS = -Wall -Iinclude -pthread
SRCDIR = src
OBJDIR = obj

# list of source files
SOURCES = $(SRCDIR)/tap.c \
		  $(SRCDIR)/utils.c \
		  $(SRCDIR)/netdev.c \
		  $(SRCDIR)/pktbuf.c \
		  $(SRCDIR)/ethernet.c \
		  $(SRCDIR)/arp.c \
		  $(SRCDIR)/main.c \
		  $(SRCDIR)/ip.c \
		  $(SRCDIR)/ip_in.c \
		  $(SRCDIR)/ip_out.c \
		  $(SRCDIR)/icmp.c \

# convert source files to object files
OBJECTS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))

# main executable
EXECUTABLE = tenstack

# ensure obj directory exists
$(shell mkdir -p $(OBJDIR))

# default target
all: $(EXECUTABLE)

# Link everything together
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o $@

# compile each source file
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -rf $(OBJDIR) $(EXECUTABLE)

# run stack with sudo (for TAP dev access)
run: $(EXECUTABLE)
	sudo ./$(EXECUTABLE)


