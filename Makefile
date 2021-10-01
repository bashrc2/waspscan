APP=waspscan
VERSION=0.01
RELEASE=1
ARCH_TYPE=`uname -m`
.PHONY: check-syntax

all:
	gcc -Wall -std=gnu99 -pedantic -O3 -o ${APP} src/*.c -Isrc -lm -fopenmp
check-syntax:
	gcc -Wall -std=gnu99 -pedantic -O3 -o ${APP} src/*.c -Isrc -lm -fopenmp -fsyntax-only
debug:
	gcc -Wall -std=gnu99 -pedantic -g -o ${APP} src/*.c -Isrc -lm -fopenmp
source:
	tar -cvzf ../${APP}_${VERSION}.orig.tar.gz ../${APP}-${VERSION} --exclude-vcs
install:
	mkdir -p ${DESTDIR}/usr
	mkdir -p ${DESTDIR}/usr/bin
	install -m 755 --strip ${APP} ${DESTDIR}/usr/bin
	install -m 755 ${APP}dir ${DESTDIR}/usr/bin
	install -m 755 fits2tbl ${DESTDIR}/usr/bin
	install -m 755 waspd ${DESTDIR}/usr/bin
	install -m 755 startwaspd ${DESTDIR}/usr/bin
	mkdir -m 755 -p ${DESTDIR}/usr/share
	mkdir -m 755 -p ${DESTDIR}/usr/share/man
	mkdir -m 755 -p ${DESTDIR}/usr/share/man/man1
	install -m 644 man/${APP}.1.gz ${DESTDIR}/usr/share/man/man1
clean:
	rm -f ${APP} \#* \.#* gnuplot* *.png debian/*.substvars debian/*.log
	rm -fr deb.* debian/$(APP) rpmpackage/${ARCH_TYPE}
	rm -f ../${APP}*.deb ../${APP}*.changes ../${APP}*.asc ../${APP}*.dsc
	rm -f rpmpackage/*.src.rpm archpackage/*.gz puppypackage/*.gz puppypackage/*.pet
