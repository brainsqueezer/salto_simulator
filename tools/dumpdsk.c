#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define	PAGENO_WORDS	1
#define	HEADER_WORDS	2
#define	LABEL_WORDS	8
#define	DATA_WORDS	256

#define	SPT		12
#define	TRACKS		2

#define	CKSUM_MAGIC	0521	/* contained in constan PROM address 035 */

typedef struct {
	unsigned char pageno[2*PAGENO_WORDS];
	unsigned char header[2*HEADER_WORDS];
	unsigned char label[2*LABEL_WORDS];
	unsigned char data[2*DATA_WORDS];
}	sector_t;

sector_t buffer;

typedef enum {
	base_OCT,
	base_DEC,
	base_HEX,
	base_COUNT
}	base_t;

base_t base = base_OCT;

static const char *addr_format[base_COUNT] = {"%04o:", "%03d:", "%02x:"};
static const char *data_format[base_COUNT] = {" %06o", " %05d", " %04x"};


#define	GET_WORD(field,offs) ((field)[2*(offs)] + 256 * (field)[2*(offs)+1])

#define	ASCII(n) ((n) < 32 || (n) > 126 ? '.' : (n))

int usage(int argc, char **argv)
{
	char *program = strrchr(argv[0], '/');
	if (NULL == program)
		program = argv[0];
	fprintf(stderr, "usage: %s [-o|-d|-x] diskimage.dsk\n", program);
	fprintf(stderr, "-o   octal display (default)\n");
	fprintf(stderr, "-d   decimal display (default)\n");
	fprintf(stderr, "-x   hexadecimal display (default)\n");
	return 0;
}

int main(int argc, char **argv)
{
	int i, lba, cylinder, track, sector, data, cksum;
	char *filename = NULL;
	FILE *fp;

	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
			case 'o':
				base = base_OCT;
				break;
			case 'd':
				base = base_DEC;
				break;
			case 'x':
				base = base_HEX;
				break;
			case 'h':
				usage(argc, argv);
				exit(0);
			default:
				fprintf(stderr, "unknown switch: %s\n", argv[i]);
				exit(1);
			}
		} else {
			filename = argv[i];
		}
	}

	if (!filename) {
	}

	fp = fopen(filename, "rb");
	if (!fp) {
		perror(filename);
		exit(1);
	}

	cylinder = 0;
	track = 0;
	sector = 0;
	lba = 0;
	while (!feof(fp)) {
		if (sizeof(buffer) != fread(&buffer, 1, sizeof(buffer), fp)) {
			perror("fread()");
			return 1;
		}
		printf("cylinder:%d track:%d sector:%d (lba:%d)\n",
			cylinder, track, sector, lba);

		printf("pageno:");
		for (i = 0, cksum = CKSUM_MAGIC; i < PAGENO_WORDS; i++) {
			data = GET_WORD(buffer.pageno, i);
			cksum ^= data;
			printf(data_format[base], data);
		}
		printf(" cksum:");
		printf(data_format[base], cksum);
		printf("\n");

		printf("header:");
		for (i = 0, cksum = CKSUM_MAGIC; i < HEADER_WORDS; i++) {
			data = GET_WORD(buffer.header, i);
			cksum ^= data;
			printf(data_format[base], data);
		}
		printf(" cksum:");
		printf(data_format[base], cksum);
		printf("\n");

		printf("label:");
		for (i = 0, cksum = CKSUM_MAGIC; i < LABEL_WORDS; i++) {
			data = GET_WORD(buffer.label, i);
			cksum ^= data;
			printf(data_format[base], data);
		}
		printf(" cksum:");
		printf(data_format[base], cksum);
		printf("\n");

		printf("data:\n");
		for (i = 0, cksum = CKSUM_MAGIC; i < DATA_WORDS; i++) {
			if (0 == (i % 8))
				printf(addr_format[base], i);
			data = GET_WORD(buffer.data, i);
			cksum ^= data;
			printf(data_format[base], data);
			if (7 == (i % 8)) {
				int n;
				printf(" [");
				for (n = 2*(i-7); n < 2*(i+1); n++)
					printf("%c", ASCII(buffer.data[n^1]));
				printf("]\n");
			}
		}
		printf("cksum:");
		printf(data_format[base], cksum);
		printf("\n");

		if (++sector == SPT) {
			sector = 0;
			if (++track == TRACKS) {
				track = 0;
				++cylinder;
			}
		}
		lba++;
	}

	return 0;
}
