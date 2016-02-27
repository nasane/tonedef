tonedef: tonedef.c
	cc -fPIC -std=c99 --shared -o libtonedef.so tonedef.c -fprofile-arcs -ftest-coverage -lm
tests: test.c
	cc -std=c99 -o test test.c -L. -ltonedef
clean:
	rm -rf libtonedef.so test
