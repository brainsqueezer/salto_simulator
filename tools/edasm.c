/**********************************************************
 * Xerox Alto Emulator Disassembler.
 *
 * (C) 2007 by Juergen Buchmueller <pullmoll@t-online.de>
 *
 *
 * $Id: edasm.c,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 **********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "emu_ops.h"
#include "mem_loc.h"

#define	RAM_SIZE	65536

static unsigned int ram[RAM_SIZE];
static char is_data[RAM_SIZE];

#define	PCREL_MODE_ABS	0
#define	PCREL_MODE_DOT	1
/**
 * @brief selected mode for PC relative addressing
 */
static int pcrel_mode = PCREL_MODE_ABS;
static int start = 0;
static int base = 8;
static int ram_data = 0;
static int swab_words = 0;

/**
 * @brief load a RAM image
 *
 * @param where starting address of RAM image where to load
 * @param name filename to read
 */
int load_ram(unsigned int *where, const char *name)
{
	FILE *fp;
	int cl, ch;
	int size;

	fp = fopen(name, "rb");
	if (!fp) {
		fprintf(stderr, "failed to open '%s'\n", name);
		return 0;
	}
	for (size = 0; size < sizeof(ram)/sizeof(ram[0]); size++) {
		cl = fgetc(fp);
		if (-1 == cl) {
			fclose(fp);
			return size;
		}
		ch = fgetc(fp);
		if (-1 == ch) {
			fclose(fp);
			return size;
		}
		*where++ = (ch << 8) | cl;
	}
	return size;
}

/* prototypes */
static const char *page0(int disp);
static const char *pcrel(int pc, int disp);
static const char *sdisp(int disp);
static const char *symaddr(int disp);

static const char *page0(int disp)
{
	static char buff[4][16];
	static int which = 0;
	char *dst;

	which = (which + 1) % 4;
	dst = buff[which];
	snprintf(dst, sizeof(buff[0]), "0%o", disp & 0377);
	return buff[which];
}

static const char *pcrel(int pc, int disp)
{
	static char buff[4][16];
	static int which = 0;
	char *dst;

	which = (which + 1) % 4;
	dst = buff[which];
	switch (base) {
	case 8:
	default:
		if (pcrel_mode == PCREL_MODE_ABS) {
			snprintf(dst, sizeof(buff[0]), "%#o", (pc + (signed char)disp) & 0177777);
		} else {
			if (disp & 0200)
				snprintf(dst, sizeof(buff[0]), ".-%#o", 0400 - (disp & 0377));
			else
				snprintf(dst, sizeof(buff[0]), ".+%#o", disp & 0177);
		}
		break;
	case 10:
		if (pcrel_mode == PCREL_MODE_ABS) {
			snprintf(dst, sizeof(buff[0]), "%d", (pc + (signed char)disp) & 0177777);
		} else {
			if (disp & 0200)
				snprintf(dst, sizeof(buff[0]), ".-%d", 0400 - (disp & 0377));
			else
				snprintf(dst, sizeof(buff[0]), ".+%d", disp & 0177);
		}
		break;
	case 16:
		if (pcrel_mode == PCREL_MODE_ABS) {
			snprintf(dst, sizeof(buff[0]), "%#x", (pc + (signed char)disp) & 0177777);
		} else {
			if (disp & 0200)
				snprintf(dst, sizeof(buff[0]), ".-%#x", 0400 - (disp & 0377));
			else
				snprintf(dst, sizeof(buff[0]), ".+%#x", disp & 0177);
		}
		break;
	}
	return buff[which];
}

static const char *sdisp(int disp)
{
	static char buff[4][16];
	static int which = 0;
	char *dst;

	which = (which + 1) % 4;
	dst = buff[which];
	switch (base) {
	case 8:
		if (disp & 0200)
			snprintf(dst, sizeof(buff[0]), "-%#o", 0400 - (disp & 0377));
		else if (disp < 8)
			snprintf(dst, sizeof(buff[0]), "%#o", disp);
		else
			snprintf(dst, sizeof(buff[0]), "%#o", disp & 0177);
		break;
	case 10:
		if (disp & 0200)
			snprintf(dst, sizeof(buff[0]), "-%d", 0400 - (disp & 0377));
		else if (disp < 10)
			snprintf(dst, sizeof(buff[0]), "%d", disp);
		else
			snprintf(dst, sizeof(buff[0]), "%d", disp & 0177);
		break;
	case 16:
		if (disp & 0200)
			snprintf(dst, sizeof(buff[0]), "-%#x", 0400 - (disp & 0377));
		else if (disp < 10)
			snprintf(dst, sizeof(buff[0]), "%d", disp);
		else
			snprintf(dst, sizeof(buff[0]), "%#x", disp & 0177);
		break;
	}
	return buff[which];
}

