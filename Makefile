PREFIX = /usr
CFLAGS = -std=c99 -Wall -O2 -fstack-protector-strong -Wformat -Werror=format-security -D_FORTIFY_SOURCE=2 `pkg-config --cflags gtk+-3.0 vte-2.91`
LDFLAGS = -s -Wl,-z,relro `pkg-config --libs gtk+-3.0 vte-2.91`

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
