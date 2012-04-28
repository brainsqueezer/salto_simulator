/**********************************************************************
 * Convert BDF files to Xerox Alto AL (assembler source)
 *
 * Copyright (c) 2007 Juergen Buchmueller (pullmoll@t-online.de)
 *
 *
 * Conversion currently only works for fonts of width <= 16
 *
 * $Id: convbdf.c,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 **********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

typedef unsigned long IMGBITS;	/* bitmap image unit size */

/* IMGBITS helper macros */
#define	IMG_BITSPERIMAGE	(sizeof(IMGBITS)*8)
#define IMG_NIBSPERIMAGE	(IMG_BITSPERIMAGE/4)
#define IMG_WORDS(x)		(((x)+IMG_BITSPERIMAGE-1)/IMG_BITSPERIMAGE)	/* image size in words */
#define IMG_BYTES(x)		(IMG_WORDS(x)*sizeof(IMGBITS))
#define	IMG_BITVALUE(n)		((IMGBITS)(((IMGBITS)1)<<(n)))
#define	IMG_FIRSTBIT		(IMG_BITVALUE(IMG_BITSPERIMAGE-1))
#define	IMG_TESTBIT(m)		((m) & IMG_FIRSTBIT)
#define	IMG_SHIFTBIT(m)		((IMGBITS)(m) << 1)
#define	IMG_PITCH(w)		((w)<=8 ? 1 : (w)<=16 ? 2 : 4)

#define	UNUSED	((unsigned)-1)

/* builtin C-based proportional/fixed font structure  */
/* based on The Microwindows Project http://microwindows.org  */
typedef struct {
	char *name;		/* font name */
	int maxwidth;		/* max width in pixels */
	int height;		/* height in pixels */
	int ascent;		/* ascent (baseline) height */
	int firstchar;		/* first character in bitmap */
	int size;		/* font size in glyphs */
	IMGBITS *bits;		/* right-padded bitmap data */
	unsigned long *offset;	/* offsets into bitmap data */
	unsigned char *width;	/* character widths or NULL if fixed */
	unsigned long *pages;	/* used unicode pages offsets */
	char **glyphname;	/* glyph names after STARTCHAR  */
	int defaultchar;	/* default char (not glyph index) */
	long bits_size;		/* # words of IMGBITS bits */

	/* unused by runtime system, read in by convbdf */
	char *facename;		/* facename of font */
	char *copyright;	/* copyright info for loadable fonts */
	int pixel_size;
	int descent;
	int fbbw, fbbh, fbbx, fbby;
} MWCFONT, *PMWCFONT;

#define isprefix(buf,str)	(!strncmp(buf, str, strlen(str)))
#define	strequal(s1,s2)		(!strcmp(s1, s2))

#define EXTRA	300		/* # bytes extra allocation for buggy .bdf files */

int gen_map = 1;
int start_char = 0;		/* by default start with char 0 */
int limit_char = 255;		/* by default limit to 255 characters */
int oformat = 0;
int oflag = 0;
char outfile[256];

void usage(void);
void getopts(int *pac, char ***pav);
int convbdf(char *path);

void free_font(PMWCFONT pf);
PMWCFONT bdf_read_font(char *path);
int bdf_read_header(FILE *fp, PMWCFONT pf);
int bdf_read_bitmaps(FILE *fp, PMWCFONT pf);
char *bdf_getline(FILE *fp, char *buf, int len);
IMGBITS bdf_hexval(unsigned char *buf, int ndx1, int ndx2);

int gen_al_binary(PMWCFONT pf, char *path);
int gen_asm_source(PMWCFONT pf, char *path);

void usage(void)
{
	char help[] = {
	"Usage: convbdf [options] [input-files]\n"
	"       convbdf [options] [-o output-file] [single-input-file]\n"
	"Options:\n"
	"    -b     output bitmap font binary format\n"
	"    -s N   Start output at character encodings >= N\n"
	"    -l N   Limit output to character encodings <= N\n"
	"    -n     Don't generate bitmaps as comments in .asm file\n"
	};

	fprintf(stderr, help);
}

