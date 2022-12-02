TESTS = packet utils server generic_list system

all: $(TESTS)

$(TESTS):
	$(MAKE) --directory=$@ test

clean:
	for T in $(TESTS);                 \
	do								   \
		$(MAKE) --directory=$$T clean; \
	done

distclean: clean
	for T in $(TESTS);                     \
	do								       \
		$(MAKE) --directory=$$T distclean; \
	done

.PHONY: all clean distclean system_test $(TESTS)
