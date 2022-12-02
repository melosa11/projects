BINS = utils client server packet main generic_list event_reader

all: build

build: $(BINS)

$(BINS):
	$(MAKE) --directory=$@

clean:
	$(RM) */*.o

distclean: clean

.PHONY: all clean distclean $(BINS)
