CC = gcc
CFLAGS = -fsanitize=address -fno-omit-frame-pointer -lsqlite3 -O0 -g -Wall -std=gnu11
TARGET = tict
OBJ = tict.o

$(TARGET): $(OBJ)
	$(CC) $^ $(CFLAGS) -o $@

.PHONY: clean
clean:
	-rm -f *.o *.gc* $(TARGET)
