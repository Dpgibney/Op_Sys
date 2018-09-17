CC=gcc
CFLAGS=-I. -ansi -pedantic -Wall
TARGET = project1

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

clean:
	$(RM) $(TARGET)

