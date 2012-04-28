%{
/**********************************************************
 * Xerox Alto Assembler.
 *
 * Parser of the Xerox Alto assembler.
 *
 * (C) 2007 by Juergen Buchmueller <pullmoll@t-online.de>
 *
 * $Id: aasm.y,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 **********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "emu_ops.h"

typedef unsigned char   byte;
typedef	unsigned short	word;

typedef	struct	tagSYM {
	struct tagSYM * next;
	int pass_no;
	int value;
	int refs;
	int *line_no;
	char *name;
}   SYM_t;

void emit(void);

void addsym(char * name, int value);
int getsym(char * name);

word *image = NULL;

int symb_no = 0;
int line_no = 1;
int pass_no = 1;
word cmd = 0x00;
int arg[4096] = {0, };
int expflg = 0;	/* print expression result */
int cmdflg = 0;
int argcnt = 0;
int arglow = 0;
int padflg = 0;	/* pad binary to new address */
word eexp = 0;
word end = 0;
word tmp = 0;
int pc	= 0;
int pcmax = -1;
int alloc = -1;

char lstbuff[256+1] = "";
char *plst  = NULL;

SYM_t *sym = NULL;
SYM_t *sym0 = NULL;

char inpname[256+1] = "";
char outname[256+1] = "";
char lstname[256+1] = "";

FILE *inpfile = NULL;
FILE *outfile = NULL;
FILE *lstfile = NULL;

int f_dump  = 0;		/* generate octal dump as output */
int f_list  = 0;		/* write listing file */
int f_syms  = 0;		/* write symbols into listing file */
int f_xref  = 0;		/* write cross references with symbols */
int f_octal = 1;		/* print numbers as octal in listing (default) */

extern void yyerror(const char *msg, ...);
extern int yylex(void);
extern void yyrestart(FILE *inputfile);

%}

%start	file

%union {
	long long int i;
	char *s;
}

%token		EOL HASH COMMA COLON LPAREN RPAREN QUOTE PLUS MINUS ASTERISK SLASH
%token		SHL SHR AMPERSAND BAR CARET TILDE AT

%token	<i>	IMM
%token	<i>	REG
%token	<s>	SYM
%token	<s>	CHR
%token	<i>	ARITH

%token		AC
%token		SKP SZC SNC SZR SNR SEZ SBN
%token		JMP JSR DSZ ISZ LDA STA
%token		COM NEG MOV INC ADC SUB ADD AND 
%token		CYCLE MUL DIV JSRII JSRIS CONVERT
%token		DIR EIR BRI RCLK SIO BLT BLKS SIT JMPRAM RDRAM WRTRAM
%token		DIRS VERS DREAD DWRITE DEXCH DIAGNOSE1 DIAGNOSE2
%token		BITBLT XMLDA XMSTA

%token		DB DW DD DS
%token		ALIGN END EQU EVEN ORG

%type	<i>	expr simple term factor arith SrcAC DstAC ea ac23 skip
%type	<i>	imm4 disp rel8

%%

file	:	line
	|	line error
	;

line	:	line token { emit(); }
	|	token { emit(); }
	;

