override CFLAGS += -std=gnu99 -Wall -Wextra -pedantic -D_GNU_SOURCE
override CPPFLAGS += -I ..

BINS = $(patsubst %.c,%.o,$(wildcard *.c))

all: $(BINS)

clean:
	$(RM) *.o

$(BINS): ../client.h ../utils.h ../log.h ../packet.h ../generic_list.h \
	../settings.h traverse_data.h copy.h event.h helper.h send.h watcher.h \
	watcher_list.h

.PHONY: all clean
