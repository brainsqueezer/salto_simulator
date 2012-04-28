#include <stdlib.h>
#include <stdio.h>
#include <math.h>

int main(void)
{
	int i;
	float f;

	printf("/* DIABLO 31 seek times; approx. 15 + 8.6 * sqrt(td) ms*/\n");
	printf("static const int diablo_31_seek_dt[] = {\n");
	for (i = 0; i < 203; i++) {
		f = 15.0 + sqrt(1.0 * i) * 8.6;
		printf("\t%d%s", (int)(f * 1000), i+1 < 203 ? "," : "\n");
		if (7 == (i % 8))
			printf("\n");

	}
	printf("};\n");
	printf("\n");

	printf("/* DIABLO 44 seek times; approx. 8 + 3 * sqrt(td) ms */\n");
	printf("static const int diablo_44_seek_dt[] = {\n");
	for (i = 0; i < 406; i++) {
		f = 8.0 + sqrt(1.0 * i) * 3.0;
		printf("\t%d%s", (int)(f * 1000), i+1 < 406 ? "," : "\n");
		if (7 == (i % 8))
			printf("\n");
	}
	printf("};\n");
	printf("\n");
	return 0;
}
