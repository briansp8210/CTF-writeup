#include <stdlib.h>

int main(void)
{
    int *p, *q, *r, *s, *t, *u;

    p = malloc(0x32000);
    free(p);
    q = malloc(0x32000);
    free(q);
    r = malloc(0x40000);
    free(r);
    s = malloc(0x40000);
    free(s);
	t = malloc(0x64000);
	free(t);
	u = malloc(0x64000);
	free(u);

    return 0;
}