/* parse command line options */
void getopts(int *pac, char ***pav)
{
	char *p;
	char **av;
	int ac;

	ac = *pac;
	av = *pav;
	while (ac > 0 && av[0][0] == '-') {
		p = &av[0][1]; 
		while( *p)
			switch(*p++) {
		case ' ':			/* multiple -args on av[] */
			while( *p && *p == ' ')
				p++;
			if( *p++ != '-')	/* next option must have dash */
				p = "";
			break;			/* proceed to next option */
		case 'b':			/* select binary output format */
			oformat = 1;
			break;
		case 'n':			/* don't gen bitmap comments */
			gen_map = 0;
			break;
		case 'o':			/* set output file */
			oflag = 1;
			if (*p) {
				strcpy(outfile, p);
				while (*p && *p != ' ')
					p++;
			} else {
				av++; ac--;
				if (ac > 0)
					strcpy(outfile, av[0]);
			}
			break;
		case 'l':			/* set encoding limit */
			if (*p) {
				limit_char = atoi(p);
				while (*p && *p != ' ')
					p++;
			} else {
				av++; ac--;
				if (ac > 0)
					limit_char = atoi(av[0]);
			}
			break;
		case 's':			/* set encoding start */
			if (*p) {
				start_char = atoi(p);
				while (*p && *p != ' ')
					p++;
			} else {
				av++; ac--;
				if (ac > 0)
					start_char = atoi(av[0]);
			}
			break;
		default:
			fprintf(stderr, "Unknown option ignored: %c\r\n", *(p-1));
		}
		++av; --ac;
	}
	*pac = ac;
	*pav = av;
}

/* remove directory prefix and file suffix from full path */
char *base_name(char *path)
{
	char *p, *b;
	static char base[256];

	/* remove prepended path and extension */
	b = path;
	for (p=path; *p; ++p) {
		if (*p == '/')
			b = p + 1;
	}
	strcpy(base, b);
	for (p=base; *p; ++p) {
		if (*p == '.') {
			*p = 0;
			break;
		}
	}
	return base;
}

int convbdf(char *path)
{
	PMWCFONT pf;
	int ret = 0;

	pf = bdf_read_font(path);
	if (!pf)
		exit(1);

	if (!oflag) {
		strcpy(outfile, base_name(path));
		if (oformat)
			strcat(outfile, ".al");
		else
			strcat(outfile, ".asm");
	}
	if (oformat)
		ret = gen_al_binary(pf, outfile);
	else
		ret = gen_asm_source(pf, outfile);

	free_font(pf);
	return ret;
}

int main(int ac, char **av)
{
	int ret = 0;

	++av; --ac;		/* skip av[0] */
	getopts(&ac, &av);	/* read command line options */

	if (ac < 1) {
		usage();
		exit(1);
	}
	if (oflag) {
		if (ac > 1) {
			usage();
			exit(1);
		}
	}

	while (ac > 0) {
		ret |= convbdf(av[0]);
		++av; --ac;
	}

	exit(ret);
}

/* free font structure */
void free_font(PMWCFONT pf)
{
	if (!pf)
		return;
	if (pf->name)
		free(pf->name);
	if (pf->facename)
		free(pf->facename);
	if (pf->bits)
		free(pf->bits);
	if (pf->offset)
		free(pf->offset);
	if (pf->width)
		free(pf->width);
	if (pf->pages)
		free(pf->pages);
	free(pf);
}

/* build incore structure from .bdf file */
PMWCFONT bdf_read_font(char *path)
{
	FILE *fp;
	PMWCFONT pf;

	fp = fopen(path, "rb");
	if (!fp) {
		fprintf(stderr, "Error opening file: %s\n", path);
		return NULL;
	}

	pf = (PMWCFONT)calloc(1, sizeof(MWCFONT));
	if (!pf)
		goto errout;
	
	pf->name = strdup(base_name(path));

	if (!bdf_read_header(fp, pf)) {
		fprintf(stderr, "Error reading font header\n");
		goto errout;
	}

	if (!bdf_read_bitmaps(fp, pf)) {
		fprintf(stderr, "Error reading font bitmaps\n");
		goto errout;
	}

	fclose(fp);
	return pf;

errout:
	fclose(fp);
	free_font(pf);
	return NULL;
}

