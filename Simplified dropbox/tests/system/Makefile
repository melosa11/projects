all: test

test: build
	bash main.sh

build:
	$(MAKE) -C ../.. test_build

clean:
	$(RM) -r data/*

distclean: clean

.PHONY: test build clean distclean
