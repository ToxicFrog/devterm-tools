/* See LICENSE for licence details. */
/* yaft.c: include main function */

// for memmem()
#define _GNU_SOURCE

#include "yaft.h"
#include "conf.h"
#include "util.h"
#include "fb/common.h"
#include "terminal.h"
#include "ctrlseq/esc.h"
#include "ctrlseq/csi.h"
#include "ctrlseq/osc.h"
#include "ctrlseq/dcs.h"
#include "parse.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

void sig_handler(int signo)
{
	sigset_t sigset;
	/* global */
	extern volatile sig_atomic_t vt_active;
	extern volatile sig_atomic_t child_alive;
	extern volatile sig_atomic_t need_redraw;

	logging(DEBUG, "caught signal! no:%d\n", signo);

	if (signo == SIGCHLD) {
		child_alive = false;
		wait(NULL);
	} else if (signo == SIGUSR1) { /* vt activate */
		vt_active   = true;
		need_redraw = true;
		ioctl(STDIN_FILENO, VT_RELDISP, VT_ACKACQ);
	} else if (signo == SIGUSR2) { /* vt deactivate */
		vt_active = false;
		ioctl(STDIN_FILENO, VT_RELDISP, 1);

		if (BACKGROUND_DRAW) { /* update passive cursor */
			need_redraw = true;
		} else {               /* sleep until next vt switching */
			sigfillset(&sigset);
			sigdelset(&sigset, SIGUSR1);
			sigsuspend(&sigset);
		}
	}
}

void set_rawmode(int fd, struct termios *save_tm)
{
	struct termios tm;

	tm = *save_tm;
	tm.c_iflag     = tm.c_oflag = 0;
	tm.c_cflag    &= ~CSIZE;
	tm.c_cflag    |= CS8;
	tm.c_lflag    &= ~(ECHO | ISIG | ICANON);
	tm.c_cc[VMIN]  = 1; /* min data size (byte) */
	tm.c_cc[VTIME] = 0; /* time out */
	etcsetattr(fd, TCSAFLUSH, &tm);
}

bool tty_init(struct termios *termios_orig)
{
	struct sigaction sigact;

	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = sig_handler;
	sigact.sa_flags   = SA_RESTART;
	esigaction(SIGCHLD, &sigact, NULL);

	if (VT_CONTROL) {
		esigaction(SIGUSR1, &sigact, NULL);
		esigaction(SIGUSR2, &sigact, NULL);

		struct vt_mode vtm;
		vtm.mode   = VT_PROCESS;
		vtm.waitv  = 0;
		vtm.acqsig = SIGUSR1;
		vtm.relsig = SIGUSR2;
		vtm.frsig  = 0;

		if (ioctl(STDIN_FILENO, VT_SETMODE, &vtm))
			logging(WARN, "ioctl: VT_SETMODE failed (maybe here is not console)\n");

		if (FORCE_TEXT_MODE == false) {
			if (ioctl(STDIN_FILENO, KDSETMODE, KD_GRAPHICS))
				logging(WARN, "ioctl: KDSETMODE failed (maybe here is not console)\n");
		}
	}

	etcgetattr(STDIN_FILENO, termios_orig);
	set_rawmode(STDIN_FILENO, termios_orig);
	ewrite(STDIN_FILENO, "\033[?25l", 6); /* make cusor invisible */

	return true;
}

void tty_die(struct termios *termios_orig)
{
 	/* no error handling */
	struct sigaction sigact;
	struct vt_mode vtm;

	memset(&sigact, 0, sizeof(struct sigaction));
	sigact.sa_handler = SIG_DFL;
	sigaction(SIGCHLD, &sigact, NULL);

	if (VT_CONTROL) {
		sigaction(SIGUSR1, &sigact, NULL);
		sigaction(SIGUSR2, &sigact, NULL);

		vtm.mode   = VT_AUTO;
		vtm.waitv  = 0;
		vtm.relsig = vtm.acqsig = vtm.frsig = 0;

		ioctl(STDIN_FILENO, VT_SETMODE, &vtm);

		if (FORCE_TEXT_MODE == false)
			ioctl(STDIN_FILENO, KDSETMODE, KD_TEXT);
	}

	tcsetattr(STDIN_FILENO, TCSAFLUSH, termios_orig);
	fflush(stdout);
	ewrite(STDIN_FILENO, "\033[?25h", 6); /* make cursor visible */
}

bool fork_and_exec(int *master, const char *cmd, char *const argv[], int lines, int cols)
{
	pid_t pid;
	struct winsize ws = {.ws_row = lines, .ws_col = cols,
		/* XXX: this variables are UNUSED (man tty_ioctl),
			but useful for calculating terminal cell size */
		.ws_ypixel = CELL_HEIGHT * lines, .ws_xpixel = CELL_WIDTH * cols};

	pid = eforkpty(master, NULL, NULL, &ws);
	if (pid < 0)
		return false;
	else if (pid == 0) { /* child */
		esetenv("TERM", term_name, 1);
		eexecvp(cmd, argv);
		/* never reach here */
		exit(EXIT_FAILURE);
	}
	return true;
}

int check_fds(fd_set *fds, struct timeval *tv, int input, int master)
{
	FD_ZERO(fds);
	FD_SET(input, fds);
	FD_SET(master, fds);
	tv->tv_sec  = 0;
	tv->tv_usec = SELECT_TIMEOUT;
	return eselect(master + 1, fds, NULL, NULL, tv);
}

