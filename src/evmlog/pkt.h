#ifndef	__PKT_H__
#define	__PKT_H__

extern	void pkt_parse(int chip, const void *rp, const void *vp, uint64_t tsf,
	    uint32_t flags, const char *pkt, int len);

#endif	/* __PKT_H__ */
