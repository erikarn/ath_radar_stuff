LIB=	radarpkt

SRCS=	ar5212_radar.c ar9280_radar.c ar5416_radar.c
SRCS+=	chan.c

# Define 'ATH_ENABLE_RADIOTAP_VENDOR_EXT' as it's not enabled by default
# in the configuration file.  Ideally this would just fake an "opt_ath.h"
# and include it, but I digress..

CFLAGS+=	-I/home/adrian/work/freebsd/head/src/sys -g -ggdb	\
	    -DATH_ENABLE_RADIOTAP_VENDOR_EXT -g -ggdb -pthread		\
	    -I../../lib/

MK_MAN=	no

.include <bsd.lib.mk>
