#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/*
 * Some macros to have easier access to the bit-reversed notation
 * that the Xerox Alto docs use all over the place.
 */
#define BITSHIFT(width,to) ((width)-1-(to))

#define BITMASK(width,from,to) (((1<<((to)+1-(from)))-1) << BITSHIFT(width,to))

#define	ALTO_EQU(reg,width,from,to,val) \
	(((reg) & BITMASK(width,from,to)) == \
		(((val) << BITSHIFT(width,to)) & BITMASK(width,from,to)))

#define	ALTO_GET(reg,width,from,to) \
	(((reg) & BITMASK(width,from,to)) >> BITSHIFT(width,to))

#define	ALTO_PUT(reg,width,from,to,val) do { \
	(reg) = ((reg) & ~BITMASK(width,from,to)) | \
		(((val) << BITSHIFT(width,to)) & BITMASK(width,from,to)); \
} while (0)

#define	CPU_BRANCH(or) do { \
	cpu.next2 |= or; \
} while (0)

/* width,from,to of the 16 bit instruction register */
#define	IR_ARITH(ir)	ALTO_GET(ir,16, 0, 0)
#define	IR_SrcAC(ir)	ALTO_GET(ir,16, 1, 2)
#define	IR_DstAC(ir)	ALTO_GET(ir,16, 3, 4)
#define	IR_AFunc(ir)	ALTO_GET(ir,16, 5, 7)
#define	IR_SH(ir)	ALTO_GET(ir,16, 8, 9)
#define	IR_CY(ir)	ALTO_GET(ir,16,10,11)
#define	IR_NL(ir)	ALTO_GET(ir,16,12,12)
#define	IR_SK(ir)	ALTO_GET(ir,16,13,15)

#define	IR_MFunc(ir)	ALTO_GET(ir,16, 1, 2)
#define	IR_JFunc(ir)	ALTO_GET(ir,16, 3, 4)
#define	IR_I(ir)	ALTO_GET(ir,16, 5, 5)
#define	IR_X(ir)	ALTO_GET(ir,16, 6, 7)
#define	IR_DISP(ir)	ALTO_GET(ir,16, 8,15)
#define	IR_AUGFUNC(ir)	ALTO_GET(ir,16, 3, 7)

#define	LOG(x)	logprintf x

typedef struct {
	int	next2;

}	cpu_t;

cpu_t	cpu;

typedef struct {
	int	ir;

}	emu_t;

emu_t	emu;

int gen = 1;

static void logprintf(int task, int level, const char *fmt, ...)
{
	va_list ap;

	if (level > 0)
		return;
	va_start(ap, fmt);
	vfprintf(stdout, fmt, ap);
	va_end(ap);
}

void f2_fn_idisp_late (void)
{
  int bval;
  /*
   * this all could be made more efficient by using an array
   * indexed by IR[0-9]
   */
  if (emu.ir & 0100000)
    bval = 3 - ((emu.ir >> 6) & 3);  /* complement of SH field of IR */
  else if ((emu.ir & 0060000) == 0)
    bval = (emu.ir >> 11) & 3;  /* JMP, JSR, ISZ, DSZ */
  else if ((emu.ir & 0060000) == 0020000)
    bval = 4;  /* LDA */
  else if ((emu.ir & 0060000) == 0040000)
    bval = 5;  /* STA */
  else if ((emu.ir & 0007400) == 0)
    bval = 1;
  else if ((emu.ir & 0007400) == 0000400)
    bval = 0;
  else if ((emu.ir & 0007400) == 0003000)
    bval = 016;  /* CONVERT */
  else if ((emu.ir & 0007400) == 0007000)
    bval = 6;
  else
    bval = (emu.ir >> 8) & 017;
  cpu.next2 |= bval;
}

/**
 * @brief f2_idisp late: branch on: arithmetic IR_SH, others PROM ctl2k_u3[IR[1-7]]
 */
