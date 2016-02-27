tonedef: tonedef.c
	clang -fPIC --shared -o libtonedef.so tonedef.c
clean:
	rm -rf libtonedef.so