/* read bdf font header information, return 0 on error */
int bdf_read_header(FILE *fp, PMWCFONT pf)
{
	int encoding;
	int nchars, maxwidth;
	int firstchar = 65535;
	int lastchar = -1;
	char buf[256];
	char facename[256];
	char copyright[256];

	/* set certain values to errors for later error checking */
	pf->defaultchar = -1;
	pf->ascent = -1;
	pf->descent = -1;

	for (;;) {
		if (!bdf_getline(fp, buf, sizeof(buf))) {
			fprintf(stderr, "Error: EOF on file\n");
			return 0;
		}
		if (isprefix(buf, "FONT ")) {		/* not required */
			if (sscanf(buf, "FONT %[^\n]", facename) != 1) {
				fprintf(stderr, "Error: bad 'FONT'\n");
				return 0;
			}
			pf->facename = strdup(facename);
			continue;
		}
		if (isprefix(buf, "COPYRIGHT ")) {	/* not required */
			if (sscanf(buf, "COPYRIGHT \"%[^\"]", copyright) != 1) {
				fprintf(stderr, "Error: bad 'COPYRIGHT'\n");
				return 0;
			}
			pf->copyright = strdup(copyright);
			continue;
		}
		if (isprefix(buf, "DEFAULT_CHAR ")) {	/* not required */
			if (sscanf(buf, "DEFAULT_CHAR %d", &pf->defaultchar) != 1) {
				fprintf(stderr, "Error: bad 'DEFAULT_CHAR'\n");
				return 0;
			}
		}
		if (isprefix(buf, "FONT_DESCENT ")) {
			if (sscanf(buf, "FONT_DESCENT %d", &pf->descent) != 1) {
				fprintf(stderr, "Error: bad 'FONT_DESCENT'\n");
				return 0;
			}
			continue;
		}
		if (isprefix(buf, "FONT_ASCENT ")) {
			if (sscanf(buf, "FONT_ASCENT %d", &pf->ascent) != 1) {
				fprintf(stderr, "Error: bad 'FONT_ASCENT'\n");
				return 0;
			}
			continue;
		}
		if (isprefix(buf, "FONTBOUNDINGBOX ")) {
			if (sscanf(buf, "FONTBOUNDINGBOX %d %d %d %d",
			    &pf->fbbw, &pf->fbbh, &pf->fbbx, &pf->fbby) != 4) {
				fprintf(stderr, "Error: bad 'FONTBOUNDINGBOX'\n");
				return 0;
			}
			continue;
		}
		if (isprefix(buf, "CHARS ")) {
			if (sscanf(buf, "CHARS %d", &nchars) != 1) {
				fprintf(stderr, "Error: bad 'CHARS'\n");
				return 0;
			}
			continue;
		}

		/*
		 * Reading ENCODING is necessary to get firstchar/lastchar
		 * which is needed to pre-calculate our offset and widths
		 * array sizes.
		  */
		if (isprefix(buf, "ENCODING ")) {
			if (sscanf(buf, "ENCODING %d", &encoding) != 1) {
				fprintf(stderr, "Error: bad 'ENCODING'\n");
				return 0;
			}
			if (encoding >= 0 && encoding <= limit_char && encoding >= start_char) {
				if (firstchar > encoding)
					firstchar = encoding;
				if (lastchar < encoding)
					lastchar = encoding;
			}
			continue;
		}
		if (strequal(buf, "ENDFONT"))
			break;
	}

	/* calc font height */
	if (pf->ascent < 0 || pf->descent < 0 || firstchar < 0) {
		fprintf(stderr, "Error: Invalid BDF file, requires FONT_ASCENT/FONT_DESCENT/ENCODING\n");
		return 0;
	}
	pf->height = pf->ascent + pf->descent;

	/* calc default char */
	if (pf->defaultchar < 0 || pf->defaultchar < firstchar ||
	    pf->defaultchar > limit_char)
		pf->defaultchar = firstchar;

	/* calc font size (offset/width entries) */
	pf->firstchar = firstchar;
	pf->size = lastchar - firstchar + 1;
	
	/* use the font boundingbox to get initial maxwidth */
	/*maxwidth = pf->fbbw - pf->fbbx; */
	maxwidth = pf->fbbw;

	/* initially use font maxwidth * height for bits allocation */
	pf->bits_size = nchars * IMG_WORDS(maxwidth) * pf->height;

	/* allocate bits, offset, and width arrays */
	pf->bits = (IMGBITS *)malloc(pf->bits_size * sizeof(IMGBITS) + EXTRA);
	pf->offset = (unsigned long *)malloc(pf->size * sizeof(unsigned long));
	pf->width = (unsigned char *)malloc(pf->size * sizeof(unsigned char));
	pf->pages = (unsigned long *)malloc(256 * sizeof(unsigned long));
	pf->glyphname = (char **)calloc(pf->size, sizeof(char *));
	
	if (!pf->bits || !pf->offset || !pf->width) {
		fprintf(stderr, "Error: no memory for font load\n");
		return 0;
	}

	return 1;
}

