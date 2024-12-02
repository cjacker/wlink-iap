DESTDIR=

all:
	gcc -o wlink-iap main.c usb.c `pkg-config --libs --cflags libusb-1.0`

clean:
	rm -f wlink-iap

install: all
	mkdir -p $(DESTDIR)/usr/bin
	install -m0755 wlink-iap $(DESTDIR)/usr/bin