static void f2_idisp_1(void)
{
	int or;

	if (IR_ARITH(emu.ir)) {                                
		/* 1xxxxxxxxxxxxxxx arithmetical group */
		or = IR_SH(emu.ir) ^ 3;       /* complement of SH */
		LOG((0,2,"	IDISP<-; IR (%#o) branch on SH^3 (%#o|%#o)\n",
		       	emu.ir, cpu.next2, or));
	} else if (IR_MFunc(emu.ir) == 0) {            
		/* 000xxxxxxxxxxxxx jump group */
		or = IR_JFunc(emu.ir); /* JMP, JSR, ISZ, DSZ */
		LOG((0,2,"	IDISP<-; IR (%#o) branch on JFunc (%#o|%#o)\n",
		       	emu.ir, cpu.next2, or));
	} else if (IR_MFunc(emu.ir) == 1) {            
		/* 001xxxxxxxxxxxxx LDA */
		or = 4;
		LOG((0,2,"	IDISP<-; IR (%#o) branch on LDA (%#o|%#o)\n",
			emu.ir, cpu.next2, or));
	} else if (IR_MFunc(emu.ir) == 2) {            
		/* 010xxxxxxxxxxxxx STA */
		or = 5;
		LOG((0,2,"	IDISP<-; IR (%#o) branch on STA (%#o|%#o)\n",
			emu.ir, cpu.next2, or));
	} else if (ALTO_GET(emu.ir,16,4,7) == 0) {     
		/* 011x0000xxxxxxxx CYCLE */
		or = 1;
		LOG((0,2,"	IDISP<-; IR (%#o) branch on CYCLE (%#o|%#o)\n",
			emu.ir, cpu.next2, or));
	} else if (ALTO_GET(emu.ir,16,4,7) == 1) {     
		/* 011x0001xxxxxxxx JSRII */
		or = 0;
		LOG((0,2,"	IDISP<-; IR (%#o) branch on JSRII (%#o|%#o)\n",
			emu.ir, cpu.next2, or));
	} else if (ALTO_GET(emu.ir,16,4,7) == 6) {     
		/* 011x0110xxxxxxxx JSRIS */
		or = 016;
		LOG((0,2,"	IDISP<-; IR (%#o) branch on JSRIS (%#o|%#o)\n",
			emu.ir, cpu.next2, or));
	} else if (ALTO_GET(emu.ir,16,4,7) == 016) {   
		/* 011x1110xxxxxxxx CONVERT */
		or = 6;
		LOG((0,2,"	IDISP<-; IR (%#o) branch on CONVERT (%#o|%#o)\n",
			emu.ir, cpu.next2, or));
	} else {                                       
		/* 011-????-------- */
		or = ALTO_GET(emu.ir,16,4,7);         /* others and TRAPS */
		LOG((0,2,"	IDISP<-; IR (%#o) branch on IR[4-7] (%#o|%#o)\n",
			emu.ir, cpu.next2, or));
	}
	CPU_BRANCH(or);
}

static void f2_fn_acsource_late(void)
{
  int bval;
  if (emu.ir & 0100000)
    bval = 3 - ((emu.ir >> 6) & 3);  /* complement of SH field */
  else if ((emu.ir & 0060000) != 0060000)
    bval = (emu.ir >> 10) & 1;  /* indemu.irect bit */
  else
    switch ((emu.ir >> 8) & 037)
      {
      case 000:  bval = 002;  break;  /* CYCLE */
      case 001:  bval = 005;  break;  /* RAMTRAP */
      case 002:  bval = 003;  break;  /* NOPAR -- parameterless group */
      case 003:  bval = 006;  break;  /* RAMTRAP */
      case 004:  bval = 007;  break;  /* RAMTRAP */
      case 011:  bval = 004;  break;  /* JSRII */
      case 012:  bval = 004;  break;  /* JSRIS */
      case 016:  bval = 001;  break;  /* CONVERT */
      case 037:  bval = 017;  break;  /* ROMTRAP - used by Swat debugger */
      default:   bval = 016;  break;  /* RAMTRAP */
      }
  cpu.next2 |= bval;
}

/**
 * @brief f2_acsource late: branch on: arithmetic IR_SH, others PROM ctl2k_u3[IR[1-7]]
 */
