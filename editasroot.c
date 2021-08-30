/* See LICENSE file for copyright and license details. */
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TEMPFILEPATTERN "tmpXXXXXX"


static const char *argv0 = "editasroot";
static const char *unlink_this = NULL;


static void
usage(void)
{
	fprintf(stderr, "usage: %s file\n", argv0);
	exit(1);
}


static void
cleanup(void)
{
	if (unlink_this) {
		unlink(unlink_this);
		unlink_this = NULL;
	}
}


static void
graceful_exit(int signo)
{
	cleanup();
	if (signal(signo, SIG_DFL) != SIG_ERR)
		raise(signo);
	exit(1);
}


static const char *
get_editor(void)
{
	const char *editor = NULL;
	int fd, isbg;

	fd = open(_PATH_TTY, O_RDONLY);
        if (fd < 0) {
		fprintf(stderr, "%s: open %s O_RDONLY: %s", argv0, _PATH_TTY, strerror(errno));
		exit(1);
	}
        isbg = getpgrp() != tcgetpgrp(fd);
        close(fd);

	if (isbg)
		editor = getenv("VISUAL");
	if (!editor || !*editor) {
		editor = getenv("EDITOR");
		if (!editor || !*editor) {
			fprintf(stderr, "%s: warning: EDITOR not set, defaulting to 'vi'\n", argv0);
			editor = "vi";
		}
	}

	return editor;
}


static char *
get_tmpfile_pattern(void)
{
	const char *tmpdir = NULL;
	char *path, *p;

	tmpdir = getenv("XDG_RUNTIME_DIR");
	if (!tmpdir || !*tmpdir)
		tmpdir = _PATH_TMP;

	path = malloc(strlen(tmpdir) + sizeof("/"TEMPFILEPATTERN));
	if (!path) {
		fprintf(stderr, "%s: malloc: %s", argv0, strerror(ENOMEM));
		exit(1);
	}
	p = stpcpy(path, tmpdir);
	p -= p[-1] == '/';
	stpcpy(p, "/"TEMPFILEPATTERN);

	return path;
}


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


static pid_t
run_child(const char *file, int fd, int close_this)
{
	char fdstr[3 * sizeof(fd) + 2];
	pid_t pid;

	switch (pid = fork()) {
	case -1:
		fprintf(stderr, "%s: fork: %s", argv0, strerror(errno));
		exit(1);

	case 0:
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(close_this);
		prctl(PR_SET_PDEATHSIG, SIGKILL);
		sprintf(fdstr, "%i", fd);
		execlp("asroot", "asroot", LIBEXECDIR"/copier", file, fdstr, NULL);
		fprintf(stderr, "%s: execlp asroot: %s\n", argv0, strerror(errno));
		exit(1);

	default:
		return pid;
	}
}


static void
run_editor(const char *editor, const char *file, int close_this)
{
	pid_t pid;
	int status;

	switch (pid = fork()) {
	case -1:
		fprintf(stderr, "%s: fork: %s", argv0, strerror(errno));
		exit(1);

	case 0:
		close(close_this);
		execlp(editor, editor, "--", file, NULL);
		fprintf(stderr, "%s: execlp %s: %s\n", argv0, editor, strerror(errno));
		exit(1);

	default:
		break;
	}

	if (waitpid(pid, &status, 0) != pid) {
		fprintf(stderr, "%s: waitpid %s 0: %s", argv0, editor, strerror(errno));
		exit(1);
	}

	if (status)
		exit(WIFEXITED(status) ? WEXITSTATUS(status) : 1);
}


int
main(int argc, char *argv[])
{
	const char *editor;
	char *path;
	int fd, fds[2], status, i;
	pid_t pid;

	if (!argc)
		usage();
	argv0 = *argv++;
	if (--argc != 1)
		usage();

	editor = get_editor();

	/* Start copier, with a bidirectional channel to it for copying the file */
	if (socketpair(PF_LOCAL, SOCK_STREAM, 0, fds)) {
		fprintf(stderr, "%s: socketpair PF_LOCAL SOCK_STREAM 0: %s", argv0, strerror(errno));
		exit(1);
	}
	pid = run_child(argv[0], fds[1], fds[0]);
	close(fds[1]);

	/* Make sure (enough) tmpfile is unlinked if killed */
	for (i = 1; i < NSIG; i++)
		if (i != SIGCHLD)
			signal(i, graceful_exit);

	/* Create tmpfile from file to edit */
	path = get_tmpfile_pattern();
	fd = mkstemp(path);
	if (fd < 0) {
		fprintf(stderr, "%s: mkstemp %s: %s", argv0, path, strerror(errno));
		exit(1);
	}
	unlink_this = path;
	if (atexit(cleanup)) {
		fprintf(stderr, "%s: atexit: %s", argv0, strerror(errno));
		exit(1);
	}
	copy_file(fd, path, fds[0], "<socket to child>");
	if (close(fd)) {
		fprintf(stderr, "%s: write %s: %s", argv0, path, strerror(errno));
		exit(1);
	}
	if (shutdown(fds[0], SHUT_RD)) {
		fprintf(stderr, "%s: shutdown <socket to parent> SHUT_RD: %s\n", argv0, strerror(errno));
		exit(1);
	}

	/* Start file editor */
	run_editor(editor, path, fds[0]);

	/* Signal to child that editor exited as expected */
	if (write(fds[0], &(char){1}, 1) != 1) {
		fprintf(stderr, "%s: write <socket to child>: %s", argv0, strerror(errno));
		exit(1);
	}

	/* Rewrite file from tmpfile and unlink tmpfile */
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "%s: open %s O_RDONLY: %s", argv0, path, strerror(errno));
		exit(1);
	}
	copy_file(fds[0], "<socket to child>", fd, path);
	if (close(fd)) {
		fprintf(stderr, "%s: read %s: %s", argv0, path, strerror(errno));
		exit(1);
	}
	if (close(fds[0])) {
		fprintf(stderr, "%s: write <socket to child>: %s", argv0, strerror(errno));
		exit(1);
	}
	unlink(unlink_this);
	unlink_this = NULL;
	free(path);

	/* Wait for exit copier to exit */
	if (waitpid(pid, &status, 0) != pid) {
		fprintf(stderr, "%s: waitpid <child> 0: %s", argv0, strerror(errno));
		exit(1);
	}
	return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}
