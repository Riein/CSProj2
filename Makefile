COPTS = -std=c99 -g -D_GNU_SOURCE

backitup: backitup.o
	cc backitup.o -o backitup
backitup.o: backitup.c
	cc  $(COPTS) -c backitup.c
