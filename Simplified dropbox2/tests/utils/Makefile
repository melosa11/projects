TARGET = test_utils
DEPS = utils

VALGRIND = valgrind --leak-check=full --error-exitcode=1 --track-origins=yes

SRC = ../src/
override CFLAGS += -std=c99 -Wall -Wextra -pedantic -D_GNU_SOURCE
override CPPFLAGS += -I ..
override CPPFLAGS += -I $(SRC)

BINS = $(patsubst %.c,%.o,$(wildcard *.c))

SRC_DEPS = $(addprefix $(SRC), $(DEPS))

all:$(SRC_DEPS) $(TARGET) .gitignore

$(TARGET): $(BINS) $(addprefix $(SRC), $(DEPS:%=%/*.o))

test: all
	$(VALGRIND) ./$(TARGET)

.gitignore:
	echo $(TARGET) > $@

$(SRC_DEPS): 
	$(MAKE) --directory=$@

clean:
	$(RM) *.o
distclean: clean
	$(RM) $(TARGET)

.PHONY: all $(SRC_DEPS) distclean clean test ignore