# Maintainer: Britt W. Yazel <bwyazel@gmail.com>
# Maintainer: K. Hampf <khampf@users.sourceforge.net>
# Original maintainer: Lukas Sabota <lukas@lwsabota.com>

pkgbase="g13d"
pkgname="g13-git"
pkgver=0.1.0
pkgrel=1
pkgdesc="Userspace driver for the Logitech G13 Keyboard"
arch=('x86_64' 'i686')
url="https://github.com/brittyazel/g13d/"
license=('MIT')
depends=('log4cpp')
makedepends=('git' 'cmake')
source=("git+https://github.com/brittyazel/g13d.git")
sha256sums=('SKIP')

build() {
    export CFLAGS+=" ${CPPFLAGS}"
    export CXXFLAGS+=" ${CPPFLAGS}"
    cmake -S "${pkgbase}"
    make -C . g13d pbm2lpbm
}

package() {
    # binaries
    install -dm 755 "${pkgdir}"/usr/bin/
    install -m 755 g13d "${pkgdir}"/usr/bin/
    install -m 755 pbm2lpbm "${pkgdir}"/usr/bin/

    # host
    cp -r "${pkgbase}/assets/host/." "${pkgdir}/"

}