# Created by: Peter C. Lai <cowbert@gmail.com>

PORTNAME=	mk-livestatus
PORTVERSION=	1.2.8p25
PORTREVISION=	2
CATEGORIES=	net-mgmt
MASTER_SITES=	http://mathias-kettner.de/download/
DIST_SUBDIR=	check_mk

MAINTAINER=	egypcio@FreeBSD.org
COMMENT=	Nagios event broker module

LICENSE=	GPLv2

BROKEN_FreeBSD_13=	ld: error: duplicate symbol: g_mainthread_id
BROKEN_FreeBSD_14=	ld: error: duplicate symbol: g_mainthread_id

USES=		compiler:c++11-lang

USE_HARDENING=	safestack

GNU_CONFIGURE=	yes

PLIST_FILES=	lib/mk-livestatus/unixcat \
		lib/mk-livestatus/livestatus.o
SUB_FILES=	pkg-message

PORTEXAMPLES=	*.py README
EXAMPLE_FILES=	example_multisite.py example.py \
		livestatus.py make_nagvis_map.py \
		README

# parameter "command_file" from nagios.cfg
COMMAND_FILE?=	/var/spool/nagios/rw/live

OPTIONS_DEFINE=	EXAMPLES PERL
OPTIONS_SINGLE=	NAGIOS
OPTIONS_SINGLE_NAGIOS=	NAGIOS3 NAGIOS4
OPTIONS_DEFAULT=	NAGIOS4

NAGIOS3_DESC=	Build support for Nagios 3.x
NAGIOS4_DESC=	Build support for Nagios 4.x
PERL_DESC=	Install Monitoring-Livestatus Perl API

EXAMPLES_USES=	python
NAGIOS3_RUN_DEPENDS=	nagios>=3.5.1:net-mgmt/nagios
# --without-nagios4 is broken and is equivalent to --with-nagios4,
# so do not use NAGIOS4_CONFIGURE_WITH here or it breaks the NAGIOS3
# option, cf. PR 232716.
NAGIOS4_CONFIGURE_ON=	--with-nagios4
NAGIOS4_RUN_DEPENDS=	nagios4>=4.4:net-mgmt/nagios4
PERL_RUN_DEPENDS=	p5-Monitoring-Livestatus>=0:net-mgmt/p5-Monitoring-Livestatus

post-patch:
	${REINPLACE_CMD} -e 's|/usr/local/nagios/var/rw/live|${COMMAND_FILE}|' \
		${WRKSRC}/src/livestatus.h

post-install-EXAMPLES-on:
	${MKDIR} ${STAGEDIR}${EXAMPLESDIR}
	${REINPLACE_CMD} -e 's|/usr/bin/python|${PYTHON_CMD}|g' \
		-e 's|/var/lib/nagios/rw/live|${COMMAND_FILE}|g' \
		-e 's|/var/run/nagios/rw/live|${COMMAND_FILE}|g' \
		${PORTEXAMPLES:S|^|${WRKSRC}/api/python/|}
	${INSTALL_DATA} ${PORTEXAMPLES:S|^|${WRKSRC}/api/python/|} ${STAGEDIR}${EXAMPLESDIR}

.include <bsd.port.mk>
