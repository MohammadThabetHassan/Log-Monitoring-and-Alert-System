CC = gcc
CFLAGS = -Wall -O2
TARGET = logParser

all: $(TARGET)

$(TARGET): logParser.c
	$(CC) $(CFLAGS) -o $(TARGET) logParser.c

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean