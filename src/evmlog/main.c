#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <string.h>
#include <sys/endian.h>

#include <pcap.h>

#include "libradiotap/radiotap.h"
#include "libradiotap/radiotap_iter.h"

#include "evm.h"
#include "pkt.h"

struct xchan {
	uint32_t flags;
	uint16_t freq;
	uint8_t ieee;
	uint8_t maxpow;
} __packed;


// non-HT
// 0x00200140
// HT, not HT40
// 0x00210140

#if 0
/*
 * Compile up a rule that's bound to be useful - it only matches on
 * radar errors.
 *
 * tcpdump -ni wlan0 -y IEEE802_11_RADIO -x -X -s0 -v -ve \
 *    'radio[79] == 0x01'
 */
#define	PKTRULE "radio[79] & 0x01 == 0x01"

static int
pkt_compile(pcap_t *p, struct bpf_program *fp)
{
	if (pcap_compile(p, fp, PKTRULE, 1, 0) != 0)
		return 0;
	return 1;
}
#endif

void
pkt_handle(int chip, const char *pkt, int len)
{
	struct ieee80211_radiotap_iterator iter;
	const struct ieee80211_radiotap_header *rh;
	const void *vh = NULL;
	int err;
	uint32_t chan_flags;
	uint64_t tsf;
	struct xchan xc;

	/* XXX assume it's a radiotap frame */
	rh = (struct ieee80211_radiotap_header *) pkt;

	err = ieee80211_radiotap_iterator_init(&iter, (void *) pkt, len, NULL);
	if (err) {
		fprintf(stderr, "%s: invalid radiotap header\n", __func__);
		return;
	}

	while (!(err = ieee80211_radiotap_iterator_next(&iter))) {
		if (iter.this_arg_index == IEEE80211_RADIOTAP_VENDOR_NAMESPACE) {
			vh = (char *) iter.this_arg + 6; /* XXX 6 byte vendor header */
		} else if (iter.is_radiotap_ns) {
			if (iter.this_arg_index == IEEE80211_RADIOTAP_TSFT) {
				tsf = le64toh(*(unsigned long long *)iter.this_arg);
			} else if (iter.this_arg_index == IEEE80211_RADIOTAP_XCHANNEL) {
				/* XXX should limit copy size to this_arg's size */
				memcpy(&xc, iter.this_arg, sizeof(xc));
				/* XXX endian */
				chan_flags = xc.flags;
			}
		} else {
			printf("vendor ns\n");
		}
	}
	//printf("done\n");

	if (vh == NULL)
		return;

	pkt_parse(chip, rh, vh, tsf, chan_flags, pkt, len);
}

static pcap_t *
open_offline(const char *fname)
{
	pcap_t *p;
	char errbuf[PCAP_ERRBUF_SIZE];

	p = pcap_open_offline(fname, errbuf);
	if (p == NULL) {
		printf("pcap_create failed: %s\n", errbuf);
		return (NULL);
	}

	return (p);
}

static pcap_t *
open_online(const char *ifname)
{
	pcap_t *p;
	char errbuf[PCAP_ERRBUF_SIZE];
	struct bpf_program fp;

	p = pcap_open_live(ifname, 65536, 1, 1000, errbuf);
	if (! p) {
		err(1, "pcap_create: %s\n", errbuf);
		return (NULL);
	}

	if (pcap_set_datalink(p, DLT_IEEE802_11_RADIO) != 0) {
		pcap_perror(p, "pcap_set_datalink");
		return (NULL);
	}

	/* XXX pcap_is_swapped() ? */

#if 0
	if (! pkt_compile(p, &fp)) {
		pcap_perror(p, "pkg_compile compile error\n");
		return (NULL);
	}

	if (pcap_setfilter(p, &fp) != 0) {
		printf("pcap_setfilter failed\n");
		return (NULL);
	}
#endif

	return (p);
}

static void
usage(const char *progname)
{

	printf("Usage: %s <file|if> <filename|ifname>\n",
	    progname);
}

int
main(int argc, const char *argv[])
{
	char *dev;
	pcap_t * p;
	const char *fname;
	const unsigned char *pkt;
	struct pcap_pkthdr *hdr;
	int len, r;
	int chip = 0;

	if (argc < 3) {
		usage(argv[0]);
		exit(255);
	}

	/* XXX verify */
	fname = argv[2];

	if (strcmp(argv[1], "file") == 0) {
		p = open_offline(fname);
	} else if (strcmp(argv[1], "if") == 0) {
		p = open_online(fname);
	} else {
		usage(argv[0]);
		exit(255);
	}

	if (p == NULL)
		exit(255);

	/*
	 * XXX We should compile a filter for this, but the
	 * XXX access method is a non-standard hack atm.
	 */
	while ((r = pcap_next_ex(p, &hdr, &pkt)) >= 0) {
#if 0
		printf("capture: len=%d, caplen=%d\n",
		    hdr->len, hdr->caplen);
#endif
		if (r > 0)
			pkt_handle(chip, (const char *) pkt, hdr->caplen);
	}

	pcap_close(p);
}