token	:							EOL
		{
			cmdflg = 0;
		}
	|	symdef						EOL
		{
			cmdflg = 0;
			expflg = 1;
			eexp = pc;
		}
	|	symdef	imm16list				EOL
		{
			cmdflg = 0;
		}
	|	SYM	EQU expr				EOL
		{
			cmdflg = 0;
			argcnt = 0;
			expflg = 1;
			eexp = $3;
			addsym($1, $3);
			free($1);
		}
	|	symdef	JMP	ea					EOL
		{ cmd |= op_JMP | ea_DIRECT | $3; } 
	|	symdef	JMP	AT ea					EOL
		{ cmd |= op_JMP | ea_INDIRECT | $4; } 
	|	symdef	JSR	ea					EOL
		{ cmd |= op_JSR | ea_DIRECT | $3; } 
	|	symdef	JSR	AT ea					EOL
		{ cmd |= op_JSR | ea_INDIRECT | $4; } 
	|	symdef	ISZ	ea					EOL
		{ cmd |= op_ISZ | ea_DIRECT | $3; } 
	|	symdef	ISZ	AT ea					EOL
		{ cmd |= op_ISZ | ea_INDIRECT | $4; } 
	|	symdef	DSZ	ea					EOL
		{ cmd |= op_DSZ | ea_DIRECT | $3; } 
	|	symdef	DSZ	AT ea					EOL
		{ cmd |= op_DSZ | ea_INDIRECT | $4; } 

	|	symdef	LDA	DstAC ea				EOL
		{ cmd |= op_LDA | ea_DIRECT | $3 | $4; } 
	|	symdef	LDA	DstAC AT ea				EOL
		{ cmd |= op_LDA | ea_INDIRECT | $3 | $5; } 
	|	symdef	STA	DstAC ea				EOL
		{ cmd |= op_STA | ea_DIRECT | $3 | $4; } 
	|	symdef	STA	DstAC AT ea				EOL
		{ cmd |= op_STA | ea_INDIRECT | $3 | $5; } 

	|	symdef	COM arith	SrcAC DstAC skip		EOL
		{ cmd |= op_COM | $3 | $4 | $5 | $6; } 
	|	symdef	NEG arith	SrcAC DstAC skip		EOL
		{ cmd |= op_NEG | $3 | $4 | $5 | $6; } 
	|	symdef	MOV arith	SrcAC DstAC skip		EOL
		{ cmd |= op_MOV | $3 | $4 | $5 | $6; } 
	|	symdef	INC arith	SrcAC DstAC skip		EOL
		{ cmd |= op_INC | $3 | $4 | $5 | $6; } 
	|	symdef	ADC arith	SrcAC DstAC skip		EOL
		{ cmd |= op_ADC | $3 | $4 | $5 | $6; } 
	|	symdef	SUB arith	SrcAC DstAC skip		EOL
		{ cmd |= op_SUB | $3 | $4 | $5 | $6; } 
	|	symdef	ADD arith	SrcAC DstAC skip		EOL
		{ cmd |= op_ADD | $3 | $4 | $5 | $6; } 
	|	symdef	AND arith	SrcAC DstAC skip		EOL
		{ cmd |= op_AND | $3 | $4 | $5 | $6; } 

	|	symdef	CYCLE	imm4					EOL
		{ cmd |= op_CYCLE | $3; }
	|	symdef	MUL						EOL
		{ cmd |= op_MUL; }
	|	symdef	DIV						EOL
		{ cmd |= op_DIV; }
	|	symdef	JSRII	disp					EOL
		{ cmd |= op_JSRII | $3; }
	|	symdef	JSRIS	rel8					EOL
		{ cmd |= op_JSRIS | $3; }
	|	symdef	CONVERT	rel8 ac2				EOL
		{ cmd |= op_CONVERT | $3; }
	|	symdef	CONVERT	rel8 COMMA ac2				EOL
		{ cmd |= op_CONVERT | $3; }
	|	symdef	DIR						EOL
		{ cmd |= op_DIR; }
	|	symdef	EIR						EOL
		{ cmd |= op_EIR; }
	|	symdef	BRI						EOL
		{ cmd |= op_BRI; }
	|	symdef	RCLK						EOL
		{ cmd |= op_RCLK; }
	|	symdef	SIO						EOL
		{ cmd |= op_SIO; }
	|	symdef	BLT						EOL
		{ cmd |= op_BLT; }
	|	symdef	BLKS						EOL
		{ cmd |= op_BLKS; }
	|	symdef	SIT						EOL
		{ cmd |= op_SIT; }
	|	symdef	JMPRAM						EOL
		{ cmd |= op_JMPRAM; }
	|	symdef	RDRAM						EOL
		{ cmd |= op_RDRAM; }
	|	symdef	WRTRAM						EOL
		{ cmd |= op_WRTRAM; }
	|	symdef	DIRS						EOL
		{ cmd |= op_DIRS; }
	|	symdef	VERS						EOL
		{ cmd |= op_VERS; }
	|	symdef	DREAD						EOL
		{ cmd |= op_DREAD; }
	|	symdef	DWRITE						EOL
		{ cmd |= op_DWRITE; }
	|	symdef	DEXCH						EOL
		{ cmd |= op_DEXCH; }
	|	symdef	DIAGNOSE1					EOL
		{ cmd |= op_DIAGNOSE1; }
	|	symdef	DIAGNOSE2					EOL
		{ cmd |= op_DIAGNOSE2; }
	|	symdef	BITBLT						EOL
		{ cmd |= op_BITBLT; }
	|	symdef	XMLDA						EOL
		{ cmd |= op_XMLDA; }
	|	symdef	XMSTA						EOL
		{ cmd |= op_XMSTA; }

	|	symdef	DB	imm8list				EOL
		{ cmdflg = 0; }
	|	symdef	DB	strlist					EOL
		{ cmdflg = 0; }
	|	symdef	DW	imm16list				EOL
		{ cmdflg = 0; }
	|	symdef	DD	imm32list				EOL
		{ cmdflg = 0; }
	|	symdef	DS	imm8					EOL
		{
			cmdflg = 0;
			argcnt = arg[0];
			if (argcnt > sizeof(arg)/sizeof(arg[0])) {
				fprintf(stderr, "line (%d) warning: size clipped to assembler limits (%d : %d)\n", line_no, argcnt, sizeof(arg));
				argcnt = sizeof(arg)/sizeof(arg[0]);
			}
			memset(arg, 0, argcnt * sizeof(arg[0]));
		}
	|		ORG	expr					EOL
		{
			cmdflg = 0;
			argcnt = 0;
			expflg = 1;
			eexp = $2;
			pc = eexp;
		}
	|	symdef	ALIGN	IMM					EOL
		{
			cmdflg = 0;
			argcnt = 0;
			expflg = 1;
			eexp = pc;
			tmp = $3 - 1;
			while (((pc + argcnt) & tmp) && argcnt < (sizeof(arg)/sizeof(arg[0])))
				arg[argcnt++] = 0;
		}
	|	symdef	EVEN						EOL
		{
			cmdflg = 0;
			argcnt = 0;
			expflg = 1;
			eexp = pc;
			if (pc & 1)
				arg[argcnt++] = 0;
		}
	|	symdef	END	imm16					EOL
		{
			cmdflg = 0;
			argcnt = 0;
			expflg = 1;
			eexp = arg[0];
			end = eexp;
		}
	;

