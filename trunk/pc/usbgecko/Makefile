ifeq ($(WIN32), 1)
PREFIX = mingw32-
BIN_EXT = .exe
else
PREFIX =
BIN_EXT =
endif

CC = $(PREFIX)gcc
STRIP = strip

CFLAGS = -Wall -Wextra -Os -g -pipe
LFLAGS = -L. -lFTD2XX

BIN = swissserver$(BIN_EXT)

OBJS = \
	gecko.o \
	main.o

all: $(BIN)

clean:
	@rm -f *.o core core.* $(BIN)

strip:
	$(STRIP) -g $(BIN)

$(BIN): $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o $@ $(LFLAGS)

%o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

