CXXFLAGS = -W -Wall -O2

all: test

clean:
	rm -f sds011_test

test: sds011_test
	./sds011_test