symdef	:	/* may be empty */
	|	SYM COLON
		{
			addsym($1, pc);
			free($1);
		}
	;

arith	:	/* may be empty */
		{
			$$ = 0;
		}
	|	ARITH
		{
			$$ = $1;
		}
	;
		

skip	:	/* may be empty */
		{
			$$ = 0;
		}
	|	SKP
		{
			$$ = sk_SKP;
		}
	|	SZC
		{
			$$ = sk_SZC;
		}
	|	SNC
		{
			$$ = sk_SNC;
		}
	|	SZR
		{
			$$ = sk_SZR;
		}
	|	SNR
		{
			$$ = sk_SNR;
		}
	|	SEZ
		{
			$$ = sk_SEZ;
		}
	|	SBN
		{
			$$ = sk_SBN;
		}
	;

ac2	:	expr
		{
			if ($1 != 2) {
				fprintf(stderr, "line (%d) fatal: only AC(2) can be base register\n",
					line_no);
				exit(1);
			}
		}	
	|	AC LPAREN expr RPAREN
		{
			if ($3 != 2) {
				fprintf(stderr, "line (%d) fatal: only AC(2) can be base register\n",
					line_no);
				exit(1);
			}
		}	
	;

ac23	:	expr
		{
			if ($1 < 2 || $1 > 3) {
				fprintf(stderr, "line (%d) fatal: only AC(2) or AC(3) can be base register\n",
					line_no);
				exit(1);
			}
			$$ = ($1 == 2) ? ea_AC2REL : ea_AC3REL;
		}	
	|	AC LPAREN expr RPAREN
		{
			if ($3 < 2 || $3 > 3) {
				fprintf(stderr, "line (%d) fatal: only AC(2) or AC(3) can be base register\n",
					line_no);
				exit(1);
			}
			$$ = ($3 == 2) ? ea_AC2REL : ea_AC3REL;
		}	
	;

