/* See LICENSE file for copyright and license details. */
#include "common.h"


const char *argv0 = "editasroot/copier";


int
main(int argc, char *argv[])
{
	int filefd, parentfd, ok;
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
	copy_file(parentfd, "<socket to parent>", filefd, path, NULL);
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
	copy_file(filefd, path, parentfd, "<socket to parent>", &ok);
	if (close(filefd)) {
		fprintf(stderr, "%s: write %s: %s\n", argv0, path, strerror(errno));
		exit(1);
	}
	if (close(parentfd)) {
		fprintf(stderr, "%s: read <socket to parent>: %s\n", argv0, strerror(errno));
		exit(1);
	}
	if (!ok) {
		fprintf(stderr, "%s: parent exited before sending file termination message, file may be truncated\n", argv0);
		exit(1);
	}

	return 0;
}
