# Created by: Emanuel Haupt <ehaupt@critical.ch>

PORTNAME=	mp3guessenc
PORTVERSION=	0.27.5
CATEGORIES=	audio
MASTER_SITES=	SF/${PORTNAME}/${PORTNAME}-${PORTVERSION:C/\.[0-9]*$//}

MAINTAINER=	ehaupt@FreeBSD.org
COMMENT=	Utility for guessing which encoder was used to encode an mp3 file

LICENSE=	LGPL21
LICENSE_FILE=	${WRKSRC}/COPYING

PLIST_FILES=	bin/mp3guessenc
SRC=		decode.c mp3guessenc.c bit_utils.c tags.c

USE_HARDENING=	pie:off relro:off

do-build:
.for f in ${SRC}
	${CC} ${CFLAGS} ${WRKSRC}/${f} -c -o ${WRKSRC}/${f:C/\.c/.o/}
.endfor
	${CC} ${SRC:S|\.c|${WRKSRC}/.o|:S|^|${WRKSRC}/|} \
		-o ${WRKSRC}/${PORTNAME} ${LDFLAGS}

do-install:
	${INSTALL_PROGRAM} ${WRKSRC}/${PORTNAME} ${STAGEDIR}${PREFIX}/bin

.include <bsd.port.mk>
