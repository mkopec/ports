# Created by: skreuzer@FreeBSD.org

PORTNAME=	zookeeper
PORTVERSION=	3.7.0
CATEGORIES=	devel
MASTER_SITES=	APACHE/${PORTNAME}/${PORTNAME}-${PORTVERSION}
PKGNAMEPREFIX=	lib
DISTNAME=	apache-${PORTNAME}-${PORTVERSION}

MAINTAINER=	dmitry.wagin@ya.ru
COMMENT=	C client interface to Zookeeper server

LICENSE=	APACHE20
LICENSE_FILE=	${WRKSRC}/LICENSE

LIB_DEPENDS=	libcppunit.so:devel/cppunit

USE_HARDENING=	stackautoinit:off

USES=		autoreconf libtool pkgconfig
USE_LDCONFIG=	yes

GNU_CONFIGURE=	yes

WRKSRC=		${WRKDIR}/${DISTNAME}/zookeeper-client/zookeeper-client-c

post-extract:
	@${MKDIR} ${WRKSRC}/generated/
	@cd ${FILESDIR} && ${CP} -p zookeeper.jute.* ${WRKSRC}/generated/

post-install:
	@${STRIP_CMD} ${STAGEDIR}${PREFIX}/bin/* ${STAGEDIR}${PREFIX}/lib/*.so
	${LN} -sf libzookeeper_mt.so.2 ${STAGEDIR}${PREFIX}/lib/libzookeeper_mt.so
	${LN} -sf libzookeeper_st.so.2 ${STAGEDIR}${PREFIX}/lib/libzookeeper_st.so

.include <bsd.port.mk>
