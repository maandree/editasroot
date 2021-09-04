/* See LICENSE file for copyright and license details. */
#include "common.h"


void
copy_file(int destfd, const char *destfname, int srcfd, const char *srcfname, int *okp)
{
	char buf[BUFSIZ];
	ssize_t r, w, p, ok_off;
	size_t read_off;

	if (okp) {
		*okp = 0;
		ok_off = 1;
		read_off = 0;
	} else {
		ok_off = 0;
		read_off = 1;
		buf[0] = 0;
	}

	for (;;) {
		r = read(srcfd, &buf[read_off], sizeof(buf) - read_off);
		if (r <= 0) {
			if (!r)
				break;
			fprintf(stderr, "%s: read %s: %s\n", argv0, srcfname, strerror(errno));
			exit(1);
		}

		if (r == ok_off) { /* Only possible if okp != NULL */
			*okp = 1;
			break;
		}

		r += (ssize_t)read_off;
		for (p = ok_off; p < r; p += w) {
			w = write(destfd, buf, (size_t)(r - p));
			if (r <= 0) {
				fprintf(stderr, "%s: write %s: %s\n", argv0, destfname, strerror(errno));
				exit(1);
			}
		}
	}

	if (!okp) {
		w = write(destfd, buf, 1);
		if (r <= 0) {
			fprintf(stderr, "%s: write %s: %s\n", argv0, destfname, strerror(errno));
			exit(1);
		}
	}
}
