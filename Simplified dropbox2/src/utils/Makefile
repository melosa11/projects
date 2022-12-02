override CFLAGS += -std=c99 -Wall -Wextra -pedantic -D_GNU_SOURCE
override CPPFLAGS += -I ..

BINS = $(patsubst %.c,%.o,$(wildcard *.c))

all: $(BINS)

clean:
	$(RM) *.o

$(BINS): ../utils.h

.PHONY: all clean
