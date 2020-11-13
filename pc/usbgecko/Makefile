ifeq ($(OS),Windows_NT)
CC = i686-w64-mingw32-gcc
WIN-CC= i686-w64-mingw32-gcc
else
CC = gcc
WIN-CC= i686-w64-mingw32-gcc
endif

STRIP = strip
CFLAGS = -Wall -Wextra -Os -g -pipe
LFLAGS = -L ../-lFTD2XX
WIN-LFLAGS = -L. -lFTD2XX

SRC = gecko.c \
	main.c
OBJS = $(SRC:.c=.o) 

TARGET = swissserver


all: clean linux windows

clean:
	@rm -f *.o core core.* $(TARGET)*

linux: 
	$(CC) $(CFLAGS) -c $(SRC)
	$(CC) $(LFLAGS) $(OBJS) -o $(TARGET) $(LFLAGS)
	$(STRIP) -g $(TARGET)
	@rm -f *.o
	
windows:
	$(WIN-CC) $(CFLAGS) -c $(SRC)
	$(WIN-CC) $(WIN-LFLAGS) $(OBJS) -o $(TARGET).exe $(WIN-LFLAGS)
	$(STRIP) -g $(TARGET).exe
	@rm -f *.o