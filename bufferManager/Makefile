cc=clang
CFLAGS=-std=c99 -I.

SRC = dberror.h storage_mgr.h test_helper.h buffer_mgr_stat.h buffer_mgr.h 
OBJ = dberror.o storage_mgr.o buffer_mgr_stat.o buffer_mgr.o test_assign2_1.o

%.o: %.c $(SRC)
	$(CC) -c -o $@ $< $(CFLAGS)

all: test_assign2_1 

test_assign2_1: $(OBJ)
	cc -o  $@ $^ $(CFLAGS)
clean:
	rm -f $(OBJ)

