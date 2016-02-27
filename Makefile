tonedef: tonedef.c
	clang --shared -o libtonedef.so tonedef.c
clean:
	rm -rf libtonedef.so