/* read bdf font bitmaps, return 0 on error */
int bdf_read_bitmaps(FILE *fp, PMWCFONT pf)
{
	long ofs = 0;
	int maxwidth = 0;
	int i, k, encoding, width;
	int bbw, bbh, bbx, bby;
	int proportional = 0;
	char glyphname[256];
	char buf[256];

	/* reset file pointer */
	fseek(fp, 0L, SEEK_SET);

	/* initially mark offsets as not used */
	for (i = 0; i < pf->size; i++)
		pf->offset[i] = UNUSED;
	/* initially mark pages as not used */
	for (i = 0; i < 256; i++)
		pf->pages[i] = UNUSED;

	for (;;) {
		if (!bdf_getline(fp, buf, sizeof(buf))) {
			fprintf(stderr, "Error: EOF on file\n");
			return 0;
		}
		if (isprefix(buf, "STARTCHAR")) {
			encoding = width = bbw = bbh = bbx = bby = -1;
			if (strlen(buf) > 9)
				snprintf(glyphname, sizeof(glyphname), "%s", buf + 9);
			else
				glyphname[0] = '\0';
			continue;
		}
		if (isprefix(buf, "ENCODING ")) {
			if (sscanf(buf, "ENCODING %d", &encoding) != 1) {
				fprintf(stderr, "Error: bad 'ENCODING'\n");
				return 0;
			}
			if (encoding < start_char || encoding > limit_char)
				encoding = -1;
			continue;
		}
		if (isprefix(buf, "DWIDTH ")) {
			if (sscanf(buf, "DWIDTH %d", &width) != 1) {
				fprintf(stderr, "Error: bad 'DWIDTH'\n");
				return 0;
			}
			/* use font boundingbox width if DWIDTH <= 0 */
			if (width <= 0)
				width = pf->fbbw - pf->fbbx;
			continue;
		}
		if (isprefix(buf, "BBX ")) {
			if (sscanf(buf, "BBX %d %d %d %d", &bbw, &bbh, &bbx, &bby) != 4) {
				fprintf(stderr, "Error: bad 'BBX'\n");
				return 0;
			}
			continue;
		}
		if (strequal(buf, "BITMAP")) {
			IMGBITS *ch_bitmap = pf->bits + ofs;
			int ch_words;
			int gidx;

			if (encoding < pf->firstchar)
				continue;
			gidx = encoding - pf->firstchar;

			/* set bits offset in encode map */
			if (pf->offset[gidx] != UNUSED) {
				fprintf(stderr, "Error: duplicate encoding for" \
					" character %d (0x%02x), ignoring duplicate\n",
					encoding, encoding);
				continue;
			}
			pf->offset[gidx] = ofs;
			if (UNUSED == pf->pages[encoding / 256])
				pf->pages[encoding / 256] = ofs;	/* mark page as used */

			/* calc char width */
			if (bbx < 0) {
				width -= bbx;
				/* if (width > maxwidth)
					width = maxwidth; */
				bbx = 0;
			}
			if (width > maxwidth)
				maxwidth = width;
			pf->width[gidx] = width;

			/* clear bitmap */
			memset(ch_bitmap, 0, IMG_BYTES(width) * pf->height);

			ch_words = IMG_WORDS(width);
#define BM(row,col)	*(((IMGBITS*)ch_bitmap + (row) * ch_words) + (col))

			/* read bitmaps */
			for (i = 0; /* */; i++) {
				int hexnibbles;
				int row;

				if (!bdf_getline(fp, buf, sizeof(buf))) {
					fprintf(stderr, "Error: EOF reading BITMAP data\n");
					return 0;
				}
				if (isprefix(buf, "ENDCHAR"))
					break;

				row = pf->height - pf->descent - bby - bbh + i;
				if (row < 0 || row > pf->height) {
#if	0
					/* silently ignore rows above/below font height */
					fprintf(stderr, "Error: bitmap row (= %d) out of bounds (%s:%d)\n",
						row, __FILE__, __LINE__);
					fprintf(stderr, "	encoding:	%d\n", encoding);
					fprintf(stderr, "	pf->height:	%d\n", pf->height);
					fprintf(stderr, "	pf->descent:	%d\n", pf->descent);
					fprintf(stderr, "	bby:		%d\n", bby);
					fprintf(stderr, "	bbh:		%d\n", bbh);
					fprintf(stderr, "	i:		%d\n", i);
#endif
					continue;
				}
				hexnibbles = strlen(buf);
				for (k = 0; k < ch_words; ++k) {
					int ndx = k * IMG_NIBSPERIMAGE;
					int padnibbles = IMG_NIBSPERIMAGE - (hexnibbles % IMG_NIBSPERIMAGE);
					IMGBITS value;

					if (padnibbles <= 0)
						break;
					if (padnibbles >= IMG_NIBSPERIMAGE)
						padnibbles = 0;

					value = bdf_hexval((unsigned char *)buf,
						ndx, ndx + IMG_NIBSPERIMAGE - 1 - padnibbles);
#if	0
					printf("%d-%d val:0x%08x pad:%d\n",
						ndx, ndx + IMG_NIBSPERIMAGE - 1 - padnibbles,
						value, padnibbles);
#endif
					value <<= padnibbles * 4;

					BM(row, k) |= value >> bbx;
					/* handle overflow into next image word */
					if (bbx) {
						BM(row, k+1) = value << (IMG_BITSPERIMAGE - bbx);
					}
				}
			}
			if ('\0' != glyphname[0])
				pf->glyphname[gidx] = strdup(glyphname);
			ofs += IMG_WORDS(width) * pf->height;
			continue;
		}
		if (strequal(buf, "ENDFONT"))
			break;
	}

	/* set max width */
	pf->maxwidth = maxwidth;

	/* change unused offset/width values to default char values */
	for (i = 0; i< pf->size; ++i) {
		int defchar = pf->defaultchar - pf->firstchar;

		if (pf->offset[i] == UNUSED) {
			pf->offset[i] = pf->offset[defchar];
			pf->width[i] = pf->width[defchar];
		}
	}

#if	0
	{
		int encodetable = 0;
		unsigned long l;
		/* determine whether font doesn't require encode table */
		l = 0;
		for (i = 0; i < pf->size; ++i) {
			if (pf->offset[i] != l) {
				encodetable = 1;
				break;
			}
			l += IMG_WORDS(pf->width[i]) * pf->height;
		}
		if (!encodetable) {
			free(pf->offset);
			pf->offset = NULL;
		}
	}
#endif

	/* determine whether font is fixed-width */
	for (i = 0; i < pf->size; ++i) {
		if (pf->width[i] != maxwidth) {
			proportional = 1;
			break;
		}
	}
	if (!proportional) {
		free(pf->width);
		pf->width = NULL;
	}

	/* reallocate bits array to actual bits used */
	if (ofs < pf->bits_size) {
		pf->bits = realloc(pf->bits, ofs * sizeof(IMGBITS));
		pf->bits_size = ofs;
	} else if (ofs > pf->bits_size) {
		fprintf(stderr, "Warning: DWIDTH spec > max FONTBOUNDINGBOX\n");
		if (ofs > pf->bits_size+EXTRA) {
			fprintf(stderr, "Error: Not enough bits initially allocated\n");
			return 0;
		}
		pf->bits_size = ofs;
	}

	return 1;
}

