CFLAGS=-std=c11 -g -static 
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o) #https://orumin.blogspot.com/2019/12/makefile.html

takuya: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) 

$(OBJS): takuya.h

test: takuya
		./test.sh

clean:
		rm -f chibicc *.o *~ tmp*

.PHONY: test clean

run:
		docker-compose run  --rm main bash 
debug:
		gdb ./takuya core
		