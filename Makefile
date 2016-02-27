tonedef: tonedef.c
	cc -fPIC -std=c99 --shared -o libtonedef.so tonedef.c -fprofile-arcs -ftest-coverage
tests: test.c
	cc -std=c99 -o test test.c libtonedef.so
clean:
	rm -rf libtonedef.so test