void process_shifted_arrows(int fd, int modifiers, char ch) {
		// Translate kernel modifier state into xterm modifier state.
		// xterm uses shift=1, alt=2, ctrl=4, then adds one to the final value
		uint8_t mod_code = 1
			+ ((modifiers & 1<<KG_SHIFT) ? 1 : 0)
			+ ((modifiers & 1<<KG_ALT) ? 2 : 0)
			+ ((modifiers & 1<<KG_CTRL) ? 4 : 0);
		dprintf(fd, "\x1B[1;%d%c", mod_code, ch);
}

void process_shifted_nav(int fd, int modifiers, char ch) {
		uint8_t mod_code = 1
			+ ((modifiers & 1<<KG_SHIFT) ? 1 : 0)
			+ ((modifiers & 1<<KG_ALT) ? 2 : 0)
			+ ((modifiers & 1<<KG_CTRL) ? 4 : 0);
		dprintf(fd, "\x1B[%c;%d~", ch, mod_code);
}

void process_input(int fd, uint8_t * buf, ptrdiff_t size) {
	uint8_t * cur = buf;
	int modifiers = get_modifier_state(STDIN_FILENO);

	if (!modifiers) {
		// No modifier keys held down, so we don't need to do anything, yay
		ewrite(fd, buf, size);
		return;
	}

	while ((cur = memmem(cur, size, "\x1B[", 2)) != NULL) {
		// Found a CSI sequence. First some checks to see if we care about it.
		// If it's at the end of the buffer and there's no room for a split escape
		// sequence, just give up. Hopefully this is vanishingly rare. We don't
		// bother re-calling memmem() because we know we're at the end of the buffer.
		if (cur - buf >= size-2) break;

		// Arrow keys are CSI A through CSI D for up/down/right/left.
		if (cur[2] >= 'A' && cur[2] <= 'D') {
			ewrite(fd, buf, cur-buf);
			process_shifted_arrows(fd, modifiers, cur[2]);
			cur += 3;
			size -= cur - buf;
			buf = cur;
			continue;
		}

		// Nav keys. These sequences are one byte longer:
		// CSI 1 ~ through CSI 6 ~ for home/ins/del/end/pgup/pgdn.
		if (cur - buf >= size-3) break;

		if (cur[3] == '~' && cur[2] >= '1' && cur[2] <= '6') {
			ewrite(fd, buf, cur-buf);
			process_shifted_nav(fd, modifiers, cur[2]);
			cur += 4;
			size -= cur - buf;
			buf = cur;
			continue;
		}

		// It's not anything we care about, so just advance the cursor past it and
		// keep going.
		cur += 2;
	}
	ewrite(fd, buf, size);
}

int main(int argc, char *const argv[])
{
	extern const char *shell_cmd; /* defined in conf.h */
	const char *cmd;
	uint8_t buf[BUFSIZE];
	ssize_t size;
	fd_set fds;
	struct timeval tv;
	struct framebuffer_t fb;
	struct terminal_t term;
	/* global */
	extern volatile sig_atomic_t need_redraw;
	extern volatile sig_atomic_t child_alive;
	extern struct termios termios_orig;

	/* init */
	if (setlocale(LC_ALL, "") == NULL) /* for wcwidth() */
		logging(WARN, "setlocale failed\n");

	if (!tty_init(&termios_orig)) {
		logging(FATAL, "tty initialize failed\n");
		goto tty_init_failed;
	}

	if (!fb_init(&fb)) {
		logging(FATAL, "framebuffer initialize failed\n");
		goto fb_init_failed;
	}

	if (!term_init(&term, &fb)) {
		logging(FATAL, "terminal initialize failed\n");
		goto term_init_failed;
	}

	/* fork and exec shell */
	cmd = (argc < 2) ? shell_cmd: argv[1];
	if (!fork_and_exec(&term.fd, cmd, argv + 1, term.lines, term.cols)) {
		logging(FATAL, "forkpty failed\n");
		goto tty_init_failed;
	}
	child_alive = true;

	/* main loop */
	while (child_alive) {
		if (need_redraw) {
			need_redraw = false;
			cmap_update(fb.fd, fb.cmap); /* after VT switching, need to restore cmap (in 8bpp mode) */
			redraw(&term);
			refresh(&fb, &term);
		}

		if (check_fds(&fds, &tv, STDIN_FILENO, term.fd) == -1)
			continue;

		if (FD_ISSET(STDIN_FILENO, &fds)) {
			if ((size = read(STDIN_FILENO, buf, BUFSIZE)) > 0) {
				process_input(term.fd, buf, size);
			}
		}
		if (FD_ISSET(term.fd, &fds)) {
			if ((size = read(term.fd, buf, BUFSIZE)) > 0) {
				if (VERBOSE)
					ewrite(STDOUT_FILENO, buf, size);
				parse(&term, buf, size);
				if (LAZY_DRAW && size == BUFSIZE)
					continue; /* maybe more data arrives soon */
				refresh(&fb, &term);
			}
		}
	}

	/* normal exit */
	tty_die(&termios_orig);
	term_die(&term);
	fb_die(&fb);
	return EXIT_SUCCESS;

	/* error exit */
tty_init_failed:
	term_die(&term);
term_init_failed:
	fb_die(&fb);
fb_init_failed:
	return EXIT_FAILURE;
}
