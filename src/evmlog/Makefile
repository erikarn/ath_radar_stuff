PROG=	evmlog

X_INCS=	-I../../lib/
X_LIBS=	-L../../lib/libradarpkt/

SRCS=	evm.c main.c
CFLAGS+=	-I/home/adrian/work/freebsd/head/src/sys $(X_INCS) -g -ggdb \
	-DATH_ENABLE_RADIOTAP_VENDOR_EXT
LDADD+=	$(X_LIBS) -lpcap -lradarpkt -lm

MK_MAN=	no

.include <bsd.prog.mk>
