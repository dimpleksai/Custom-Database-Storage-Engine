### Group 15 makefile ###
CC = gcc
CFLAGS = -Wall -Wextra -g

# C source files
SOURCES = storage_mgr.c dberror.c test_assign1_1.c
# header files
HEADERS = storage_mgr.h dberror.h test_helper.h
# The corresponding object files of above 
OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = test_assign1

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(EXECUTABLE)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

testCode: $(EXECUTABLE)
	./$(EXECUTABLE)

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)

### Group 15 makefile ###