SrcAC	:	expr
		{
			if ($1 < 0 || $1 > 3) {
				fprintf(stderr, "line (%d) fatal: invalid source AC(%lld)\n",
					line_no, $1);
				exit(1);
			}
			$$ = $1 << 13;
		}
	|	AC LPAREN IMM RPAREN
		{
			if ($3 < 0 || $3 > 3) {
				fprintf(stderr, "line (%d) fatal: invalid source AC(%lld)\n",
					line_no, $3);
				exit(1);
			}
			$$ = $3 << 13;
		}
	;

DstAC	:	expr
		{
			if ($1 < 0 || $1 > 3) {
				fprintf(stderr, "line (%d) fatal: invalid destination AC(%lld)\n",
					line_no, $1);
				exit(1);
			}
			$$ = $1 << 11;
		}
	|	AC LPAREN expr RPAREN
		{
			if ($3 < 0 || $3 > 3) {
				fprintf(stderr, "line (%d) fatal: invalid destination AC(%lld)\n",
					line_no, $3);
				exit(1);
			}
			$$ = $3 << 11;
		}
	;

ea	:	expr
		{
			if ($1 >= 0 && $1 < 256) {
				/* page 0 address */
				$$ = $1;
			} else {
				int rel = $1 - pc - argcnt;
				/* PC relative */
				if (pass_no > 1 && (rel < -128 || rel > 127))
					fprintf(stderr, "line (%d) warning: displacement range (%lld, %d)\n",
						line_no, $1, rel);
				$$ = ea_PCREL | (rel & 255);
			}
		}
	|	expr ac23
		{
			int rel = $1;
			if (rel < -128 || rel > 127)
				fprintf(stderr, "line (%d) warning: displacement range (%lld, %d)\n",
					line_no, $1, rel);
			$$ = $2 | (rel & 255);
		}
	|	expr COMMA ac23
		{
			int rel = $1;
			if (rel < -128 || rel > 127)
				fprintf(stderr, "line (%d) warning: displacement range (%lld, %d)\n",
					line_no, $1, rel);
			$$ = $3 | (rel & 255);
		}
	;

disp	:	expr
		{
			int rel = $1 - pc - argcnt;
			if (pass_no > 1 && (rel < -128 || rel > 127))
				fprintf(stderr, "line (%d) warning: displacement out of range (%lld)\n",
					line_no, $1 - pc - 1);
			cmd |= rel & 255;
		}
	;

rel8	:	expr
		{
			int rel = $1;
			if (pass_no > 1 && (rel < -128 || rel > 127))
				fprintf(stderr, "line (%d) warning: displacement out of range (%lld)\n",
					line_no, $1 - pc - 1);
			cmd |= rel & 255;
		}
	;

expr	:	simple
		{ $$ = $1; }
	|	expr SHL simple
		{ $$ = $1 << $3; }
	|	expr SHR simple
		{ $$ = $1 >> $3; }
	;

simple	:	term
		{ $$ = $1; }
	|	simple PLUS term
		{ $$ = $1 + $3; }
	|	simple MINUS term
		{ $$ = $1 - $3; }
	|	simple AMPERSAND term
		{ $$ = $1 & $3; }
	;

