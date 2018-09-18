CC=gcc
CFLAGS=-I. -pedantic -Wall
TARGET = project1

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c

clean:
	$(RM) $(TARGET)

