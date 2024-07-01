pkgname='xiterm'
pkgver='0.0'
pkgdesc='simple vte terminal emulator'
arch=('amd64')
license='MIT'
depends=('libgtk-3-0' 'libvte-2.91-0' 'libc6')
makedepends=(
	'libvte-2.91-dev'
	'libgtk-3-dev'
)
provides='x-terminal-emulator'

package() {
	make
	make DESTDIR="$pkgdir" install

	mkdir -p "$pkgdir/DEBIAN"
	printf '#!/bin/sh -e\nupdate-alternatives --install /usr/bin/x-terminal-emulator x-terminal-emulator /usr/bin/xiterm 50\n' > "$pkgdir/DEBIAN/postinst"
	printf '#!/bin/sh -e\nupdate-alternatives --remove x-terminal-emulator /usr/bin/xiterm\n' > "$pkgdir/DEBIAN/prerm"
	chmod +x "$pkgdir/DEBIAN/postinst" "$pkgdir/DEBIAN/prerm"
}