term	:	factor
		{ $$ = $1; }
	|	PLUS factor
		{ $$ = $2; }
	|	MINUS factor
		{ $$ = -$2; }
	|	TILDE factor
		{ $$ = ~$2; }
	|	term ASTERISK factor
		{ $$ = $1 * $3; }
	|	term SLASH factor
		{ $$ = $1 / $3; }
	|	term BAR factor
		{ $$ = $1 | $3; }
	|	term CARET factor
		{ $$ = $1 ^ $3; }
	;

factor	:	IMM
		{ $$ = $1; }
	|	SYM
		{ $$ = getsym($1); free($1); }
	|	LPAREN expr RPAREN
		{ $$ = $2; }
	;

imm4	:	expr
		{
			if ($1 < 0 || $1 > 15)
				fprintf(stderr, "line (%d) warning: bad cycle count (%lld)\n",
					line_no, $1);
			$$ = $1 & 15;
		}
	;

imm8	:	expr
		{
			if ($1 < -128 || $1 > 255)
				fprintf(stderr, "line (%d) warning: size exceeds 8 bits (%lld)\n",
					line_no, $1);
			if (arglow)
				arg[argcnt++] |= $1 << 8;
			else
				arg[argcnt] = $1;
			arglow ^= 1;
		}
	;

imm8list	:	imm8list COMMA imm8
		|	imm8
	;

imm16	:	expr
		{
			if ($1 < -32768 || $1 > 65535)
				fprintf(stderr, "line (%d) warning: size exceeds 16 bits (%lld)\n",
					line_no, $1);
			arg[argcnt++] = $1;
		}
	;

imm16list	:	imm16list COMMA imm16
		|	imm16
	;

imm32	:	expr
		{
			if ($1 < -2147483648ll || $1 > 21474836647ll)
				fprintf(stderr, "line (%d) warning: size exceeds 32 bits (%lld)\n",
					line_no, $1);
			arg[argcnt++] = $1 % 65536;
			arg[argcnt++] = $1 / 65536;
		}
	;

imm32list	:	imm32list COMMA imm32
		|	imm32
	;

str	:	CHR
		{
			int len = strlen($1);
			int i;
			for (i = 0; i < len; i++) {
				if (arglow)
					arg[argcnt++] |= $1[i] << 8;
				else
					arg[argcnt] = $1[i];
				arglow ^= 1;
			}
			free($1);
		}
	|	imm8
	;

strlist		:	strlist COMMA str
		|	str
	;

%%

int list(char * fmt, ...)
{
	va_list arg;
	int rc = 0;

	if (!f_list)
		return rc;
	if (pass_no < 2)
		return rc;
	va_start(arg, fmt);
	rc = vfprintf(lstfile, fmt, arg);
	va_end(arg);
	return rc;
}

int outw(word c)
{
	int rc = 0;
	if (pass_no == 1) {
		pc++;
		return rc;
	}
	if (f_octal)
		rc = list("%06o ", c);
	else
		rc = list("%04x ", c);
	if (f_dump) {
		if (pc != pcmax || 0 == (pcmax % 4)) {
			if (-1 != pcmax)
				fprintf(outfile, "\n");
			fprintf(outfile, "%07o:", pc);
			pcmax = pc;
		}
		fprintf(outfile, " %07o", c);
		pcmax = ++pc;
	} else {
		if (pc > alloc) {
			alloc = (pc | 0xffff) + 1;
			image = (word *)realloc(image, alloc * sizeof(word));
			if (!image) {
				fprintf(stderr, "fatal: could not realloc() %d words of memory\n",
					alloc);
				exit(1);
			}
		}
		image[pc] = c;
		pc++;
		if (pc > pcmax)
			pcmax = pc;
	}
	return rc;
}

