# Created by: Kirill Ponomarew <krion@FreeBSD.org>

PORTNAME=	shaaft
PORTVERSION=	0.5.0
PORTREVISION=	25
CATEGORIES=	games
MASTER_SITES=	SF/criticalmass/OldFiles
DISTNAME=	Shaaft-${PORTVERSION}

MAINTAINER=	ports@FreeBSD.org
COMMENT=	OpenGL 3D falling block game similar to Blockout

LIB_DEPENDS=	libpng.so:graphics/png

CONFLICTS=	criticalmass-*

USE_HARDENING=	pie:off relro:off

USES=		gl sdl tar:bzip2
USE_SDL=	mixer image sdl
USE_GL=		gl
USE_GCC=	yes
USE_CXXSTD=	gnu++98
GNU_CONFIGURE=	yes
CONFIGURE_ARGS=	--disable-optimize

CPPFLAGS+=	$$(libpng-config --I_opts)
LDFLAGS+=	$$(libpng-config --L_opts)

DATADIR=	${PREFIX}/share/Shaaft

PLIST_FILES=	bin/Packer bin/shaaft %%DATADIR%%/resource.dat \
		man/man6/shaaft.6.gz

DESKTOP_ENTRIES="Shaaft" "" "" "${PORTNAME}" "" ""

post-patch:
	@${REINPLACE_CMD} -e 's|sdl-config|${SDL_CONFIG:T}|g ; \
		s|^CXXFLAGS|#CXXFLAGS|g ; \
		s|/usr/X11R6|${LOCALBASE}|g ; \
		s|-lSDL | |g ; \
		s|-lpng12|-lpng|g' ${WRKSRC}/configure
	@${GREP} -lR "SDL/SDL" ${WRKSRC} | ${XARGS} ${REINPLACE_CMD} -e \
		's|SDL/SDL|SDL|g'
	@${REINPLACE_CMD} -e 's|== 3|>= 3|' ${WRKSRC}/utils/hashMap.hpp

.include <bsd.port.mk>
