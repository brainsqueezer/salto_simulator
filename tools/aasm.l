%{
/**********************************************************
 * Xerox Alto Assembler.
 *
 * Lexer of the Xerox Alto assembler.
 *
 * (C) 2007 by Juergen Buchmueller <pullmoll@t-online.de>
 *
 * $Id: aasm.l,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 **********************************************************/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#define	YYDEBUG	ASM_DEBUG
#include "aasm.tab.h"
#include "emu_ops.h"

extern char lstbuff[256];
extern char *plst;
extern int line_no;
extern int pass_no;
extern FILE *inpfile;
extern int argcnt;
extern int pc;
extern int pcmax;

static const char *valid_b = "01";
static const char *valid_o = "01234567";
static const char *valid_d = "0123456789";
static const char *valid_x = "0123456789ABCDEF";
int aton(char *src, int base, const char *valid, char end);

#define	TOKEN do { \
	if (!plst) \
		break; \
	plst += snprintf(plst, sizeof(lstbuff) - (size_t)(plst - lstbuff), "%s", yytext); \
} while (0)

%}

delim	[\t ,;()]
bin	[0-1]
oct	[0-7]
dec	[0-9]
alpha	[A-Za-z_]
hex	[0-9A-Fa-f]
alnum	[0-9A-Za-z_]
ax	[LlRrSsZzOoCc#]

%s	arith

%%

<arith>[Ll]			{ yylval.i = sh_L; 			return ARITH; 	}
<arith>[Rr]			{ yylval.i = sh_R; 			return ARITH;	}
<arith>[Ss]			{ yylval.i = sh_S; 			return ARITH; 	}
<arith>[Zz]			{ yylval.i = cy_Z; 			return ARITH; 	}
<arith>[Oo]			{ yylval.i = cy_O; 			return ARITH; 	}
<arith>[Cc]			{ yylval.i = cy_C; 			return ARITH; 	}
<arith>"#"			{ yylval.i = nl_;  			return ARITH; 	}
<arith>[Ll]"#"			{ yylval.i = sh_L | nl_;		return ARITH; 	}
<arith>[Rr]"#"			{ yylval.i = sh_R | nl_;		return ARITH;	}
<arith>[Ss]"#"			{ yylval.i = sh_S | nl_;		return ARITH; 	}
<arith>[Zz]"#"			{ yylval.i = cy_Z | nl_;		return ARITH; 	}
<arith>[Oo]"#"			{ yylval.i = cy_O | nl_;		return ARITH; 	}
<arith>[Cc]"#"			{ yylval.i = cy_C | nl_;		return ARITH; 	}
<arith>[Zz][Ll]			{ yylval.i = sh_L | cy_Z; 		return ARITH;	}
<arith>[Zz][Ll]"#"		{ yylval.i = sh_L | cy_Z | nl_; 	return ARITH;	}
<arith>[Ll][Zz]			{ yylval.i = sh_L | cy_Z; 		return ARITH;	}
<arith>[Ll][Zz]"#"		{ yylval.i = sh_L | cy_Z | nl_; 	return ARITH;	}
<arith>[Zz][Rr]			{ yylval.i = sh_R | cy_Z; 		return ARITH;	}
<arith>[Zz][Rr]"#"		{ yylval.i = sh_R | cy_Z | nl_; 	return ARITH;	}
<arith>[Rr][Zz]			{ yylval.i = sh_R | cy_Z; 		return ARITH;	}
<arith>[Rr][Zz]"#"		{ yylval.i = sh_R | cy_Z | nl_; 	return ARITH;	}
<arith>[Zz][Ss]			{ yylval.i = sh_S | cy_Z; 		return ARITH;	}
<arith>[Zz][Ss]"#"		{ yylval.i = sh_S | cy_Z | nl_; 	return ARITH;	}
<arith>[Ss][Zz]			{ yylval.i = sh_S | cy_Z; 		return ARITH;	}
<arith>[Ss][Zz]"#"		{ yylval.i = sh_S | cy_Z | nl_; 	return ARITH;	}
<arith>[Oo][Ll]			{ yylval.i = sh_L | cy_O; 		return ARITH;	}
<arith>[Oo][Ll]"#"		{ yylval.i = sh_L | cy_O | nl_; 	return ARITH;	}
<arith>[Ll][Oo]			{ yylval.i = sh_L | cy_O; 		return ARITH;	}
<arith>[Ll][Oo]"#"		{ yylval.i = sh_L | cy_O | nl_; 	return ARITH;	}
<arith>[Oo][Rr]			{ yylval.i = sh_R | cy_O; 		return ARITH;	}
<arith>[Oo][Rr]"#"		{ yylval.i = sh_R | cy_O | nl_; 	return ARITH;	}
<arith>[Rr][Oo]			{ yylval.i = sh_R | cy_O; 		return ARITH;	}
<arith>[Rr][Oo]"#"		{ yylval.i = sh_R | cy_O | nl_; 	return ARITH;	}
<arith>[Oo][Ss]			{ yylval.i = sh_S | cy_O; 		return ARITH;	}
<arith>[Oo][Ss]"#"		{ yylval.i = sh_S | cy_O | nl_; 	return ARITH;	}
<arith>[Ss][Oo]			{ yylval.i = sh_S | cy_O; 		return ARITH;	}
<arith>[Ss][Oo]"#"		{ yylval.i = sh_S | cy_O | nl_; 	return ARITH;	}
<arith>[Cc][Ll]			{ yylval.i = sh_L | cy_C; 		return ARITH;	}
<arith>[Cc][Ll]"#"		{ yylval.i = sh_L | cy_C | nl_; 	return ARITH;	}
<arith>[Ll][Cc]			{ yylval.i = sh_L | cy_C; 		return ARITH;	}
<arith>[Ll][Cc]"#"		{ yylval.i = sh_L | cy_C | nl_; 	return ARITH;	}
<arith>[Cc][Rr]			{ yylval.i = sh_R | cy_C; 		return ARITH;	}
<arith>[Cc][Rr]"#"		{ yylval.i = sh_R | cy_C | nl_; 	return ARITH;	}
<arith>[Rr][Cc]			{ yylval.i = sh_R | cy_C; 		return ARITH;	}
<arith>[Rr][Cc]"#"		{ yylval.i = sh_R | cy_C | nl_; 	return ARITH;	}
<arith>[Cc][Ss]			{ yylval.i = sh_S | cy_C; 		return ARITH;	}
<arith>[Cc][Ss]"#"		{ yylval.i = sh_S | cy_C | nl_; 	return ARITH;	}
<arith>[Ss][Cc]			{ yylval.i = sh_S | cy_C; 		return ARITH;	}
<arith>[Ss][Cc]"#"		{ yylval.i = sh_S | cy_C | nl_; 	return ARITH;	}
<arith>[\t ]?			{ yyless(0); BEGIN(INITIAL); }

[Aa][Cc]			{ TOKEN; return AC; }

[Ss][Kk][Pp]			{ TOKEN; return SKP; }
[Ss][Zz][Cc]			{ TOKEN; return SZC; }
[Ss][Nn][Cc]			{ TOKEN; return SNC; }
[Ss][Zz][Rr]			{ TOKEN; return SZR; }
[Ss][Nn][Rr]			{ TOKEN; return SNR; }
[Ss][Ee][Zz]			{ TOKEN; return SEZ; }
[Ss][Bb][Nn]			{ TOKEN; return SBN; }

[Jj][Mm][Pp]			{ TOKEN; return JMP; }
[Jj][Ss][Rr]			{ TOKEN; return JSR; }
[Dd][Ss][Zz]			{ TOKEN; return DSZ; }
[Ii][Ss][Zz]			{ TOKEN; return ISZ; }

[Ll][Dd][AA]			{ TOKEN; return LDA; }
[Ss][Tt][Aa]			{ TOKEN; return STA; }

[Cc][Oo][Mm]			{ TOKEN; return COM; }
[Nn][Ee][Gg]			{ TOKEN; return NEG; }
[Mm][Oo][Vv]			{ TOKEN; return MOV; }
[Ii][Nn][Cc]			{ TOKEN; return INC; }
[Aa][Dd][Cc]			{ TOKEN; return ADC; }
[Ss][Uu][Bb]			{ TOKEN; return SUB; }
[Aa][Dd][Dd]			{ TOKEN; return ADD; }
[Aa][Nn][Dd]			{ TOKEN; return AND; }

[Cc][Oo][Mm]{ax}*		{ TOKEN; BEGIN(arith); yyless(3); return COM; }
[Nn][Ee][Gg]{ax}*		{ TOKEN; BEGIN(arith); yyless(3); return NEG; }
[Mm][Oo][Vv]{ax}*		{ TOKEN; BEGIN(arith); yyless(3); return MOV; }
[Ii][Nn][Cc]{ax}*		{ TOKEN; BEGIN(arith); yyless(3); return INC; }
[Aa][Dd][Cc]{ax}*		{ TOKEN; BEGIN(arith); yyless(3); return ADC; }
[Ss][Uu][Bb]{ax}*		{ TOKEN; BEGIN(arith); yyless(3); return SUB; }
[Aa][Dd][Dd]{ax}*		{ TOKEN; BEGIN(arith); yyless(3); return ADD; }
[Aa][Nn][Dd]{ax}*		{ TOKEN; BEGIN(arith); yyless(3); return AND; }

[Cc][Yy][Cc][Ll][Ee]		{ TOKEN; return CYCLE; }
[Mm][Uu][Ll]			{ TOKEN; return MUL; }
[Dd][Ii][Vv]			{ TOKEN; return DIV; }
[Jj][Ss][Rr][Ii][Ii]		{ TOKEN; return JSRII; }
[Jj][Ss][Rr][Ii][Ss]		{ TOKEN; return JSRIS; }
[Cc][Oo][Nn][Vv][Ee][Rr][Tt]	{ TOKEN; return CONVERT; }
[Dd][Ii][Rr]			{ TOKEN; return DIR; }
[Ee][Ii][Rr]			{ TOKEN; return EIR; }
[Bb][Rr][Ii]			{ TOKEN; return BRI; }
[Rr][Cc][Ll][Kk]		{ TOKEN; return RCLK; }
[Ss][Ii][Oo]			{ TOKEN; return SIO; }
[Bb][Ll][Tt]			{ TOKEN; return BLT; }
[Bb][Ll][Kk][Ss]		{ TOKEN; return BLKS; }
[Ss][Ii][Tt]			{ TOKEN; return SIT; }
[Jj][Mm][Pp][Rr][Aa][Mm]	{ TOKEN; return JMPRAM; }
[Rr][Dd][Rr][Aa][Mm]		{ TOKEN; return RDRAM; }
[Ww][Rr][Tt][Rr][Aa][Mm]	{ TOKEN; return WRTRAM; }
[Dd][Ii][Rr][Ss]		{ TOKEN; return DIRS; }
[Vv][Ee][Rr][Ss]		{ TOKEN; return VERS; }
[Dd][Rr][Ee][Aa][Dd]		{ TOKEN; return DREAD; }
[Dd][Ww][Rr][Ii][Tt][Ee]	{ TOKEN; return DWRITE; }
[Dd][Ee][Xx][Cc][Hh]		{ TOKEN; return DEXCH; }
[Dd][Ii][Aa][Gg][Nn][Oo][Ss][Ee]1	{ TOKEN; return DIAGNOSE1; }
[Dd][Ii][Aa][Gg][Nn][Oo][Ss][Ee]2	{ TOKEN; return DIAGNOSE2; }
[Bb][Ii][Tt][Bb][Ll][Tt]	{ TOKEN; return BITBLT; }
[Xx][Mm][Ll][Dd][Aa]		{ TOKEN; return XMLDA; }
[Xx][Mm][Ss][Tt][Aa]		{ TOKEN; return XMSTA; }

[Dd][Bb]			{ TOKEN; return DB; }
[Dd][Ww]			{ TOKEN; return DW; }
[Dd][Dd]			{ TOKEN; return DD; }
[Dd][Ss]			{ TOKEN; return DS; }
[Ee][Nn][Dd]			{ TOKEN; return END; }
[Ee][Qq][Uu]			{ TOKEN; return EQU; }
[Aa][Ll][Ii][Gg][Nn]		{ TOKEN; return ALIGN; }
[Ee][Vv][Ee][Nn]		{ TOKEN; return EVEN; }
[Oo][Rr][Gg]			{ TOKEN; return ORG; }



"0"{oct}+			{
					TOKEN;
					yylval.i = aton(yytext,8,valid_o,0);
					return IMM;
				}

{bin}+[Bb]			{
					TOKEN;
					yylval.i = aton(yytext,2,valid_b,'B');
					return IMM;
				}

{oct}+[Oo]			{
					TOKEN;
					yylval.i = aton(yytext,8,valid_o,'O');
					return IMM;
				}

{dec}+[Dd]?			{
					TOKEN;
					yylval.i = aton(yytext,10,valid_d,'D');
					return IMM;
				}

{hex}+[Hh]			{
					TOKEN;
					yylval.i = aton(yytext,16,valid_x,'H');
					return IMM;
				}

"0"[Xx]{hex}+			{
					TOKEN;
					yylval.i = aton(yytext+2,16,valid_x,0);
					return IMM;
				}

"."				{ TOKEN; yylval.i = pc + argcnt; return IMM; }
","				{ TOKEN; return COMMA; }
":"                     	{ TOKEN; return COLON; }
"#"				{ TOKEN; return HASH; }
"("				{ TOKEN; return LPAREN; }
")"				{ TOKEN; return RPAREN; }
"'"				{ TOKEN; return QUOTE; }
"+"				{ TOKEN; return PLUS; }
"-"				{ TOKEN; return MINUS; }
"*"				{ TOKEN; return ASTERISK; }
"/"				{ TOKEN; return SLASH; }
"<<"				{ TOKEN; return SHL; }
">>"				{ TOKEN; return SHR; }
"&"                     	{ TOKEN; return AMPERSAND; }
"|"                     	{ TOKEN; return BAR; }
"^"                     	{ TOKEN; return CARET; }
"~"                     	{ TOKEN; return TILDE; }
"@"				{ TOKEN; return AT; }

\"[^\"\n]+\"            	{	/* anything enclosed in double quotes, w/o CR */
					TOKEN;
					yylval.s = strdup(yytext + 1);
					yylval.s[strlen(yylval.s)-1] = '\0';
					return CHR;
				}

