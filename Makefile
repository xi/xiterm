PREFIX = /usr
CFLAGS = -std=c11 -pedantic -Wall -Wno-deprecated-declarations -fanalyzer -O2 `pkg-config --cflags gtk4 vte-2.91-gtk4`
LDFLAGS = -s `pkg-config --libs gtk4 vte-2.91-gtk4`

all: xiterm

xiterm: xiterm.c
	gcc $< -o $@ ${CFLAGS} ${LDFLAGS}

clean:
	rm -f xiterm

install: all
	install -D -m 755 xiterm ${DESTDIR}${PREFIX}/bin/xiterm
	install -D -m 644 xiterm.desktop ${DESTDIR}${PREFIX}/share/applications/xiterm.desktop

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/xiterm
	rm -f ${DESTDIR}${PREFIX}/share/applications/xiterm.desktop

.PHONY: all clean install uninstall