static void f2_acsource_1(void)
{
	int or;
	if (IR_ARITH(emu.ir)) {
		/* 1xxxxxxxxxxxxxxx */
		or = IR_SH(emu.ir) ^ 3;                       /* complement of SH */
		LOG((0,2,"	<-ACSOURCE; branch on SH^3 (%#o|%#o)\n",
			cpu.next2, or));
	} else if (IR_MFunc(emu.ir) != 3) {
		/* not 011xxxxxxxxxxxxx */
		or = IR_I(emu.ir);                    /* I bit (indirect) */
		LOG((0,2,"	<-ACSOURCE; branch on I (%#o|%#o)\n",
			cpu.next2, or));
	} else {
		/* 011?????xxxxxxxx */
		switch (IR_AUGFUNC(emu.ir)) {
		case 000: /* 01100000xxxxxxxx 060000-060377 CYCLE */
			or = 002;
			LOG((0,2,"	<-ACSOURCE; branch on CYCLE (%#o|%#o)\n",
				cpu.next2, or));
			break;
		case 001: /* 01100001xxxxxxxx 060400-060777 RAMTRAP */
			or = 005;
			LOG((0,2,"	<-ACSOURCE; branch on RAMTRAP (%#o|%#o)\n",
				cpu.next2, or));
			break;
		case 002: /* 01100010xxxxxxxx 061000-061377 NOPAR */
			or = 003;
			LOG((0,2,"	<-ACSOURCE; branch on NOPAR (%#o|%#o)\n",
				cpu.next2, or));
			break;
		case 003: /* 01100011xxxxxxxx 061400-061777 RAMTRAP */
			or = 006;
			LOG((0,2,"	<-ACSOURCE; branch on RAMTRAP (%#o|%#o)\n",
				cpu.next2, or));
			break;
		case 004: /* 01100100xxxxxxxx 062000-062377 RAMTRAP */
			or = 007;
			LOG((0,2,"	<-ACSOURCE; branch on RAMTRAP (%#o|%#o)\n",
				cpu.next2, or));
			break;
		case 011: /* 01101001xxxxxxxx 064400-064777 JSRII */
			or = 004;
			LOG((0,2,"	<-ACSOURCE; branch on JSRII (%#o|%#o)\n",
				cpu.next2, or));
			break;
		case 012: /* 01101010xxxxxxxx 065000-065377 JSRIS */
			or = 004;
			LOG((0,2,"	<-ACSOURCE; branch on JSRIS (%#o|%#o)\n",
				cpu.next2, or));
			break;
		case 016: /* 01101110xxxxxxxx 067000-067377 CONVERT */
			or = 001;
			LOG((0,2,"	<-ACSOURCE; branch on CONVERT (%#o|%#o)\n",
				cpu.next2, or));
			break;
		case 037: /* 01111111xxxxxxxx 077400-077777 ROMTRAP */
			or = 017;
			LOG((0,2,"	<-ACSOURCE; branch on ROMTRAP (%#o|%#o)\n",
				cpu.next2, or));
			break;
		default:  /* 011?????xxxxxxxx more RAMTRAPs */
			or = 016;
			LOG((0,2,"	<-ACSOURCE; branch on RAMTRAP (%#o|%#o)\n",
				cpu.next2, or));
		}
	}
	CPU_BRANCH(or);
}

int aswap(int addr)
{
	addr = ((addr & 0xaa) >> 1) | ((addr & 0x55) << 1);
	addr = ((addr & 0xcc) >> 2) | ((addr & 0x33) << 2);
	addr = ((addr & 0xf0) >> 4) | ((addr & 0x0f) << 4);
	return addr;
}

int main(int argc, char **argv)
{
	unsigned char prom[256];
	int i, n;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-salto"))
			gen = 1;
		if (!strcmp(argv[i], "-altogether"))
			gen = 2;
	}

	printf("branch PROM 2kctl.u3 according to ");
	switch (gen) {
	case 1: printf("SALTO"); break;
	case 2: printf("Altogether"); break;
	}

	for (i = 0000000, n = 0; i < 0100000; i += 0000400, n++) {
		emu.ir = i;
		cpu.next2 = 0;
		switch (gen) {
		case 1:
			f2_acsource_1();
			break;
		case 2:
			f2_fn_acsource_late();
			break;
		}
		prom[aswap(n) ^ 0377] = cpu.next2 ^ 017;
		if (n % 16)
			printf(" %03o", cpu.next2);
		else
			printf("\n%04o: %03o", n, cpu.next2);
	}
	for (i = 0000000; i < 0100000; i += 0000400, n++) {
		emu.ir = i;
		cpu.next2 = 0;
		switch (gen) {
		case 1:
			f2_idisp_1();
			break;
		case 2:
			f2_fn_idisp_late();
			break;
		}
		prom[aswap(n) ^ 0377] = cpu.next2 ^ 017;
		if (n % 16)
			printf(" %03o", cpu.next2);
		else
			printf("\n%04o: %03o", n, cpu.next2);
	}
	printf("\n");
	for (n = 0; n < 256; n++)
		if (n % 16)
			printf(" %03o", prom[n]);
		else
			printf("\n%04o: %03o", n, prom[n]);
	return 0;
}
