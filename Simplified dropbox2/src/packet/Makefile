override CFLAGS += -std=c11 -Wall -Wextra -pedantic -D_GNU_SOURCE
override CPPFLAGS += -I ..

BINS = $(patsubst %.c,%.o,$(wildcard *.c))

all: $(BINS)

clean:
	$(RM) *.o

$(BINS) : ../utils.h ../packet.h net.h

.PHONY: all clean
