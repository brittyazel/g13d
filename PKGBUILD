# Maintainer: Britt W. Yazel <bwyazel@gmail.com>
# Maintainer: K. Hampf <khampf@users.sourceforge.net>
# Original maintainer: Lukas Sabota <lukas@lwsabota.com>

pkgbase="g13"
pkgname="g13-git"
pkgver=0.1.0
pkgrel=1
pkgdesc="Userspace driver for the Logitech G13 Keyboard"
arch=('x86_64' 'i686')
url="https://github.com/brittyazel/g13/"
license=('unknown')
depends=('log4cpp')
makedepends=('git' 'cmake')
source=("${pkgname}::git+https://github.com/brittyazel/g13.git")
sha256sums=('SKIP')

build() {
  #cd "${srcdir}/${pkgname}"
  export CFLAGS+=" ${CPPFLAGS}"
  export CXXFLAGS+=" ${CPPFLAGS}"
  cmake -B build -S "${srcdir}/${pkgname}"
  make -C build g13d pbm2lpbm
}

package() {
  cd build
  # binaries
  install -dm 755 "${pkgdir}"/usr/bin
  install -m 755 g13d "${pkgdir}"/usr/bin
  install -m 755 pbm2lpbm "${pkgdir}/usr/bin"

  cd "${srcdir}/${pkgname}"
  # configuration (location of default.bind)
  install -dm 755 "${pkgdir}"/etc/g13/
  install -m 644 bindfiles/default.bind "${pkgdir}"/etc/g13/default.bind
  
  # docs
  install -dm 755 "${pkgdir}"/usr/share/doc/${pkgname}
  install -m 644 README.md g13.png g13.svg "${pkgdir}"/usr/share/doc/${pkgname}
  install -dm 755 "${pkgdir}"/usr/share/doc/${pkgname}/examples
  install -dm 755 "${pkgdir}"/usr/share/doc/${pkgname}/examples/bitmaps
  install -m 644 bitmaps/*.lpbm "${pkgdir}"/usr/share/doc/${pkgname}/examples/bitmaps
  install -dm 755 "${pkgdir}"/usr/share/doc/${pkgname}/examples/bindfiles
  install -m 644 bindfiles/*.bind "${pkgdir}"/usr/share/doc/${pkgname}/examples/bindfiles

  # udev
  install -dm 755 "${pkgdir}"/usr/lib/udev/rules.d
  install -m 644 udev/91-g13.rules "${pkgdir}"/usr/lib/udev/rules.d/91-g13.rules
  
  # systemd
  install -dm 755 "${pkgdir}"/usr/lib/systemd/user
  install -m 644 systemd/g13.service "${pkgdir}"/usr/lib/systemd/user/g13.service
}