void emit(void)
{
	int i;
	int x = 0;

	if (arglow) {
		argcnt++;
		arglow = 0;
	}

	if (cmdflg || argcnt) {
		if (f_octal)
			x += list("%06o: ", pc);
		else
			x += list("%04x: ", pc);
	}

	if (cmdflg)
		x += outw(cmd);
	for (i = 0; i < argcnt && i < 4; i++)
		x += outw(arg[i]);
	if (strchr(lstbuff, '\n')) {
		if (!(cmdflg || argcnt) && expflg) {
			if (f_octal)
				x += list("=%06o ", eexp);
			else
				x += list("=%04x ", eexp);
		}
		if (x >= 0 && x < 24)
			list("%*s", 24 - x, " ");
		list("%s", lstbuff);
	}
	if (argcnt > 4) {
		for (i = 4; i < argcnt; i++) {
			if ((i & 3) == 0) {
				if (f_octal)
					x = list("%06o: ", pc);
				else
					x = list("%04x: ", pc);
			}
			x += outw(arg[i]);
			if ((i & 3) == 3)
				x += list("\n");
		}
		if (i & 3)
			x += list("\n");
	}
	cmd = 0;
	arg[0] = 0;
	expflg = 0;
	cmdflg = 1;
	argcnt = 0;
	arglow = 0;
}

void addsym(char * name, int value)
{
	SYM_t *s, *s0, *s1;
	for (s = sym; (s); s = s->next) {
		if (strcasecmp(name, s->name) == 0) {
			if (s->pass_no == pass_no)
				fprintf(stderr, "line (%d) warning: double defined symbol %s\n",
					line_no, name);
			s->line_no[0] = line_no;
			s->pass_no = pass_no;
			if (s->value != value)
				fprintf(stderr, "line (%d) warning: %s has different value on pass 2\n",
					line_no, name);
			s->value   = value;
			return;
		}
	}
	for (s0 = NULL, s1 = sym; (s1); s0 = s1, s1 = s1->next)
		if (strcasecmp(name, s1->name) <= 0)
			break;
	s = (SYM_t *) calloc(1, sizeof(SYM_t));
	if (!s) {
		fprintf(stderr, "error: out of memory!\n");
		exit(1);
	}
	s->next = s1;
	s->pass_no = pass_no;
	s->refs = 0;
	s->line_no = malloc(sizeof(int));
	*s->line_no = line_no;
	s->value = value;
	s->name = strdup(name);
	if (s0)
		s0->next = s;
	else
		sym = s;
	symb_no++;
}

int getsym(char * name)
{
	SYM_t *s;
	for (s = sym; (s); s = s->next) {
		if (strcasecmp(name, s->name) == 0) {
			if (pass_no > 1) {
				s->refs += 1;
				s->line_no = (int *) realloc(s->line_no, (s->refs + 1) * sizeof(int));
				s->line_no[s->refs] = line_no;
			}
			return s->value;
		}
	}
	if (pass_no > 1)
		fprintf(stderr, "line (%d) undefined symbol: %s\n",
			line_no, name);
	return 0;
}

void prtsym(FILE * filp)
{
	SYM_t * s;
	int i;
	for (s = sym; (s); s = s->next) {
		fprintf(filp, "%-32.32s%04X line %d, %d references\n",
			s->name, s->value, s->line_no[0], s->refs);
		if (f_xref) {
			if (s->refs > 0) {
				for (i = 0; i < s->refs; i++) {
					if ((i & 7) == 0)
						fprintf(filp, "\t");
					if (i > 0)
						fprintf(filp, ",");
					fprintf(filp, "%d", s->line_no[i+1]);
					if ((i & 7) == 7)
						fprintf(filp,"\n");
					}
				if (i & 7)
					fprintf(filp, "\n");
			}
		}
	}
}

