PROG=	main

X_INCS=	-I../../lib/
X_LIBS=	-L../../lib/libradarpkt/ -L../../lib/libradiotap

SRCS=	main.c
CFLAGS+=	-I/home/adrian/work/freebsd/ath/head/src/sys $(X_INCS) -g -ggdb \
	-DATH_ENABLE_RADIOTAP_VENDOR_EXT
LDADD+=	$(X_LIBS) -lpcap -lradarpkt -lm -lradiotap

NO_MAN=	1

.include <bsd.prog.mk>
