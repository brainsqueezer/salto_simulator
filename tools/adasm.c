/**********************************************************
 * Xerox Alto Disassembler.
 *
 * (C) 2007 by Juergen Buchmueller <pullmoll@t-online.de>
 *
 * Disassembly of the Xerox Alto microcode PROMs,
 * the constant PROMs and a emulator binary.
 *
 * If no binary is specified on the command line, the
 * disassembler will disassemble the "duckbreath".
 *
 * Requires Alto (I/II) PROM images in current directory.
 *
 * $Id: adasm.c,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 **********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "emu_ops.h"
#include "mem_loc.h"

/**
 * @brief Microcode and constants PROM size
 */
#define	MCODE_PAGE	1024
#define	MCODE_SIZE	(2*MCODE_PAGE)		/* Alto II may have 2 pages */
#define	MCODE_MASK	(MCODE_SIZE - 1)
#define	PROM_SIZE	256

#define	RAM_SIZE	65536

static char rompath[FILENAME_MAX] = "roms";

static unsigned int prom_raw[PROM_SIZE];	/* constant PROM, raw (from file) */
static unsigned int mcode_raw[MCODE_SIZE];	/* microcode words, raw (from file)*/

static unsigned int prom[PROM_SIZE];		/* constant PROM (useable) */
static unsigned int mcode[MCODE_SIZE];		/* microcode words (useable) */

#define	MCODE_INVERTED	0x00088400	/** @brief mcode inverted bits */

/**
 * @brief short names for the 16 tasks
 */
static const char *tn[16] = {
	"EMU",		/* emulator task */
	"T01",
	"T02",
	"T03",
	"DSC",		/* disk sector c(ontrol?) */
	"T05",
	"T06",
	"ETH",
	"MRT",		/* memory refresh task */
	"DWT",		/* display word task */
	"CUR",		/* cursor task */
	"DHT",		/* display horizontal task */
	"DVT",		/* display vertical task */
	"PAR",		/* parity task */
	"DWD",		/* disk word done */
	"T17"
};

/**
 * @brief names for the 32 R registers
 */
static const char *rn[32] = {
	"AC(3)",
	"AC(2)",
	"AC(1)",
	"AC(0)",
	"R04",
	"R05",
	"PC",		/* emulator program counter */
	"R07",
	"R10",
	"R11",
	"R12",
	"R13",
	"R14",
	"R15",
	"R16",
	"R17",
	"R20",
	"R21",
	"CBA",		/* address of the currently active DCB+1 */
	"AECL",		/* address of end of current scanline's bitmap */
	"SLC",		/* scan line count */
	"HTAB",		/* number of tab words remaining on current scanline */
	"DWA",		/* address of the bit map double word being fetched */
	"MTEMP",	/* temporary cell */
	"R30",
	"R31",
	"R32",
	"R33",
	"R34",
	"R35",
	"R36",
	"R37"
};

/**
 * @brief load a 4 bit ROM image, shifting the nibble in its place
 *
 * @param where starting address of ROM image where to load
 * @param shift shift left count for the data
 * @param name filename to read
 */
void load_rom(unsigned int *where, int shift, const char *name)
{
	char filename[FILENAME_MAX];
	FILE *fp;
	int ch;

	snprintf(filename, sizeof(filename), "%s/%s", rompath, name);
	fp = fopen(filename, "rb");
	if (!fp) {
		fprintf(stderr, "failed to open '%s'\n", filename);
		exit(1);
	}
	for (;;) {
		ch = fgetc(fp);
		if (-1 == ch) {
			fclose(fp);
			return;
		}
		ch <<= shift;
		*where++ |= ch;
	}
	fclose(fp);
}

/**
 * @brief print a symbolic name for an mpc address
 *
 * @param a microcode address (mpc)
 * @result pointer to const string with the address or symbolic name
 */
