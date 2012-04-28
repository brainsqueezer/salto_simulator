/*****************************************************************************
 * Portable Pix Map to C source converter (for P6 images)
 *
 * Copyright (C) 2007 by Juergen Buchmueller <pullmoll@t-online.de>
 * Partially based on info found in Eric Smith's Alto simulator: Altogether
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: ppm2c.c,v 1.1.1.1 2008/07/22 19:02:07 pm Exp $
 *****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

typedef struct {
	char *name;
	int mode;
	int width;
	int height;
	int mask;
	unsigned char *data;
}	ppm_t;

int ppm_to_c_source(ppm_t *p)
{
	int x, y, w, offs;

	printf("const uint8_t ppm_%s[%d*%d][%d] = {\n",
		p->name, p->width, p->height, p->mode);
	offs = 0;
	for (y = 0; y < p->height; y++) {
		for (x = 0, w = 0; x < p->width; x++) {
			switch (p->mode) {
			case 1:
				w += printf("{0x%02x}",
					p->data[offs+0]);
				offs++;
				break;
			case 2:
				w += printf("{0x%02x,0x%02x}",
					p->data[offs+0],p->data[offs+1]);
				offs += 2;
				break;
			case 3:
				w += printf("{0x%02x,0x%02x,0x%02x}",
					p->data[offs+0],p->data[offs+1],p->data[offs+2]);
				offs += 3;
				break;
			}
			if (x + 1 < p->width || y + 1 < p->height)
				w += printf(",");
			if (w >= 64) {
				printf("\n");
				w = 0;
			}
		}
	}
	printf("};\n");

	return 0;
}

int read_ppm(ppm_t *p, const char *filename)
{
	char w_h_line[32];
	char mask_line[32];
	const char *name;
	char *dot;
	int state = 0;
	int offs = 0;
	int size = 0;
	int ch;
	FILE *fp;

	fp = fopen(filename, "rb");

	if (!fp) {
		perror(filename);
		return -1;
	}
	name = strrchr(filename, '/');
	if (name)
		name++;
	else
		name = filename;
	p->name = strdup(name);
	if (!p->name) {
		perror("strdup()");
		return -1;
	}
	dot = strrchr(p->name, '.');
	if (dot)
		*dot = '\0';
	state = 0;
	while ((ch = fgetc(fp)) != -1) {
		switch (state) {
		case 0:	/* first char must be P */
			if (ch != 'P') {
				fprintf(stderr,
					"No PPM file (first char isn't 'P') '%s'\n",
					filename);
				return -1;
			}
			state++;
			break;
		case 1:	/* second char is mode; we support only P6 for now */
			if (ch != '6') {
				fprintf(stderr,
					"Unsupported PPM format (second char isn't '6') '%s'\n",
					filename);
				return -1;
			}
			/* 3 bytes R,G,B data */
			p->mode = 3;
			state++;
			break;
		case 2: /* wait for end of line */
			if (ch != '\n')
				break;
			state++;
			break;
		case 3: /* ignore comments */
			if (ch == '#') {
				state = 2;
				break;
			}
			p->width = 0;
			p->height = 0;
			offs = 0;
			size = sizeof(w_h_line);
			state++;
			/* FALLTHRU */
		case 4:	/* read width and height line */
			if (ch == '\n') {
				w_h_line[offs] = '\0';
				if (w_h_line[0] == '#')
					break;
				if (2 != sscanf(w_h_line, "%d %d", &p->width, &p->height)) {
					fprintf(stderr,
						"fatal: PPM width height line format (%s) '%s'\n",
						w_h_line, filename);
					return -1;
				}
				offs = 0;
				size = sizeof(mask_line);
				state++;
				break;
			}
			w_h_line[offs++] = ch;
			if (offs >= size) {
				w_h_line[--offs] = '\0';
				fprintf(stderr,
					"fatal: PPM width height line too long (%s) '%s'\n",
					w_h_line, filename);
				return -1;
			}
			break;
		case 5:	/* read mask line */
			if (ch == '\n') {
				mask_line[offs] = '\0';
				if (mask_line[0] == '#')
					break;
				if (1 != sscanf(mask_line, "%d", &p->mask)) {
					fprintf(stderr,
						"fatal: PPM mask line format (%s) '%s'\n",
						mask_line, filename);
					return -1;
				}
				offs = 0;
				size = p->width * p->height * 3;
				p->data = malloc(size * 3);
				state++;
				break;
			}
			mask_line[offs++] = ch;
			if (offs >= size) {
				mask_line[--offs] = '\0';
				fprintf(stderr,
					"fatal: PPM mask line too long (%s) '%s'\n",
					mask_line, filename);
				return -1;
			}
			break;
		case 6:	/* read R,G,B data */
			p->data[offs++] = ch & p->mask;
			if (offs >= size)
				state++;
			break;
		default:
			/* ignoring data beyond image */
			break;
		}
	}
	fclose(fp);
	return 0;
}

int main(int argc, char **argv)
{
	ppm_t ppm;
	char *program;
	char *filename;
	int i;

	program = strrchr(argv[0], '/');
	program = program ? program + 1 : argv[0];
	

	printf("/******************************************\n");
	printf(" * raw PPM files converted to byte arrays\n");
	printf(" ******************************************/\n");
	printf("#include \"alto.h\"\n");
	printf("\n");

	for (i = 1; i < argc; i++) {
		filename = argv[i];

		/* Don't try to convert ppm2c binary */
		if (!strcmp(argv[i], program))
			continue;

		memset(&ppm, 0, sizeof(ppm));
		if (0 == read_ppm(&ppm, filename))
			ppm_to_c_source(&ppm);
		if (ppm.name)
			free(ppm.name);
		if (ppm.data)
			free(ppm.data);
	}

	return 0;
}
