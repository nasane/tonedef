tonedef: tonedef.c
	cc -fPIC -std=c99 --shared -o libtonedef.so tonedef.c
clean:
	rm -rf libtonedef.so