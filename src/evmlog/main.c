#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <string.h>
#include <netinet/in.h>	/* for ntohl etc */
#include <sys/endian.h>

#include <sys/socket.h>
#include <net/if.h>

#include <pcap.h>

#include "net80211/ieee80211.h"
#include "net80211/ieee80211_radiotap.h"

#include "dev/ath/if_athioctl.h"

#if 0
#include "libradarpkt/pkt.h"
#include "libradarpkt/ar5212_radar.h"
#include "libradarpkt/ar5416_radar.h"
#include "libradarpkt/ar9280_radar.h"
#endif

#include "libradarpkt/chan.h"

#include "evm.h"

/* from _ieee80211.h */
#define      IEEE80211_CHAN_HT40U    0x00020000 /* HT 40 channel w/ ext above */
#define      IEEE80211_CHAN_HT40D    0x00040000 /* HT 40 channel w/ ext below */

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
	struct ieee80211_radiotap_header *rh;
	struct ath_rx_radiotap_header *rx;
	uint8_t rssi, nf;
	int r;
	struct xchan x;
	uint32_t evm[5];	/* XXX ATH_RADIOTAP_MAX_CHAINS */
	uint8_t rx_chainmask;
	uint8_t rx_hwrate;
	int rx_isht40;
	int rx_isht;
	int rx_isaggr;
	int rx_lastaggr;
	struct evm e;

	/* XXX assume it's a radiotap frame */
	rh = (struct ieee80211_radiotap_header *) pkt;

	/*
	 * XXX assume it's an ath radiotap header; don't decode the payload
	 * via a radiotap decoder!
	 */
	rx = (struct ath_rx_radiotap_header *) pkt;

	if (rh->it_version != 0) {
		printf("%s: incorrect version (%d)\n", __func__,
		    rh->it_version);
		return;
	}

#if 0
	printf("%s: len=%d, present=0x%08x\n",
	    __func__,
	    (rh->it_len),	/* XXX why aren't these endian-converted? */
	    (rh->it_present));
#endif

	/* XXX TODO: check vh_flags to ensure this is an RX frame */

	/*
	 * Do a frequency lookup.
	 */
	/* XXX rh->it_len should be endian checked?! */
	if (pkt_lookup_chan((char *) pkt, len, &x) != 0) {
//		printf("%s: channel lookup failed\n", __func__);
		return;
	}

	/*
	 * Copy out the EVM data, receive rate, RX chainmask from the
	 * header.
	 *
	 * XXX TODO: methodize this; endianness!
	 */
	memcpy(evm, pkt + 48, 4 * 4);
	rx_chainmask = rx->wr_v.vh_rx_chainmask;
	rx_hwrate = rx->wr_v.vh_rx_hwrate;
	rx_isht40 = !! (rx->wr_chan_flags & (IEEE80211_CHAN_HT40U | IEEE80211_CHAN_HT40D));
	rx_isht = !! (rx_hwrate & 0x80);

	/*
	 * If aggr=1, then we only care about lastaggr.
	 * If aggr=0, then the stack will only pass us up a
	 * completed frame, with the final descriptors' status.
	 */

	rx_isaggr = !! (rx->wr_v.vh_flags & ATH_VENDOR_PKT_ISAGGR);
	rx_lastaggr = 0;
	if ((rx->wr_v.vh_flags & ATH_VENDOR_PKT_ISAGGR) &&
	    ! (rx->wr_v.vh_flags & ATH_VENDOR_PKT_MOREAGGR)) {
		rx_lastaggr = 1;
	}

	if (rx_isht && (! rx_isaggr || rx_lastaggr)) {
		populate_evm(&e, evm, rx_hwrate, rx_isht40);

		printf("ts=%llu: rs_status=0x%x, chainmask=0x%x, "
		    "hwrate=0x%02x, isht=%d, is40=%d, "
		    "rssi_comb=%d, rssi_ctl=[%d %d %d], "
		    "rssi_ext=[%d %d %d]",
		    (unsigned long long) le64toh(rx->wr_tsf),
		    (int) rx->wr_v.vh_rs_status,
		    (int) rx->wr_v.vh_rx_chainmask,
		    (int) rx->wr_v.vh_rx_hwrate,
		    (int) rx_isht,
		    (int) rx_isht40,
		    (int) (int8_t) ((rx->wr_v.vh_rssi) & 0xff),
		    (int) (int8_t) ((rx->wr_v.rssi_ctl[0]) & 0xff),
		    (int) (int8_t) ((rx->wr_v.rssi_ctl[1]) & 0xff),
		    (int) (int8_t) ((rx->wr_v.rssi_ctl[2]) & 0xff),
		    (int) (int8_t) ((rx->wr_v.rssi_ext[0]) & 0xff),
		    (int) (int8_t) ((rx->wr_v.rssi_ext[1]) & 0xff),
		    (int) (int8_t) ((rx->wr_v.rssi_ext[2]) & 0xff)
		    );
		print_evm(&e);

		printf("\n");
	}
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
