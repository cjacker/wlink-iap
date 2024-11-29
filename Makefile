DESTDIR=

all:
	gcc -o wlink-iap main.c usb.c `pkg-config --libs --cflags libusb-1.0`

clean:
	rm -f wlink-iap
