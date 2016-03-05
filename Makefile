tonedef: tonedef.c
	cc -fPIC -std=c99 --shared -o libtonedef.so tonedef.c -fprofile-arcs -ftest-coverage -lm -lsndfile -lfftw3
tests: test.c
	cc -std=c99 -o test test.c libtonedef.so -lm
clean:
	rm -rf libtonedef.so test ./*.gcno ./*.gcov ./*.gcda
