/*
 * Macros stolen from Berkeley sys/select.h.
 * These macros manipulate bit fields.
 * BIT_SETSIZE may be defined by the user, but the default here
 * should be >= NOFILE (param.h).
 */
#ifndef	BIT_SETSIZE
#define	BIT_SETSIZE	1024
#endif

#ifndef NBBY		/* number of bits per byte */
#define	NBBY 8
#endif

typedef	long	fd_mask;
#define	NBITS	(sizeof (fd_mask) * NBBY)	/* bits per mask */
#ifndef	howmany
#define	howmany(x, y)	(((x)+((y)-1))/(y))
#endif

typedef	struct fd_set {
	fd_mask	fds_bits[howmany(BIT_SETSIZE, NBITS)];
} bit_set;

#define	BIT_SET(n, p)	((p)->fds_bits[(n)/NBITS] |= \
			    ((unsigned)1 << ((NBITS-n-1) % NBITS)))

#define	BIT_CLR(n, p)	((p)->fds_bits[(n)/NBITS] &= \
			    ~((unsigned)1 << ((NBITS-n-1) % NBITS)))

#define	BIT_ISSET(n, p)	((p)->fds_bits[(n)/NBITS] & \
			    ((unsigned)1 << ((NBITS-n-1) % NBITS)))

#define	BIT_ZERO(p)	memset((char *)(p), 0, sizeof (*(p)))
