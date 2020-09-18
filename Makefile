PREFIX = /usr
CFLAGS = -std=c99 -pedantic -Wall -Wno-deprecated-declarations -Os `pkg-config --cflags gtk+-3.0 vte-2.91`
LDFLAGS = `pkg-config --libs gtk+-3.0 vte-2.91`

all: xiterm

xiterm: xiterm.c
	gcc $< -o $@ ${CFLAGS} ${LDFLAGS}

clean:
	rm -f xiterm

install: all
	install -D -m 755 xiterm ${DESTDIR}${PREFIX}/bin/xiterm

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/xiterm

.PHONY: all clean install uninstall
