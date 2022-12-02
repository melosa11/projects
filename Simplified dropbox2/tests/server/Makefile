TARGET = test_server
DEPS = server packet utils generic_list event_reader

VALGRIND = valgrind --leak-check=full --error-exitcode=1 --trace-children=yes --track-origins=yes

SRC = ../src/
override CFLAGS += -g -ggdb3 -std=c99 -Wall -Wextra -pedantic -pthread -DCUT_NO_FORK -D_GNU_SOURCE
override LDLIBS += -pthread
override CPPFLAGS += -I ..
override CPPFLAGS += -I $(SRC)

BINS = $(patsubst %.c,%.o,$(wildcard *.c))

SRC_DEPS = $(addprefix $(SRC), $(DEPS))

all: $(SRC_DEPS) $(TARGET)

$(TARGET): $(BINS) $(wildcard $(addprefix $(SRC), $(DEPS:%=%/*.o)))

test: all
	$(VALGRIND) ./$(TARGET)

test_cap: all
	sudo setcap 'cap_chown+ep' ./test_server
	./$(TARGET)

$(SRC_DEPS): 
	$(MAKE) --directory=$@

clean:
	$(RM) *.o
distclean: clean
	$(RM) $(TARGET)
	$(RM) -R `ls -1 -d */`

.PHONY: all $(SRC_DEPS) distclean clean test test_cap
