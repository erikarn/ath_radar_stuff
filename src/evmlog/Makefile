PROG=	evmlog

X_INCS=	-I../../lib/
X_LIBS=	-L../../lib/libradarpkt/ -L../../lib/libradiotap

SRCS=	evm.c pkt.c main.c
CFLAGS+=	-I/home/adrian/work/freebsd/head/src/sys $(X_INCS) -g -ggdb \
	-DATH_ENABLE_RADIOTAP_VENDOR_EXT
LDADD+=	$(X_LIBS) -lpcap -lradarpkt -lm -lradiotap

MK_MAN=	no

.include <bsd.prog.mk>