static const char *an(int a)
{
	static char buff[4][32];
	static int which = 0;
	char *dst;

	which = (which + 1) % 4;
	dst = buff[which];

	switch (a) {
	/* start value for mpc per task is the task number */
	case  0: sprintf(dst, "»%s", tn[0]); break;
	case  1: sprintf(dst, "»%s", tn[1]); break;
	case  2: sprintf(dst, "»%s", tn[2]); break;
	case  3: sprintf(dst, "»%s", tn[3]); break;
	case  4: sprintf(dst, "»%s", tn[4]); break;
	case  5: sprintf(dst, "»%s", tn[5]); break;
	case  6: sprintf(dst, "»%s", tn[6]); break;
	case  7: sprintf(dst, "»%s", tn[7]); break;
	case  8: sprintf(dst, "»%s", tn[8]); break;
	case  9: sprintf(dst, "»%s", tn[9]); break;
	case 10: sprintf(dst, "»%s", tn[10]); break;
	case 11: sprintf(dst, "»%s", tn[11]); break;
	case 12: sprintf(dst, "»%s", tn[12]); break;
	case 13: sprintf(dst, "»%s", tn[13]); break;
	case 14: sprintf(dst, "»%s", tn[14]); break;
	case 15: sprintf(dst, "»%s", tn[15]); break;
	default: sprintf(dst, "%04o", a); break;
	}

	return buff[which];
}