/* read the next non-comment line, returns buf or NULL if EOF */
char *bdf_getline(FILE *fp, char *buf, int len)
{
	int c;
	char *b;

	for (;;) {
		b = buf;
		while ((c = getc(fp)) != EOF) {
			if (c == '\r')
				continue;
			if (c == '\n')
				break;
			if (b - buf >= (len - 1))
				break;
			*b++ = c;
		}
		*b = '\0';
		if (c == EOF && b == buf)
			return NULL;
		if (b != buf && !isprefix(buf, "COMMENT"))
			break;
	}
	return buf;
}

/* return hex value of portion of buffer */
IMGBITS bdf_hexval(unsigned char *buf, int ndx1, int ndx2)
{
	IMGBITS val = 0;
	int i, c;

	for (i = ndx1; i <= ndx2; ++i) {
		c = buf[i];
		if (c == 0)
			break;
		if (c >= '0' && c <= '9')
			c -= '0';
		else if (c >= 'A' && c <= 'F')
			c = c - 'A' + 10;
		else if (c >= 'a' && c <= 'f')
			c = c - 'a' + 10;
		else c = 0;
		val = (val << 4) | c;
	}
	for (/* */; i <= ndx2; ++i)
		val <<= 4;
	return val;
}


static int write_string(FILE *f, char *src, int size)
{
	char buff[256];

	if (size > sizeof(buff)) {
		fprintf(stderr, "buffer overrun %s.%d\n",
			__FILE__, __LINE__);
		exit(1);
	}
	memset(buff, 0, sizeof(buff));
	strncpy(buff, src, size);
	return fwrite(buff, 1, size, f);
}