\'[^\'\n]+\'			{	/* anything enclosed in single quotes, w/o CR */
					TOKEN;
					yylval.i = yytext[1];
					if (yyleng > 3)
						yylval.i += 256 * yytext[2];
					return IMM;
				}

{alpha}{alnum}*			{	/* alpha followed by alpha numerical */
					TOKEN;
					yylval.s = strdup(yytext);
					return SYM;
				}

[\t ]				{
					/* white space */
					TOKEN;
					BEGIN(INITIAL);
				}

";"[^\n]*\n			|	/* comment until CR */
[\n]				{	/* CR */
					TOKEN;
					BEGIN(INITIAL);
					if (plst)
						plst = lstbuff;
					line_no++;
					return EOL;
				}

%%

int aton(char *src, int base, const char *valid, char end)
{
	int result = 0;
	int sign = 1;
	char * pos;

	while (*src && !strchr(valid, toupper((unsigned char)*src)) &&
		(toupper((unsigned char)*src) != end)) {
		if (*src == '+')
			sign = +1;
		if (*src == '-')
			sign = -sign;
		src++;
	}
	while (*src && ((pos = strchr(valid, toupper((unsigned char)*src))) != NULL)) {
		result *= base;
		if (pos)
			result += (int)(pos - valid);
		src++;
	}
	result *= sign;
	return result;
}

void yyerror(char * msg, ...)
{
	va_list arg;
	fprintf(stderr, "line %d: ", line_no);
	va_start(arg, msg);
	fprintf(stderr, msg, arg);
	va_end(arg);
	fprintf(stderr, " at \"%s\"\n", yytext);
}

int yywrap(void)
{
	if (pass_no < 2) {
		rewind(inpfile);
		pc = 0;
		pcmax = -1;
		line_no = 1;
		pass_no++;
		printf("pass %d\n", pass_no);
		return 0;
	}
	return 1;
}