int main(int ac, char ** av)
{
	int i;
	char * p;

	if (ac < 2) {
		fprintf(stderr, "usage:\t%s [options] input[.asm] [output[.bin]] [filename[.lst]]\n", av[0]);
		fprintf(stderr, "options can be one or more of:\n");
		fprintf(stderr, "-a\tgenerate octal dump instead of binary\n");
		fprintf(stderr, "-l\tgenerate listing (to file input.lst)\n");
		fprintf(stderr, "-s\toutput symbol table\n");
		fprintf(stderr, "-o\tprint octal addresses and opcodes in listing\n");
		fprintf(stderr, "-x\toutput cross reference with symbol table (implies -s)\n");
#if	ASM_DEBUG
		fprintf(stderr, "-d\tgenerate parser debug output to stderr\n");
#endif
		exit(1);
	}
	for (i = 1; i < ac; i++) {
		if (av[i][0] == '-') {
			switch (av[i][1]) {
			case 'a':
				f_dump = 1;
				break;
			case 'l':
				f_list = 1;
				break;
			case 'h':
				f_octal = 0;
				break;
			case 's':
				f_syms = 1;
				break;
			case 'x':
				f_syms = 1;
				f_xref = 1;
				break;
			case 'o':
				if (++i >= ac) {
					fprintf(stderr, "-o output filename missing\n");
					exit(1);
				}
				snprintf(outname, sizeof(outname), "%s", av[i]);
				break;
#if	ASM_DEBUG
			case 'd':
				yydebug = 1;
				break;
#endif
			default:
				fprintf(stderr, "illegal option %s\n", av[i]);
				exit(1);
			}
		} else if (!strlen(inpname)) {
			snprintf(inpname, sizeof(inpname), "%s", av[i]);
		} else if (!strlen(outname)) {
			snprintf(outname, sizeof(outname), "%s", av[i]);
		} else if (!strlen(lstname)) {
			snprintf(lstname, sizeof(lstname), "%s", av[i]);
		} else {
			fprintf(stderr, "additional argument %s ignored\n", av[i]);
		}
	}
	if (!strlen(inpname)) {
		fprintf(stderr, "input filename missing!\n");
		exit(1);
	}
	if (!strlen(outname)) {
		strcpy(outname, inpname);
		p = strrchr(outname, '.');
		if (f_dump) {
			if (p)
				strcpy(p, ".oct");
			else
				strcat(outname, ".oct");
		} else {
			if (p)
				strcpy(p, ".bin");
			else
				strcat(outname, ".bin");
		}
	}
	if (f_list) {
		if (!strlen(lstname)) {
			strcpy(lstname, inpname);
			p = strrchr(lstname, '.');
			if (p)
				strcpy(p, ".lst");
			else
				strcat(lstname, ".lst");
		}
	} else if (strlen(lstname)) {
		f_list = 1;
		p = strrchr(lstname, '.');
		if (p)
			strcpy(p, ".lst");
		else
			strcat(lstname, ".lst");
	}
	p = strrchr(inpname, '.');
	if (!p)
		strcat(inpname, ".asm");
	inpfile = fopen(inpname, "r");
	if (!inpfile) {
		fprintf(stderr, "can't open %s\n", inpname);
		exit(1);
	}
	outfile = fopen(outname, "wb");
	if (!outfile) {
		fprintf(stderr, "can't create %s\n", outname);
		exit(1);
	}
	if (f_list) {
		lstfile = fopen(lstname, "w");
		if (!lstfile) {
			fprintf(stderr, "can't create %s\n", outname);
			exit(1);
		}
		plst = lstbuff;
		*plst = '\0';
	}
	printf("assembling %s\npass 1\n", inpname);
	yyrestart(inpfile);
	yyparse();
	list("\n");
	if (f_syms)
		prtsym((f_list) ? lstfile : stdout);
	fclose(inpfile);
	if (!f_dump) {
		if (pcmax != fwrite(image, sizeof(word), pcmax, outfile)) {
			fprintf(stderr, "error writing output %s\n", outname);
			exit(1);
		}
	}
	fclose(outfile);
	if (f_list)
		fclose(lstfile);
	printf("%d lines, %d symbols\n", line_no, symb_no);

	return 0;
}
