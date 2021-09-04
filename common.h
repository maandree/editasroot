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
#include <termios.h>
#include <unistd.h>

/* We need at least 2 bytes we are sending 1 addition byte in each chunk */
#if BUFSIZ < 2
# undef BUFSIZ
# define BUFSIZ 2
#endif

/* We impose a limit because PF_UNIX/SOCK_SEQPACKET can block if sending
 * too much (I don't know the exact limit and it can change), without
 * first setting up a socket options, which is overkill for this application,
 * and there is also a hard limit just above 210000B for each EMSGSIZE will
 * be thrown. */
#if BUFSIZ > (8 << 10)
# undef BUFSIZ
# define BUFSIZ (8 << 10)
#endif


extern const char *argv0;

void copy_file(int destfd, const char *destfname, int srcfd, const char *srcfname, int *okp);
