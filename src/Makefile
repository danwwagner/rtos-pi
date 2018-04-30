EXECUTABLE := project

CC := /usr/bin/gcc
XENO_CONFIG := /usr/xenomai/bin/xeno-config

CFLAGS :=  $(shell $(XENO_CONFIG)   --posix --alchemy --cflags)
LDFLAGS :=  $(shell $(XENO_CONFIG)  --posix --alchemy --ldflags)

all: $(EXECUTABLE)

%: %.c
	$(CC) -o $@ $< $(CFLAGS) $(LDFLAGS) -lwiringPi

clean:
	rm -f $(EXECUTABLE)

run: $(EXECUTABLE)
	./$(EXECUTABLE)

