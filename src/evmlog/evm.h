#ifndef	__EVM_H__
#define	__EVM_H__

/*
 * Accessor macros to go dig through the DWORD for the relevant
 * EVM bytes.
 */
#define	MS(_v, _f)		( ((_v) & (_f)) >> _f ## _S )

#define	EVM_0		0x000000ff
#define	EVM_0_S		0
#define	EVM_1		0x0000ff00
#define	EVM_1_S		8
#define	EVM_2		0x00ff0000
#define	EVM_2_S		16
#define	EVM_3		0xff000000
#define	EVM_3_S		24

/*
 * The EVM pilot information representation, post-processed.
 */
#define	NUM_EVM_PILOTS		6
#define	NUM_EVM_STREAMS		3
struct evm {
	int8_t	evm_pilots[NUM_EVM_STREAMS][NUM_EVM_PILOTS];
	int num_pilots;
	int num_streams;
};

extern	void print_evm(struct evm *e);
extern	void populate_evm(struct evm *e, uint32_t evm[5], uint8_t rx_hwrate,
	    int rx_isht40);

#endif
