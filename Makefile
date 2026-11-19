CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c11
TARGET = solver

all: $(TARGET)

$(TARGET): main.c solver.c solver.h
	$(CC) $(CFLAGS) -o $(TARGET) main.c solver.c

clean:
	rm -f $(TARGET) *.o