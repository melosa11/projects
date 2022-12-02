override CFLAGS += -std=gnu99 -Wall -Wextra -pedantic -D_GNU_SOURCE
override CPPFLAGS += -I ..

BINS = $(patsubst %.c,%.o,$(wildcard *.c))

all: $(BINS)

clean:
	$(RM) *.o


$(BINS): ../client.h ../server.h ../packet.h ../log.h ../settings.h ../utils.h \
	options.h run.h target.h setup.h

.PHONY: all clean