static const char *symaddr(int addr)
{
	static char buff[4][16];
	static int which = 0;
	char *dst;

	which = (which + 1) % 4;
	dst = buff[which];
	switch (addr) {
	case loc_DASTART:	/* display list header */
		sprintf(dst, "DASTART");
		break;
	case loc_DVIBITS:	/* display vertical field interrupt bitword */
		sprintf(dst, "DVIBITS");
		break;
	case loc_ITQUAN:	/* interval timer stored quantity */
		sprintf(dst, "ITQUAN");
		break;
	case loc_ITBITS:	/* interval timer bitword */
		sprintf(dst, "ITBITS");
		break;
	case loc_MOUSEX:	/* mouse X coordinate */
		sprintf(dst, "MOUSEX");
		break;
	case loc_MOUSEY:	/* mouse Y coordinate */
		sprintf(dst, "MOUSEY");
		break;
	case loc_CURSORX:	/* cursor X coordinate */
		sprintf(dst, "CURSORX");
		break;
	case loc_CURSORY:	/* cursor Y coordinate */
		sprintf(dst, "CURSORY");
		break;
	case loc_RTC:		/* real time clock */
		sprintf(dst, "RTC");
		break;
	case loc_CURMAP:    case loc_CURMAP+1:  case loc_CURMAP+2:  case loc_CURMAP+3:
	case loc_CURMAP+4:  case loc_CURMAP+5:  case loc_CURMAP+6:  case loc_CURMAP+7:
	case loc_CURMAP+8:  case loc_CURMAP+9:  case loc_CURMAP+10: case loc_CURMAP+11:
	case loc_CURMAP+12: case loc_CURMAP+13: case loc_CURMAP+14: case loc_CURMAP+15:
		/* cursor bitmap (16 words up to 00450) */
		sprintf(dst, "CURMAP+%#o", addr - loc_CURMAP);
		break;
	case loc_WW:		/* interrupt wakeups waiting */
		sprintf(dst, "WW");
		break;
	case loc_ACTIVE:	/* active interrupt bitword */
		sprintf(dst, "ACTIVE");
		break;
	case loc_MASKTAB:	/* mask table for convert */
		sprintf(dst, "MASKTAB");
		break;
	case loc_PCLOC:		/* saved interrupt PC */
		sprintf(dst, "PCLOC");
		break;
	case loc_INTVEC:    case loc_INTVEC+1:  case loc_INTVEC+2:  case loc_INTVEC+3:
	case loc_INTVEC+4:  case loc_INTVEC+5:  case loc_INTVEC+6:  case loc_INTVEC+7:
	case loc_INTVEC+8:  case loc_INTVEC+9:  case loc_INTVEC+10: case loc_INTVEC+11:
	case loc_INTVEC+12: case loc_INTVEC+13: case loc_INTVEC+14:
		/* interrupt transfer vector 1 (15 words up to 00517) */
		sprintf(dst, "INTVEC+%#o", addr - loc_INTVEC);
		break;
	case loc_KBLK:		/* disk command block address */
		sprintf(dst, "KBLK");
		break;
	case loc_KSTAT:		/* disk status at start of current sector */
		sprintf(dst, "KSTAT");
		break;
	case loc_KADDR:		/* disk address of latest disk command */
		sprintf(dst, "KADDR");
		break;
	case loc_KSIBITS:	/* sector interrupt bit mask */
		sprintf(dst, "KSIBITS");
		break;
	case loc_ITTIME:	/* interval timer timer */
		sprintf(dst, "ITTIME");
		break;
	case loc_TRAPPC:	/* trap saved PC */
		sprintf(dst, "TRAPPC");
		break;
	case loc_TRAPVEC:	/* trap vectors (up to 0567) */
		sprintf(dst, "TRAPVEC");
		break;
	case loc_TIMERDATA:	/* timer data (OS; up to 0577) */
		sprintf(dst, "TIMERDATA");
		break;
	case loc_EPLOC:		/* ethernet post location */
		sprintf(dst, "EPLOC");
		break;
	case loc_EBLOC:		/* ethernet interrupt bitmask */
		sprintf(dst, "EBLOC");
		break;
	case loc_EELOC:		/* ethernet ending count */
		sprintf(dst, "EELOC");
		break;
	case loc_ELLOC:		/* ethernet load location */
		sprintf(dst, "ELLOC");
		break;
	case loc_EICLOC:	/* ethernet input buffer count */
		sprintf(dst, "EICLOC");
		break;
	case loc_EIPLOC:	/* ethernet input buffer pointer */
		sprintf(dst, "EIPLOC");
		break;
	case loc_EOCLOC:	/* ethernet output buffer count */
		sprintf(dst, "EOCLOC");
		break;
	case loc_EOPLOC:	/* ethernet output buffer pointer */
		sprintf(dst, "EOPLOC");
		break;
	case loc_EHLOC:		/* ethernet host address */
		sprintf(dst, "EHLOC");
		break;
	case loc_ERSVD:		/* reserved for ethernet expansion (up to 00612) */
		sprintf(dst, "ERSVD");
		break;
	case loc_ALTOV:		/* Alto I/II indication that microcode can
				 * interrogate (0 = Alto I, -1 = Alto II)
				 */
		sprintf(dst, "ALTOV");
		break;
	case loc_DCBR:		/* posted by parity task (main memory parity error) */
		sprintf(dst, "DCBR");
		break;
	case loc_KNMAR:		/* -"- */
		sprintf(dst, "KNMAR");
		break;
	case loc_CBA:		/* -"- */
		sprintf(dst, "CBA");
		break;
	case loc_SAD:		/* -"- */
		sprintf(dst, "SAD");
		break;
	case loc_UTILOUT: case loc_UTILOUT+1:
		/* printer output (up to 177017) */
		sprintf(dst, "UTILOUT+%d", addr - loc_UTILOUT);
		break;
	case loc_XBUS: case loc_XBUS+1: case loc_XBUS+2: case loc_XBUS+3:
		/* untility input bus (up to 177023) */
		sprintf(dst, "XBUS+%d", addr - loc_XBUS);
		break;
	case loc_MEAR:		/* memory error address register */
		sprintf(dst, "MEAR");
		break;
	case loc_MESR:		/* memory error status register */
		sprintf(dst, "MESR");
		break;
	case loc_MECR:		/* memory error control register */
		sprintf(dst, "MECR");
		break;
	case loc_UTILIN:	/* printer status, mouse keyset */
		sprintf(dst, "UTILIN");
		break;
	case loc_KBDAD: case loc_KBDAD+1: case loc_KBDAD+2: case loc_KBDAD+3:
		/* undecoded keyboard (up to 177037) */
		sprintf(dst, "KBDAD+%#o", addr - loc_KBDAD);
		break;
	case loc_BANKREGS:	/* extended memory option bank registers */
		sprintf(dst, "BANKREGS");
		break;
	default:
		snprintf(dst, sizeof(buff[0]), "%#o", addr);
	}
	return buff[which];
}

