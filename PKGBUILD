pkgname='xiterm'
pkgver='0.0'
pkgdesc='simple vte terminal emulator'
arch=('amd64')
license='MIT'
depends=('libgtk-3-0' 'libvte-2.91-0' 'libc6')
provides='x-terminal-emulator'

package() {
	make
	make DESTDIR="$pkgdir" install
}
