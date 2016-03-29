ALL_SRC := $(wildcard *.c)
SRC     := $(filter-out test.c, $(ALL_SRC))

tonedef: $(SRC)
	cc -fPIC -std=c99 --shared -o libtonedef.so $(SRC) -fprofile-arcs -ftest-coverage -lm -lsndfile -lfftw3 -Werror -Wunused-variable -DTESTING
tests: test.c
	cc -std=c99 -o test test.c libtonedef.so -lm -lfftw3 -Werror -Wunused-variable
clean:
	rm -rf libtonedef.so test ./*.gcno ./*.gcov ./*.gcda