#define	emit(str) \
	dst += snprintf(dst, size-(size_t)(dst-buff), str)
#define	emit1(str,arg) \
	dst += snprintf(dst, size-(size_t)(dst-buff), str, arg)
#define	emit2(str,arg1,arg2) \
	dst += snprintf(dst, size-(size_t)(dst-buff), str, arg1, arg2)

int emu_dasm(char *buff, size_t size, int indent, int pc, unsigned int ir)
{
	char *dst = buff;
	int rc = 0;

	if (ram_data && is_data[pc]) {
		emit2("RAM%06o:\t%s\t\t; data", pc, symaddr(ir));
		return 0;
	}

	if (ir & 0100000) {
		/* arithmetic group */
		switch (ir & op_ARITH_MASK) {
			case op_COM:
				emit("\t\tCOM");
				break;
			case op_NEG:
				emit("\t\tNEG");
				break;
			case op_MOV:
				emit("\t\tMOV");
				break;
			case op_INC:
				emit("\t\tINC");
				break;
			case op_ADC:
				emit("\t\tADC");
				break;
			case op_SUB:
				emit("\t\tSUB");
				break;
			case op_ADD:
				emit("\t\tADD");
				break;
			case op_AND:
				emit("\t\tAND");
				break;
		}
		switch (ir & sh_MASK) {
			case 0:
				break;
			case sh_L:
				emit( "L");
				break;
			case sh_R:
				emit( "R");
				break;
			case sh_S:
				emit( "S");
				break;
		}
		switch (ir & cy_MASK) {
			case 0:
				break;
			case cy_Z:
				emit( "Z");
				break;
			case cy_O:
				emit( "O");
				break;
			case cy_C:
				emit( "C");
				break;
		}
		switch (ir & nl_MASK) {
			case 0:
				break;
			case nl_:
				emit( "#");
				break;
		}
		emit2("\t%d %d", (ir >> 13) & 3, (ir >> 11) & 3);
		switch (ir & sk_MASK) {
			case sk_NVR:	/* never skip */
				break;
			case sk_SKP:	/* always skip */
				emit(" SKP");
				rc = 1;
				break;
			case sk_SZC:	/* skip if carry result is zero */
				emit(" SZC");
				rc = 1;
				break;
			case sk_SNC:	/* skip if carry result is non-zero */
				emit(" SNC");
				rc = 1;
				break;
			case sk_SZR:	/* skip if 16-bit result is zero */
				emit(" SZR");
				rc = 1;
				break;
			case sk_SNR:	/* skip if 16-bit result is non-zero */
				emit(" SNR");
				rc = 1;
				break;
			case sk_SEZ:	/* skip if either result is zero */
				emit(" SEZ");
				rc = 1;
				break;
			case sk_SBN:	/* skip if both results are non-zero */
				emit(" SBN");
				rc = 1;
				break;
		}
	} else {
		int disp = ir & 0377;
		char e[128];
		int ea = -1;

		if (ir & ea_INDIRECT) {
			switch (ir & ea_MASK) {
				case ea_PAGE0:
					ea = disp & 0377;
					snprintf(e, sizeof(e), "@%s", page0(disp));
					if (ram_data) {
						/* tag RAM location as data */
						is_data[ea] = 1;
						snprintf(e, sizeof(e), "@RAM%06o", ea);
					}
					break;
				case ea_PCREL:
					ea = (pc + (signed char)disp) & 0177777;
					snprintf(e, sizeof(e), "@%s", pcrel(pc, disp));
					if (ram_data) {
						/* tag RAM location as data */
						is_data[ea] = 1;
						snprintf(e, sizeof(e), "@RAM%06o", ea);
					}
					break;
				case ea_AC2REL:
					snprintf(e, sizeof(e), "@%s,2", sdisp(disp));
					/* can't tag, since we don't know the contents of AC2 */
					break;
				case ea_AC3REL:
					snprintf(e, sizeof(e), "@%s,3", sdisp(disp));
					/* can't tag, since we don't know the contents of AC3 */
					break;
			}
		} else {
			switch (ir & ea_MASK) {
				case ea_PAGE0:
					ea = disp & 0377;
					snprintf(e, sizeof(e), "%s", page0(disp));
					break;
				case ea_PCREL:
					ea = (pc + (signed char)disp) & 0177777;
					snprintf(e, sizeof(e), "%s", pcrel(pc, disp));
					break;
				case ea_AC2REL:
					snprintf(e, sizeof(e), "%s,2", sdisp(disp));
					break;
				case ea_AC3REL:
					snprintf(e, sizeof(e), "%s,3", sdisp(disp));
					break;
			}
		}
		switch (ir & op_MFUNC_MASK) {
			case op_MFUNC_JUMP:
				switch (ir & op_JUMP_MASK) {
					case op_JMP:
						emit2("\t\t%sJMP\t%s", indent ? "  " : "", e);
						break;
					case op_JSR:
						emit2("\t\t%sJSR\t%s", indent ? "  " : "", e);
						break;
					case op_ISZ:
						if (ram_data && -1 != ea) {
							is_data[ea] = 1;
							snprintf(e, sizeof(e), "%sRAM%06o",
								(ir & ea_INDIRECT) ? "@" : "", ea);
						}
						emit1("\t\tISZ\t%s", e);
						rc = 1; 
						break;
					case op_DSZ:
						if (ram_data && -1 != ea) {
							is_data[ea] = 1;
							snprintf(e, sizeof(e), "%sRAM%06o",
								(ir & ea_INDIRECT) ? "@" : "", ea);
						}
						emit1("\t\tDSZ\t%s", e);
						rc = 1; 
						break;
				}
				break;
			case op_LDA:
				if (ram_data && -1 != ea) {
					is_data[ea] = 1;
					snprintf(e, sizeof(e), "%sRAM%06o",
						(ir & ea_INDIRECT) ? "@" : "", ea);
				}
				emit2("\t\tLDA\t%d %s", (ir >> 11) & 3, e);
				break;
			case op_STA:
				if (ram_data && -1 != ea) {
					is_data[ea] = 1;
					snprintf(e, sizeof(e), "%sRAM%06o",
						(ir & ea_INDIRECT) ? "@" : "", ea);
				}
				emit2("\t\tSTA\t%d %s", (ir >> 11) & 3, e);
				break;
			case op_AUGMENTED:
				switch (ir & op_AUGM_MASK) {
					case op_CYCLE:
						emit1("\t\tCYCLE\t%03o", ir & 017);
						break;
					case op_AUGM_NODISP:
						switch (ir & (op_AUGM_MASK | op_AUGM_SUBFUNC)) {
						case op_DIR:
							emit("\t\tDIR");
							break;
						case op_EIR:
							emit("\t\tEIR");
							break;
						case op_BRI:
							emit("\t\tBRI");
							break;
						case op_RCLK:
							emit("\t\tRCLK");
							break;
						case op_SIO:
							emit("\t\tSIO");
							break;
						case op_BLT:
							emit("\t\tBLT");
							break;
						case op_BLKS:
							emit("\t\tBLKS");
							break;
						case op_SIT:
							emit("\t\tSIT");
							break;
						case op_JMPRAM:
							emit("\t\tJMPRAM");
							break;
						case op_RDRAM:
							emit("\t\tRDRAM");
							break;
						case op_WRTRAM:
							emit("\t\tWRTRAM");
							break;
						case op_DIRS:
							emit("\t\tDIRS");
							rc = 1;	/* skips if interrupts were disabled */
							break;
						case op_VERS:
							emit("\t\tVERS");
							break;
						case op_DREAD:
							emit("\t\tDREAD");
							break;
						case op_DWRITE:
							emit("\t\tDWRITE");
							break;
						case op_DEXCH:
							emit("\t\tDEXCH");
							break;
						case op_MUL:
							emit("\t\tMUL");
							break;
						case op_DIV:
							emit("\t\tDIV");
							break;
						case op_DIAGNOSE1:
							emit("\t\tDIAGNOSE1");
							break;
						case op_DIAGNOSE2:
							emit("\t\tDIAGNOSE2");
							break;
						case op_BITBLT:
							emit("\t\tBITBLT");
							break;
						case op_XMLDA:
							emit("\t\tXMLDA");
							break;
						case op_XMSTA:
							emit("\t\tXMSTA");
							break;
						default:
							emit2("\t\t?%02o\t0%o", ir & op_AUGM_MASK, disp);
						}
						break;
					case op_JSRII:
						emit1("\t\tJSRII\t%s", pcrel(pc, disp));
						break;
					case op_JSRIS:
						emit1("\t\tJSRIS\t%s", sdisp(disp));
						break;
					case op_CONVERT:
						emit1("\t\tCONVERT\t%s,2", sdisp(disp));
						break;
					default:
						emit2("\t\t?%02o\t0%o", ir & op_AUGM_MASK, disp);
				}
				break;
		}
	}
	return rc;
}

