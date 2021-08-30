/* See LICENSE file for copyright and license details. */
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


static const char *argv0 = "editasroot/copier";


static void
copy_file(int destfd, const char *destfname, int srcfd, const char *srcfname)
{
	char buf[BUFSIZ];
	ssize_t r, w, p;

	for (;;) {
		r = read(srcfd, buf, sizeof(buf));
		if (r <= 0) {
			if (!r)
				break;
			fprintf(stderr, "%s: read %s: %s", argv0, srcfname, strerror(errno));
			exit(1);
		}

		for (p = 0; p < r; p += w) {
			w = write(destfd, buf, (size_t)(r - p));
			if (r <= 0) {
				fprintf(stderr, "%s: write %s: %s", argv0, destfname, strerror(errno));
				exit(1);
			}
		}
	}
}


int
main(int argc, char *argv[])
{
	int filefd, parentfd;
	const char *path;
	ssize_t r;
	char b;

	if (!argc)
		exit(1);
	argv0 = *argv++;
	if (--argc != 2)
		exit(1);

	path = argv[0];
	parentfd = atoi(argv[1]);

	filefd = open(path, O_RDWR); /* O_RDWR establishes that we can indeed write to the file */
	if (filefd < 0) {
		fprintf(stderr, "%s: open %s O_RDWR: %s\n", argv0, path, strerror(errno));
		exit(1);
	}
	copy_file(parentfd, "<socket to parent>", filefd, path);
	if (close(filefd)) {
		fprintf(stderr, "%s: read %s: %s\n", argv0, path, strerror(errno));
		exit(1);
	}
	if (shutdown(parentfd, SHUT_WR)) {
		fprintf(stderr, "%s: shutdown <socket to parent> SHUT_WR: %s\n", argv0, strerror(errno));
		exit(1);
	}

	r = read(parentfd, &b, 1);
	if (r != 1) {
		if (r)
			fprintf(stderr, "%s: read %s: %s\n", argv0, path, strerror(errno));
		exit(1);
	}

	filefd = open(path, O_WRONLY | O_TRUNC);
	if (filefd < 0) {
		fprintf(stderr, "%s: open %s O_WRONLY|O_TRUNC: %s\n", argv0, path, strerror(errno));
		exit(1);
	}
	copy_file(filefd, path, parentfd, "<socket to parent>");
	if (close(filefd)) {
		fprintf(stderr, "%s: write %s: %s\n", argv0, path, strerror(errno));
		exit(1);
	}
	if (close(parentfd)) {
		fprintf(stderr, "%s: read <socket to parent>: %s\n", argv0, strerror(errno));
		exit(1);
	}

	return 0;
}
