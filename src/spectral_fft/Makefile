PROG=	fft_eval

SRCS=	fft_eval.c fft_freebsd.c fft_histogram.c fft_display.c

CFLAGS+=	-I/usr/local/include -L/usr/local/lib -I../../lib/ -L../../lib/libradarpkt -L../../lib/libradiotap/ -pthread

LDADD+=	-lSDL -lSDL_ttf -lradarpkt -lradiotap -lpcap -pthread -lm

NO_MAN=	1

.include <bsd.prog.mk>