int main(int argc, char **argv)
{
	char buff[512];
	char *fname = NULL;
	int pc, size;
	int i;

	memset(ram, 0, sizeof(ram));
	memset(is_data, 0, sizeof(is_data));

	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
			case 'a':	/* automagically detect data RAM */
				ram_data = 1;
				break;
			case 'b':	/* set base address for image */
				if (++i >= argc) {
					fprintf(stderr, "-b without base address\n");
					return 1;
				}
				start = strtoul(argv[i], NULL, 0);
				break;
			case 'd':	/* print numbers as decimal */
				base = 10;
				break;
			case 'x':	/* print numbers as hex */
				base = 16;
				break;
			case 'o':	/* print numbers as octal (default) */
				base = 8;
				break;
			case 's':	/* swab words */
				swab_words = 1;
				break;
			case 'r':	/* print PC relative as .+offs or .-offs */
				pcrel_mode = 1;
				break;
			}
		} else {
			fname = argv[i];
		}
	}

	if (!fname) {
		fprintf(stderr, "no filename specified\n");
		return 1;
	}

	size = load_ram(&ram[start], fname);
	if (swab_words) {
		for (i = 0; i < size; i++) {
			int data = ram[i];
			data = ((data << 8) | (data >> 8)) & 0xffff;
			ram[i] = data;
		}
	}

	if (size) {
		printf("\nRAM file %s (numbers are %s):\n",
			fname, base == 8 ? "octal" : base == 10 ? "decimal" : "hex");
		for (i = 0, pc = start; pc < start + size; pc++) {
			i = emu_dasm(buff, sizeof(buff), i, pc, ram[pc]);
			switch (base) {
			case 8:
				printf("%06o: %06o\t%s\n", pc, ram[pc], buff);
				break;
			case 10:
				printf("%05d: %05d\t%s\n", pc, ram[pc], buff);
				break;
			case 16:
				printf("%04x: %04x\t%s\n", pc, ram[pc], buff);
				break;
			}
		}
	}
	printf("\n");
	return 0;
}