unsigned int mcode_dasm(char *buff, unsigned int mir, unsigned int prefetch)
{
	int rsel = (mir >> 27) & 31;
	int aluf = (mir >> 23) & 15;
	int bs = (mir >> 20) & 7;
	int f1 = (mir >> 16) & 15;
	int f2 = (mir >> 12) & 15;
	int t = (mir >> 11) & 1;
	int l = (mir >> 10) & 1;
	int next = mir & (MCODE_SIZE-1);
	int pa;
	char *dst = buff;

	switch (aluf) {
	case  0: /* T?: BUS             */
		if (t)
			dst += sprintf(dst, "T«ALU ");
		if (l)
			dst += sprintf(dst, "L« ");
		if (bs == 1)
			dst += sprintf(dst, "%s« ", rn[rsel]);
		dst += sprintf(dst, "ALUF(BUS) ");
		break;
	case  1: /*   : T               */
		if (t)
			dst += sprintf(dst, "T«BUS ");
		if (l)
			dst += sprintf(dst, "L« ");
		if (bs == 1)
			dst += sprintf(dst, "%s« ", rn[rsel]);
		dst += sprintf(dst, "ALUF(T) ");
		break;
	case  2: /* T?: BUS OR T        */
		if (t)
			dst += sprintf(dst, "T«ALU ");
		if (l)
			dst += sprintf(dst, "L« ");
		if (bs == 1)
			dst += sprintf(dst, "%s« ", rn[rsel]);
		dst += sprintf(dst, "ALUF(BUS OR T) ");
		break;
	case  3: /*   : BUS AND T       */
		if (t)
			dst += sprintf(dst, "T«BUS ");
		if (l)
			dst += sprintf(dst, "L« ");
		if (bs == 1)
			dst += sprintf(dst, "%s« ", rn[rsel]);
		dst += sprintf(dst, "ALUF(BUS AND T) ");
		break;
	case  4: /*   : BUS XOR T       */
		if (t)
			dst += sprintf(dst, "T«BUS ");
		if (l)
			dst += sprintf(dst, "L« ");
		if (bs == 1)
			dst += sprintf(dst, "%s« ", rn[rsel]);
		dst += sprintf(dst, "ALUF(BUS XOR T) ");
		break;
	case  5: /* T?: BUS + 1         */
		if (t)
			dst += sprintf(dst, "T«ALU ");
		if (l)
			dst += sprintf(dst, "L« ");
		if (bs == 1)
			dst += sprintf(dst, "%s« ", rn[rsel]);
		dst += sprintf(dst, "ALUF(BUS + 1) ");
		break;
	case  6: /* T?: BUS - 1         */
		if (t)
			dst += sprintf(dst, "T«ALU ");
		if (l)
			dst += sprintf(dst, "L« ");
		if (bs == 1)
			dst += sprintf(dst, "%s« ", rn[rsel]);
		dst += sprintf(dst, "ALUF(BUS - 1) ");
		break;
	case  7: /*   : BUS + T         */
		if (t)
			dst += sprintf(dst, "T«BUS ");
		if (l)
			dst += sprintf(dst, "L« ");
		if (bs == 1)
			dst += sprintf(dst, "%s« ", rn[rsel]);
		dst += sprintf(dst, "ALUF(BUS + T) ");
		break;
	case  8: /*   : BUS - T         */
		if (t)
			dst += sprintf(dst, "T«BUS ");
		if (l)
			dst += sprintf(dst, "L« ");
		if (bs == 1)
			dst += sprintf(dst, "%s« ", rn[rsel]);
		dst += sprintf(dst, "ALUF(BUS - T) ");
		break;
	case  9: /*   : BUS - T - 1     */
		if (t)
			dst += sprintf(dst, "T«BUS ");
		if (l)
			dst += sprintf(dst, "L« ");
		if (bs == 1)
			dst += sprintf(dst, "%s« ", rn[rsel]);
		dst += sprintf(dst, "ALUF(BUS - T - 1) ");
		break;
	case 10: /* T?: BUS + T + 1     */
		if (t)
			dst += sprintf(dst, "T«ALU ");
		if (l)
			dst += sprintf(dst, "L« ");
		if (bs == 1)
			dst += sprintf(dst, "%s« ", rn[rsel]);
		dst += sprintf(dst, "ALUF(BUS + T + 1) ");
		break;
	case 11: /* T?: BUS + SKIP      */
		if (t)
			dst += sprintf(dst, "T«ALU ");
		if (l)
			dst += sprintf(dst, "L« ");
		if (bs == 1)
			dst += sprintf(dst, "%s« ", rn[rsel]);
		dst += sprintf(dst, "ALUF(BUS + SKIP) ");
		break;
	case 12: /* T?: BUS, T (AND)    */
		if (t)
			dst += sprintf(dst, "T«ALU ");
		if (l)
			dst += sprintf(dst, "L« ");
		if (bs == 1)
			dst += sprintf(dst, "%s« ", rn[rsel]);
		dst += sprintf(dst, "ALUF(BUS AND T) ");
		break;
	case 13: /*   : BUS AND NOT T   */
		if (t)
			dst += sprintf(dst, "T«BUS ");
		if (l)
			dst += sprintf(dst, "L« ");
		if (bs == 1)
			dst += sprintf(dst, "%s« ", rn[rsel]);
		dst += sprintf(dst, "ALUF(BUS AND NOT T) ");
		break;
	case 14: /*   : undefined       */
		if (t)
			dst += sprintf(dst, "T«BUS ");
		if (l)
			dst += sprintf(dst, "L« ");
		if (bs == 1)
			dst += sprintf(dst, "%s« ", rn[rsel]);
		dst += sprintf(dst, "ALUF(14) ");
		break;
	case 15: /*   : undefined       */
		if (t)
			dst += sprintf(dst, "T«BUS ");
		if (l)
			dst += sprintf(dst, "L« ");
		if (bs == 1)
			dst += sprintf(dst, "%s« ", rn[rsel]);
		dst += sprintf(dst, "ALUF(15) ");
		break;
	}

	switch (bs) {
	case 0:	/* read R */
		dst += sprintf(dst, "BUS«%s ", rn[rsel]);
		break;
	case 1:	/* load R from shifter output */
		/* dst += sprintf(dst, "; %s«", rn[rsel]); */
		break;
	case 2: /* enables no source to the BUS, leaving it all ones */
		dst += sprintf(dst, "BUS«177777 ");
		break;
	case 3:	/* performs different functions in different tasks */
		dst += sprintf(dst, "BUS«BS3 ");
		break;
	case 4:	/* performs different functions in different tasks */
		dst += sprintf(dst, "BUS«BS4 ");
		break;
	case 5:	/* memory data */
		dst += sprintf(dst, "BUS«MD ");
		break;
	case 6:	/* BUS[3-0] <- MOUSE; BUS[15-4] <- -1 */
		dst += sprintf(dst, "BUS«MOUSE ");
		break;
	case 7:	/* IR[7-0], possibly sign extended */
		dst += sprintf(dst, "BUS«DISP ");
		break;
	}


	switch (f1) {
	case 0:	/* no operation */
		break;
	case 1:	/* load MAR from ALU output; start main memory reference */
		dst += sprintf(dst, "MAR«ALU ");
		break;
	case 2:	/* switch tasks if higher priority wakeup is pending */
		dst += sprintf(dst, "[TASK] ");
		break;
	case 3:	/* disable the current task until re-enabled by a hardware-generated condition */
		dst += sprintf(dst, "[BLOCK] ");
		break;
	case 4:	/* SHIFTER output will be L shifted left one place */
		dst += sprintf(dst, "SHIFTER«L(LSH1) ");
		break;
	case 5:	/* SHIFTER output will be L shifted right one place */
		dst += sprintf(dst, "SHIFTER«L(RSH1) ");
		break;
	case 6:	/* SHIFTER output will be L rotated left 8 places */
		dst += sprintf(dst, "SHIFTER«L(LCY8) ");
		break;
	case 7:	/* put on the bus the constant from PROM (RSELECT,BS) */
		pa = (rsel << 3) | bs;
		dst += sprintf(dst, "BUS«[%03o]%05o ", pa, prom[pa]);
		break;
	default:
		dst += sprintf(dst, "F1_%02o ", f1);
		break;
	}

	switch (f2) {
	case 0:	/* no operation */
		break;
	case 1:	/* NEXT « NEXT OR (if (BUS=0) then 1 else 0) */
		dst += sprintf(dst, "[(BUS=0) ? %s : %s] ",
			an((prefetch | 1) & (MCODE_SIZE-1)),
			an(prefetch & (MCODE_SIZE-1)));
		break;
	case 2:	/* NEXT « NEXT OR (if (SHIFTER OUTPUT<0) then 1 else 0) */
		dst += sprintf(dst, "[(SH=0) ? %s : %s] ",
			an((prefetch | 1) & (MCODE_SIZE-1)),
			an(prefetch & (MCODE_SIZE-1)));
		break;
	case 3:	/* NEXT « NEXT OR (if (SHIFTER OUTPUT<0) then 1 else 0) */
		dst += sprintf(dst, "[(SH<0) ? %s : %s] ",
			an((prefetch | 1) & (MCODE_SIZE-1)),
			an(prefetch & (MCODE_SIZE-1)));
		break;
	case 4:	/* NEXT « NEXT OR BUS */
		dst += sprintf(dst, "NEXT«BUS ");
		break;
	case 5:	/* NEXT « NEXT OR ALUC0. ALUC0 is the carry produced by last L loading microinstruction. */
		dst += sprintf(dst, "[(ALUC0) ? %s : %s] ",
			an((prefetch | 1) & (MCODE_SIZE-1)),
			an(prefetch & (MCODE_SIZE-1)));
		break;
	case 6:	/* deliver BUS data to memory */
		dst += sprintf(dst, "MD«BUS ");
		break;
	case 7:	/* put on the bus the constant from PROM (RSELECT,BS) */
		pa = (rsel << 3) | bs;
		dst += sprintf(dst, "BUS«[%03o]%05o ", pa, prom[pa]);
		break;
	default:
		dst += sprintf(dst, "BUS«F2_%02o ", f2);
		break;
	}
	return next;
}


