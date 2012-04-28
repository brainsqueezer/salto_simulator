#define	loc_DASTART	0000420	/* display list header */
#define	loc_DVIBITS	0000421	/* display vertical field interrupt bitword */
#define	loc_ITQUAN	0000422	/* interval timer stored quantity */
#define	loc_ITBITS	0000423	/* interval timer bitword */
#define	loc_MOUSEX	0000424	/* mouse X coordinate */
#define	loc_MOUSEY	0000425	/* mouse Y coordinate */
#define	loc_CURSORX	0000426	/* cursor X coordinate */
#define	loc_CURSORY	0000427	/* cursor Y coordinate */
#define	loc_RTC		0000430	/* real time clock */
#define	loc_CURMAP	0000431	/* cursor bitmap (16 words up to 00450) */
#define	loc_WW		0000452	/* interrupt wakeups waiting */
#define	loc_ACTIVE	0000453	/* active interrupt bitword */
#define	loc_MASKTAB	0000460	/* mask table for convert */
#define	loc_PCLOC	0000500	/* saved interrupt PC */
#define	loc_INTVEC	0000501	/* interrupt transfer vector (15 words up to 00517) */
#define	loc_KBLK	0000521	/* disk command block address */
#define	loc_KSTAT	0000522	/* disk status at start of current sector */
#define	loc_KADDR	0000523	/* disk address of latest disk command */
#define	loc_KSIBITS	0000524	/* sector interrupt bit mask */
#define	loc_ITTIME	0000525	/* interval timer timer */
#define	loc_TRAPPC	0000527	/* trap saved PC */
#define	loc_TRAPVEC	0000530	/* trap vectors (up to 0567) */
#define	loc_TIMERDATA	0000570	/* timer data (OS; up to 0577) */
#define	loc_EPLOC	0000600	/* ethernet post location */
#define	loc_EBLOC	0000601	/* ethernet interrupt bitmask */
#define	loc_EELOC	0000602	/* ethernet ending count */
#define	loc_ELLOC	0000603	/* ethernet load location */
#define	loc_EICLOC	0000604	/* ethernet input buffer count */
#define	loc_EIPLOC	0000605	/* ethernet input buffer pointer */
#define	loc_EOCLOC	0000606	/* ethernet output buffer count */
#define	loc_EOPLOC	0000607	/* ethernet output buffer pointer */
#define	loc_EHLOC	0000610	/* ethernet host address */
#define	loc_ERSVD	0000611	/* reserved for ethernet expansion (up to 00612) */
#define	loc_ALTOV	0000613	/* Alto I/II indication that microcode can
				 * interrogate (0 = Alto I, -1 = Alto II)
				 */
#define	loc_DCBR	0000614	/* posted by parity task (main memory parity error) */
#define	loc_KNMAR	0000615	/* -"- */
#define	loc_DWA		0000616	/* -"- */
#define	loc_CBA		0000617	/* -"- */
#define	loc_PC		0000620	/* -"- */
#define	loc_SAD		0000621	/* -"- */
#define	loc_SWATR	0000700	/* saved registers (Swat; up to 00707) */
#define	loc_UTILOUT	0177016	/* printer output (up to 177017) */
#define	loc_XBUS	0177020	/* untility input bus (up to 177023) */
#define	loc_MEAR	0177024	/* memory error address register */
#define	loc_MESR	0177025	/* memory error status register */
#define	loc_MECR	0177026	/* memory error control register */
#define	loc_UTILIN	0177030	/* printer status, mouse keyset */
#define	loc_KBDAD	0177034	/* undecoded keyboard (up to 177037) */
#define	loc_BANKREGS	0177740	/* extended memory option bank registers */
