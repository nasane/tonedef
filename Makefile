tonedef: tonedef.c
	cc -fPIC --shared -o libtonedef.so tonedef.c
clean:
	rm -rf libtonedef.so