static int write_byte(FILE *f, int value)
{
	unsigned char buff[1];

	buff[0] = (unsigned char)value;
	return fwrite(buff, 1, 1, f);
}

static int write_word(FILE *f, int value)
{
	unsigned char buff[2];

	buff[0] = (unsigned char)value;
	buff[1] = (unsigned char)(value >> 8);
	return fwrite(buff, 1, 2, f);
}

static int write_dword(FILE *f, int value)
{
	unsigned char buff[4];

	buff[0] = (unsigned char)value;
	buff[1] = (unsigned char)(value >> 8);
	buff[2] = (unsigned char)(value >> 16);
	buff[3] = (unsigned char)(value >> 24);
	return fwrite(buff, 1, 4, f);
}


/* generate Alto AL font binary from in-core font (not yet) */
int gen_al_binary(PMWCFONT pf, char *path)
{
	FILE *ofp;
	int i, j;
	int did_defaultchar = 0;
	int did_syncmsg = 0;
	size_t hdr_size;
	off_t bits_start = 0;
	off_t bits_end = 0;
	off_t page_start[256];
	off_t offs_start = 0;
	off_t offs_end = 0;
	off_t widths_start = 0;
	off_t widths_end = 0;
	IMGBITS *ofs = pf->bits;

	ofp = fopen(path, "wb");
	if (!ofp) {
		fprintf(stderr, "Can't create %s\n", path);
		return 1;
	}
	fprintf(stdout, "Generating %s\n", path);

	/* size of the PMOS bitmap font header data
	 * 32 bytes font name + 11 dwords info
	 */
	hdr_size = 32 + 11*4;

	/* seek past header */
	fseek(ofp, hdr_size, SEEK_SET);

	/* generate the font bitmaps */
	bits_start = ftell(ofp);
	for (i = 0; i < pf->size; ++i) {
		int x;
 		int width = pf->width ? pf->width[i] : pf->maxwidth;
		int height = pf->height;
		int size;
		IMGBITS *bits = pf->bits + (pf->offset? pf->offset[i] : (unsigned long)(height * i));

		/*
		 * Generate bitmap bits only if not this index isn't
		 * the default character in encode map, or the default
		 * character hasn't been generated yet.
		  */
		if (pf->offset && (pf->offset[i] == pf->offset[pf->defaultchar-pf->firstchar])) {
			if (did_defaultchar)
				continue;
			did_defaultchar = 1;
		}

		bits = pf->bits + (pf->offset ? pf->offset[i] : (unsigned long)(pf->height * i));
		size = IMG_WORDS(width) * pf->height;
		for (x = 0; x < size; x++) {
			if (pf->maxwidth < 9) {
				write_byte(ofp, *bits >> (IMG_BITSPERIMAGE - pf->maxwidth));
			} else if (pf->maxwidth < 17) {
				write_word(ofp, *bits >> (IMG_BITSPERIMAGE - pf->maxwidth));
			} else {
				write_dword(ofp, *bits >> (IMG_BITSPERIMAGE - pf->maxwidth));
			}
			if (!did_syncmsg && *bits++ != *ofs++) {
				fprintf(stderr,
					"Warning: found encoding values in non-sorted order (not an error).\n");
				did_syncmsg = 1;
			}
		}	
	}
	bits_end = ftell(ofp);

	if (0 != (bits_end & 15)) {
		/* align to 16 bytes boundary */
		write_string(ofp, "", 16 - (bits_end & 15));
		bits_end = ftell(ofp);
	}

	/* output offset table for non-complete fonts */
	if (pf->offset) {
		offs_start = ftell(ofp);
		for (i = 0; i < 256; i++) {
			unsigned long pageofs;
			if (UNUSED == pf->pages[i])
				continue;
			pageofs = pf->pages[i];
			page_start[i] = ftell(ofp);	/* remember file offset in page_start[i] */

			for (j = i*256; j < i*256 + 256; j++) {
				long int ofs;
				ofs = (pf->offset[j - pf->firstchar] - pageofs) * IMG_PITCH(pf->maxwidth);
				if (j < pf->firstchar || (j - pf->firstchar) > pf->size) {
					/* unavailable glyph */
					write_word(ofp, 0xffff);
				} else if (ofs < 0 || ofs > 65535) {
					/* (probably) the default glyph */
					write_word(ofp, 0xffff);
				} else {
					/* glyph inside page */
					write_word(ofp, ofs);
				}
			}
		}
		offs_start = ftell(ofp);
		/* output page offset table */
		for (i = 0; i < 256; i++) {
			if (UNUSED == pf->pages[i]) {
				/* unused page: 2 dwords with 0 */
				write_dword(ofp, 0);
				write_dword(ofp, 0);
			} else {
				/* used page */
				write_dword(ofp, page_start[i]);	/* start of the page table */
				write_dword(ofp, pf->pages[i] * IMG_PITCH(pf->maxwidth));
			}
		}
		offs_end = ftell(ofp);
	}

	/* output width table for proportional fonts */
	if (pf->width) {
		widths_start = ftell(ofp);
		for (i = 0; i < pf->size; ++i) {
			write_byte(ofp, pf->width[i]);
		}
		widths_end = ftell(ofp);
	}

	/* seek to the header start at abs. 0 */
	fseek(ofp, 0, SEEK_SET);

	/* output bitmap font struct */
	write_string(ofp, pf->name, 32);		/* bm_font.name[32] */
	write_dword(ofp, IMG_PITCH(pf->maxwidth));	/* bm_font.pitch    */
	write_dword(ofp, pf->maxwidth);			/* bm_font.maxw     */
	write_dword(ofp, pf->height);			/* bm_font.height   */
	write_dword(ofp, pf->ascent);			/* bm_font.ascent   */
	write_dword(ofp, pf->firstchar);		/* bm_font.first    */
	write_dword(ofp, pf->firstchar + pf->size);	/* bm_font.size     */
	write_dword(ofp, bits_start);			/* bm_font.bits     */
	write_dword(ofp, offs_start);			/* bm_font.offs     */
	write_dword(ofp, widths_start);			/* bm_font.widths   */
	write_dword(ofp, pf->defaultchar);		/* bm_font.defchar  */
	write_dword(ofp, bits_end - bits_start);	/* bm_font.length   */

	return 0;
}