int main(int ac, char **av)
{
	char buff[512];
	unsigned int mpc;
	int i;

	memset(prom_raw, 0, PROM_SIZE * sizeof(prom[0]));
	memset(mcode_raw, 0, MCODE_SIZE * sizeof(mcode[0]));

	/* load constant proms nibbles */
	load_rom(prom_raw,  0, "c0.3");
	load_rom(prom_raw,  4, "c1.3");
	load_rom(prom_raw,  8, "c2.3");
	load_rom(prom_raw, 12, "c3.3");

	/* descramble PROM contents */
	for (i = 0; i < PROM_SIZE; i++) {
		int addr = 0;
		int data = 0;

		/* unscramble PROM address input lines */
		if (i & 0x80) addr |= 0x01;	/* RSEL.4 */
		if (i & 0x40) addr |= 0x80;	/* RSEL.3 */
		if (i & 0x20) addr |= 0x40;	/* RSEL.2 */
		if (i & 0x10) addr |= 0x20;	/* RSEL.1 */
		if (i & 0x08) addr |= 0x10;	/* RSEL.0 */
		if (i & 0x04) addr |= 0x02;	/* BS.2 */
		if (i & 0x02) addr |= 0x04;	/* BS.1 */
		if (i & 0x01) addr |= 0x08;	/* BS.0 */

		/* unscramble PROM data output lines */
		data = prom_raw[i] ^ 0177777;
		data = (((data & 0xaaaa) >> 1) | ((data & 0x5555) << 1));
		data = (((data & 0xcccc) >> 2) | ((data & 0x3333) << 2));
		data = (((data & 0xf0f0) >> 4) | ((data & 0x0f0f) << 4));
		data = (((data & 0xff00) >> 8) | ((data & 0x00ff) << 8));
		/* unscrambled data */
		prom[addr] = data;
	}

	/* load ROM0 microcode rom nibbles */
	load_rom(mcode_raw,28, "55x.3");
	load_rom(mcode_raw,24, "64x.3");
	load_rom(mcode_raw,20, "65x.3");
	load_rom(mcode_raw,16, "63x.3");
	load_rom(mcode_raw,12, "53x.3");
	load_rom(mcode_raw, 8, "60x.3");
	load_rom(mcode_raw, 4, "61x.3");
	load_rom(mcode_raw, 0, "62x.3");

	/* load ROM1 (1K) microcode rom nibbles */
#if	MCODE_SIZE > MCODE_PAGE
	load_rom(mcode_raw+MCODE_PAGE,28, "xm51.u54");
	load_rom(mcode_raw+MCODE_PAGE,24, "xm51.u74");
	load_rom(mcode_raw+MCODE_PAGE,20, "xm51.u75");
	load_rom(mcode_raw+MCODE_PAGE,16, "xm51.u73");
	load_rom(mcode_raw+MCODE_PAGE,12, "xm51.u52");
	load_rom(mcode_raw+MCODE_PAGE, 8, "xm51.u70");
	load_rom(mcode_raw+MCODE_PAGE, 4, "xm51.u71");
	load_rom(mcode_raw+MCODE_PAGE, 0, "xm51.u72");
#endif
	for (mpc = 0; mpc < MCODE_SIZE; mpc++) {
		/* address lines are inverted, thus swap the mcode words */
		mcode[mpc] = ~mcode_raw[mpc ^ MCODE_MASK] ^ MCODE_INVERTED;
	}

	printf("Microcode dump (numbers are octal):\n");
	for (mpc = 0; mpc < MCODE_SIZE; mpc++) {
		unsigned int mir = mcode[mpc];
		unsigned int prefetch = mcode[mpc+1];
		unsigned int next;
		next = mcode_dasm(buff, mir, prefetch);
		if (next == mpc + 1) {
			/* sequential execution */
			printf("%s: %011o   %s\n", an(mpc), mir, buff);
		} else {
			/* new NEXT address */
			printf("%s: %011o   %-64s NEXT«%04o\n", an(mpc), mir, buff, next);
		}
	}

	printf("\nConstant PROM (numbers are octal):");
	for (i = 0; i < sizeof(prom)/sizeof(prom[0]); i++) {
		if (i % 8)
			printf(" %06o", prom[i]);
		else
			printf("\n%03o: %06o", i, prom[i]);
	}
	printf("\n");
	return 0;
}
