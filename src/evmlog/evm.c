#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <string.h>
#include <stdint.h>
#include <sys/endian.h>

#include "evm.h"

/* Print the EVM information */
void
print_evm(struct evm *e)
{
	int s, p;

	for (s = 0; s < e->num_streams; s++) {
		printf(" evm_stream_%d=[", s + 1);
		for (p = 0; p < NUM_EVM_PILOTS; p++) {
			printf(" %d", (int) (e->evm_pilots[s][p]));
		}
		printf(" ]");
	}
}

/*
 * Populate the given EVM information.
 *
 * The EVM pilot offsets depend upon whether the rates are
 * 1, 2 or 3 stream, as well as HT20 or HT40.
 */
void
populate_evm(struct evm *e, uint32_t evm[5], uint8_t rx_hwrate, int rx_isht40)
{
	/* Initialise everything to 0x80 - invalid */
	memset(e->evm_pilots, 0x80, sizeof(e->evm_pilots));

	/* HT20 pilots - always 4 */
	e->num_pilots = 4;
	if (rx_isht40)
		e->num_pilots += 2;	/* HT40 - 6 pilots */

	/* XXX assume it's MCS */
	if (rx_hwrate < 0x88) {		/* 1 stream */
		e->num_streams = 1;
		e->evm_pilots[0][0] = MS(evm[0], EVM_0);
		e->evm_pilots[0][1] = MS(evm[0], EVM_1);
		e->evm_pilots[0][2] = MS(evm[0], EVM_2);
		e->evm_pilots[0][3] =
		    MS(evm[0], EVM_3);
		if (rx_isht40) {
			e->evm_pilots[0][4] = MS(evm[1], EVM_0),
			e->evm_pilots[0][5] = MS(evm[1], EVM_1);
		}
	} else if (rx_hwrate < 0x90) {	/* 2 stream */
		e->num_streams = 2;
		e->evm_pilots[0][0] = MS(evm[0], EVM_0);
		e->evm_pilots[0][1] = MS(evm[0], EVM_2);
		e->evm_pilots[0][2] = MS(evm[1], EVM_0);
		e->evm_pilots[0][3] = MS(evm[1], EVM_2);
		if (rx_isht40) {
			e->evm_pilots[0][4] = MS(evm[2], EVM_0),
			e->evm_pilots[0][5] = MS(evm[2], EVM_2);
		}
		e->evm_pilots[1][0] = MS(evm[0], EVM_1);
		e->evm_pilots[1][1] = MS(evm[0], EVM_3);
		e->evm_pilots[1][2] = MS(evm[1], EVM_1);
		e->evm_pilots[1][3] = MS(evm[1], EVM_3);
		if (rx_isht40) {
			e->evm_pilots[1][4] = MS(evm[2], EVM_1);
			e->evm_pilots[1][5] = MS(evm[2], EVM_3);
		}
	} else {			/* 3 stream */
		e->num_streams = 3;
		e->evm_pilots[0][0] = MS(evm[0], EVM_0);
		e->evm_pilots[0][1] = MS(evm[0], EVM_3);
		e->evm_pilots[0][2] = MS(evm[1], EVM_2);
		e->evm_pilots[0][3] = MS(evm[2], EVM_1);
		if (rx_isht40) {
			e->evm_pilots[0][4] = MS(evm[3], EVM_0);
			e->evm_pilots[0][5] = MS(evm[3], EVM_3);
		}
		e->evm_pilots[1][0] = MS(evm[0], EVM_1);
		e->evm_pilots[1][1] = MS(evm[1], EVM_0);
		e->evm_pilots[1][2] = MS(evm[1], EVM_3);
		e->evm_pilots[1][3] = MS(evm[2], EVM_2);
		if (rx_isht40) {
			e->evm_pilots[1][4] = MS(evm[3], EVM_1);
			e->evm_pilots[1][5] = MS(evm[4], EVM_0);
		}
		e->evm_pilots[2][0] = MS(evm[0], EVM_2);
		e->evm_pilots[2][1] = MS(evm[1], EVM_1);
		e->evm_pilots[2][2] = MS(evm[2], EVM_0);
		e->evm_pilots[2][3] = MS(evm[2], EVM_3);
		if (rx_isht40) {
			e->evm_pilots[2][4] = MS(evm[3], EVM_2);
			e->evm_pilots[2][5] = MS(evm[4], EVM_1);
		}
	}
}