/* generate ASM source from in-core font */
int gen_asm_source(PMWCFONT pf, char *path)
{
	FILE *ofp;
	int i;
	int did_defaultchar = 0;
	int did_syncmsg = 0;
	time_t t = time(0);
	IMGBITS *ofs = pf->bits;
	char buf[256];

	ofp = fopen(path, "w");
	if (!ofp) {
		fprintf(stderr, "Can't create %s\n", path);
		return 1;
	}
	fprintf(stdout, "Generating %s\n", path);

	strcpy(buf, ctime(&t));
	buf[strlen(buf)-1] = 0;

	fprintf(ofp,
		"; Generated by convbdf on %s.\n" \
		"\n" \
		"; =====================================================================\n" \
		"; Font information\n" \
		";   name        : %s\n" \
		";   facename    : %s\n" \
		";   w x h       : %dx%d\n" \
		";   size        : %d\n" \
		";   ascent      : %d\n" \
		";   descent     : %d\n" \
		";   first char  : %d (0x%02x)\n" \
		";   last char   : %d (0x%02x)\n" \
		";   default char: %d (0x%02x)\n" \
		";   proportional: %s\n" \
		";   %s\n" \
		"; =====================================================================\n" \
		"\n",
		buf, 
		pf->name,
		pf->facename ? pf->facename : "-/-",
		pf->maxwidth, pf->height,
		pf->size,
		pf->ascent, pf->descent,
		pf->firstchar, pf->firstchar,
		pf->firstchar+pf->size-1, pf->firstchar+pf->size-1,
		pf->defaultchar, pf->defaultchar,
		pf->width? "yes": "no",
		pf->copyright? pf->copyright: "");


	/* output Alto bitmap font struct */
	
	fprintf(ofp,
		"; font height (header info)\n");
	fprintf(ofp,
		"fontheight:	dw	%d\n",
		pf->height);
	fprintf(ofp,
		"; font type, baseline, maxwidth (header info)\n");
	fprintf(ofp,
		"fonttype:	dw	(%d << 15) | (%d << 8) | %d\n",
		pf->width ? 1 : 0, pf->ascent, pf->maxwidth);
	fprintf(ofp,
		"fontbase:	; self relative offsets to character XWs\n");

	for (i = 0; i < 256; i++) {
		if (0 == (i%4))
			fprintf(ofp, "\t\tdw\t");
		if (pf->offset && (pf->offset[i] == pf->offset[pf->defaultchar - pf->firstchar])) {
			if (did_defaultchar) {
				fprintf(ofp, "XW%03o-.%s", pf->defaultchar, 3 == (i%4) ? "\n" : ",");
				continue;
			}
			did_defaultchar = 1;
		}
		fprintf(ofp,
			"XW%03o-.%s", i, 3 == (i%4) ? "\n" : ",");
	}

	did_defaultchar = 0;		
	fprintf(ofp,
		"\n" \
		"; Font bitmap data\n");

	/* generate bitmaps */
	for (i = 0; i < pf->size; ++i) {
		int x;
		int bitcount = 0;
 		int width = pf->width ? pf->width[i] : pf->maxwidth;
		int height = pf->height;
		int size;
		int skiptop = 0;
		int skipbottom = 0;
		IMGBITS *bits = pf->bits + (pf->offset? pf->offset[i] : (unsigned long)(height * i));
		IMGBITS bitvalue = 0;

		/*
		 * Generate bitmap bits only if this index isn't
		 * the default character in encode map, or the default
		 * character hasn't been generated yet.
		  */
		if (pf->offset && (pf->offset[i] == pf->offset[pf->defaultchar - pf->firstchar])) {
			if (did_defaultchar)
				continue;
			did_defaultchar = 1;
		}

		fprintf(ofp, "\n; Character %d (0x%02x):  width %d",
			i+pf->firstchar, i+pf->firstchar, width);
		if (pf->glyphname[i + pf->firstchar])
			fprintf(ofp, " %s", pf->glyphname[i+pf->firstchar]);

		if (gen_map) {
			fprintf(ofp, "\n;	+");
			for (x = 0; x < width; ++x)
				fprintf(ofp, "-");
			fprintf(ofp, "+\n");

			x = 0;
			while (height > 0) {
				if (x == 0) fprintf(ofp, ";	|");

				if (bitcount <= 0) {
					bitcount = IMG_BITSPERIMAGE;
					bitvalue = *bits++;
				}

				fprintf(ofp, IMG_TESTBIT(bitvalue) ? "*": " ");

				bitvalue = IMG_SHIFTBIT(bitvalue);
				--bitcount;
				if (++x == width) {
					fprintf(ofp, "|\n");
					--height;
					x = 0;
					bitcount = 0;
				}
			}
			fprintf(ofp, ";	+");
			for (x = 0; x < width; ++x)
				fprintf(ofp, "-");
			fprintf(ofp, "+ \n");
		} else {
			fprintf(ofp, " \n");
		}

		bits = pf->bits + (pf->offset ? pf->offset[i] : (unsigned long)(pf->height * i));
		size = IMG_WORDS(width) * pf->height;
		for (x = 0; x < size; x++) {
			if (0 == bits[x])
				skiptop++;
			else
				break;
		}
		for (x = size - 1; x >= skiptop; --x) {
			if (0 == bits[x])
				skipbottom++;
			else
				break;
		}
		size = size - IMG_WORDS(width)*skiptop - IMG_WORDS(width)*skipbottom;
		for (x = 0; x < size; x++) {
			fprintf(ofp, "\t\tdw\t%07lo\n",
				bits[x + skiptop] >> (IMG_BITSPERIMAGE-16));
			if (!did_syncmsg && bits[x + skiptop] != ofs[x + skiptop]) {
				fprintf(stderr, "Warning: found encoding values in non-sorted order (not an error).\n");
				did_syncmsg = 1;
			}
		}
		ofs += IMG_WORDS(width) * pf->height;
		fprintf(ofp,
			"XW%03o:		dw	2 * %d + 1		; no extent\n",
				i, pf->maxwidth);
		fprintf(ofp,
			"		dw	(%d << 8) | %d		; HD: %d, XH: %d\n",
				skiptop, pf->height - skiptop - skipbottom,
				skiptop, pf->height - skiptop - skipbottom);
	}

	return 0;
}
