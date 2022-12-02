override CFLAGS += -std=c99 -Wall -Wextra -pedantic -D_GNU_SOURCE
override CPPFLAGS += -I ..

BINS = $(patsubst %.c,%.o,$(wildcard *.c))

all: $(BINS)

clean:
	$(RM) *.o

$(BINS): ../server.h ../packet.h ../settings.h ../log.h ../utils.h command.h \
	extract.h file_info.h operation.h set.h

.PHONY: all clean
