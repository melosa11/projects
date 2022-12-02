TARGET = dropbox

all: build

test_build:
	$(MAKE) build CFLAGS=-DDROPBOX_TESTS

build: build_bins
	$(MAKE) $(TARGET)

build_bins:
	$(MAKE) -C src

$(TARGET): $(wildcard src/*/*.o)
	$(CC) $(LDFLAGS) $^ -o build/$@ $(LDLIBS)

tests:
	$(MAKE) -C tests

clean:
	$(MAKE) -C src clean
	$(MAKE) -C tests clean

distclean:
	$(MAKE) -C src distclean
	$(MAKE) -C tests distclean

.PHONY: all tests clean distclean build build_bins build_folder
