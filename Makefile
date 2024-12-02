DESTDIR=

all:
	make -C src
clean:
	make clean -C src
install:
	make install -C src DESTDIR=$(DESTDIR)
