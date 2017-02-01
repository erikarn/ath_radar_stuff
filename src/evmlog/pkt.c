#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <string.h>
#include <netinet/in.h>	/* for ntohl etc */
#include <sys/endian.h>

#include <sys/socket.h>
#include <net/if.h>

#include <pcap.h>

/*
 * We can't include the radiotap includes with
 * net80211, as the net80211 radiotap header
 * defines the vendor header and it clashes
 * with what's in the libradiotap header.
 *
 * The libradiotap header doesn't include
 * the vendor header, sigh.
 *
 * So, these are separate files. The radiotap parsing is
 * done in main.c; this is given a pointer to the
 * vendor area.
 */

#include "net80211/ieee80211.h"
#include "net80211/ieee80211_radiotap.h"

#include "dev/ath/if_athioctl.h"

#include "libradarpkt/chan.h"

#include "evm.h"

/* from _ieee80211.h */
#define      IEEE80211_CHAN_HT40U    0x00020000 /* HT 40 channel w/ ext above */
#define      IEEE80211_CHAN_HT40D    0x00040000 /* HT 40 channel w/ ext below */

// non-HT
// 0x00200140
// HT, not HT40
// 0x00210140

void
pkt_parse(int chip, const void *rp, const void *vp, uint64_t tsf,
    uint32_t chan_flags, const char *pkt, int len)
{
	const struct ieee80211_radiotap_header *rh;
	const struct ath_radiotap_vendor_hdr *vh;
	uint8_t rssi, nf;
	int r;
	struct xchan x;
	uint32_t evm[ATH_RADIOTAP_MAX_EVM];
	uint8_t rx_chainmask;
	uint8_t rx_hwrate;
	int rx_isht40;
	int rx_isht;
	int rx_isaggr;
	int rx_lastaggr;
	struct evm e;

	/* XXX assume it's a radiotap frame */
	rh = rp;
	vh = vp;

	if (rh->it_version != 0) {
		printf("%s: incorrect version (%d)\n", __func__,
		    rh->it_version);
		return;
	}

#if 0
	printf("%s: len=%d, present=0x%08x, vh_flags=0x%04x, type=%d, tsf=%llu, hwrate=0x%02x\n",
	    __func__,
	    (rh->it_len),	/* XXX why aren't these endian-converted? */
	    (rh->it_present),
	    vh->vh_flags,
	    vh->vh_frame_type,
	    (unsigned long long) vh->vh_raw_tsf,
	    vh->vh_rx_hwrate);
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
	memcpy(evm, vh->evm, ATH_RADIOTAP_MAX_EVM * 4);
	rx_chainmask = vh->vh_rx_chainmask;
	rx_hwrate = vh->vh_rx_hwrate;
	rx_isht40 = !! (chan_flags & (IEEE80211_CHAN_HT40U | IEEE80211_CHAN_HT40D));
	rx_isht = !! (rx_hwrate & 0x80);

	/*
	 * If aggr=1, then we only care about lastaggr.
	 * If aggr=0, then the stack will only pass us up a
	 * completed frame, with the final descriptors' status.
	 */

	rx_isaggr = !! (vh->vh_flags & ATH_VENDOR_PKT_ISAGGR);
	rx_lastaggr = 0;
	if ((vh->vh_flags & ATH_VENDOR_PKT_ISAGGR) &&
	    ! (vh->vh_flags & ATH_VENDOR_PKT_MOREAGGR)) {
		rx_lastaggr = 1;
	}

	if (rx_isht && (! rx_isaggr || rx_lastaggr)) {
		populate_evm(&e, evm, rx_hwrate, rx_isht40);

		printf("ts=%llu: rs_status=0x%x, chainmask=0x%x, "
		    "hwrate=0x%02x, isht=%d, is40=%d, "
		    "rssi_comb=%d, rssi_ctl=[%d %d %d], "
		    "rssi_ext=[%d %d %d]",
		    (unsigned long long) le64toh(tsf),
		    (int) vh->vh_rs_status,
		    (int) vh->vh_rx_chainmask,
		    (int) vh->vh_rx_hwrate,
		    (int) rx_isht,
		    (int) rx_isht40,
		    (int) (int8_t) ((vh->vh_rssi) & 0xff),
		    (int) (int8_t) ((vh->rssi_ctl[0]) & 0xff),
		    (int) (int8_t) ((vh->rssi_ctl[1]) & 0xff),
		    (int) (int8_t) ((vh->rssi_ctl[2]) & 0xff),
		    (int) (int8_t) ((vh->rssi_ext[0]) & 0xff),
		    (int) (int8_t) ((vh->rssi_ext[1]) & 0xff),
		    (int) (int8_t) ((vh->rssi_ext[2]) & 0xff)
		    );
		print_evm(&e);

		printf("\n");
	}
}
