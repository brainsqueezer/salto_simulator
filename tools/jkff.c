#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
	/** @brief clock signal */
	JKFF_CLK	= 0001,
	/** @brief J input */
	JKFF_J		= 0002,
	/** @brief K' input */
	JKFF_K		= 0004,
	/** @brief S' input */
	JKFF_S		= 0010,
	/** @brief C' input */
	JKFF_C		= 0020,
	/** @brief Q output */
	JKFF_Q		= 0040,
	/** @brief Q' output */
	JKFF_Q0		= 0100,
}	jkff_t;
#define	JKFF_MAX	JKFF_Q0

/**
 * @brief simulate a 74109 J-K flip-flop with set and reset inputs
 *
 * @param s0 is the previous state of the FF's in- and outputs
 * @param s1 is the next state
 * @result returns the next state and probably modified Q output
 */
__inline static jkff_t update_jkff(jkff_t s0, jkff_t s1)
{
	switch (s1 & (JKFF_C | JKFF_S)) {
	case JKFF_C | JKFF_S:	/* C' is 1, and S' is 1 */
		if (((s0 ^ s1) & s1) & JKFF_CLK) {
			/* rising edge of the clock */
			switch (s1 & (JKFF_J | JKFF_K)) {
			case 0:
				/* both J and K' are 0: set Q to 0, Q' to 1 */
				s1 = (s1 & ~JKFF_Q) | JKFF_Q0;
				break;
			case JKFF_J:
				/* J is 1, and K' is 0: toggle Q */
				if (s0 & JKFF_Q)
					s1 = (s1 & ~JKFF_Q) | JKFF_Q0;
				else
					s1 = (s1 | JKFF_Q) & ~JKFF_Q0;
				break;
			case JKFF_K:
				/* J is 0, and K' is 1: keep Q as is */
				if (s0 & JKFF_Q)
					s1 = (s1 | JKFF_Q) & ~JKFF_Q0;
				else
					s1 = (s1 & ~JKFF_Q) | JKFF_Q0;
				break;
			case JKFF_J | JKFF_K:
				/* both J and K' are 1: set Q to 1 */
				s1 = (s1 | JKFF_Q) & ~JKFF_Q0;
				break;
			}
		} else {
			/* keep Q */
			s1 = (s1 & ~JKFF_Q) | (s0 & JKFF_Q);
		}
		break;
	case JKFF_S:
		/* S' is 1, C' is 0: set Q to 0, Q' to 1 */
		s1 = (s1 & ~JKFF_Q) | JKFF_Q0;
		break;
	case JKFF_C:
		/* S' is 0, C' is 1: set Q to 1, Q' to 0 */
		s1 = (s1 | JKFF_Q) & ~JKFF_Q0;
		break;
	case 0:
	default:
		/* unstable state (what to do?) */
		s1 = s1 | JKFF_Q | JKFF_Q0;
		break;
	}
	return s1;
}

int main(int argc, char **argv)
{
	int s0, s1;
	printf("/** @brief table constructed for update_jkff(s0,s1) */\n");
	printf("static jkff_t jkff_lookup[%d][%d] =\n{\n\t",
		JKFF_MAX, JKFF_MAX);
	for (s1 = 0; s1 < JKFF_MAX; s1++) {
		printf("{\n\t");
		for (s0 = 0; s0 < JKFF_MAX; s0++) {
			int res = update_jkff(s0,s1);
			if (0 == (s0 % 8))
				printf("\t");
			printf("0x%02x", res);
			if (s0 + 1 < JKFF_MAX)
				printf(",");
			if (7 == (s0 % 8))
				printf("\n\t");
		}
		printf("}%s", (s1 + 1) < JKFF_MAX ? "," : "\n");
	}
	printf("};\n");
	return 0;
}
