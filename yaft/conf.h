/* See LICENSE for licence details. */
/* conf.h: define custom variables */

/* color: index number of color_palette[] (see color.h) */
enum {
	DEFAULT_FG           = 7,
	DEFAULT_BG           = 0,
	ACTIVE_CURSOR_COLOR  = 3,
	PASSIVE_CURSOR_COLOR = 1,
};

/* misc */
enum {
	VERBOSE          = false,  /* write dump of input to stdout, debug message to stderr */
	TABSTOP          = 8,      /* hardware tabstop */
	LAZY_DRAW        = true,   /* don't draw when input data size is larger than BUFSIZE */
	BACKGROUND_DRAW  = false,  /* always draw even if vt is not active */
	VT_CONTROL       = true,   /* handle vt switching */
	FORCE_TEXT_MODE  = false,  /* force KD_TEXT mode (not use KD_GRAPHICS mode) */
	SUBSTITUTE_HALF  = 0x0020, /* used for missing glyph(single width): U+0020 (SPACE) */
	SUBSTITUTE_WIDE  = 0x3000, /* used for missing glyph(double width): U+3000 (IDEOGRAPHIC SPACE) */
	REPLACEMENT_CHAR = 0x003F, /* used for malformed UTF-8 sequence   : U+003F (QUESTION MARK) */
	DEAD_LINES       = 0,      /* HACK: dead lines along right side of fb on devterm */
};

/* number of font variants */
enum glyph_variant {
	GV_NORMAL = -1,
	GV_BOLD,
	GV_ITALIC,
	GV_BOLDITALIC,
	NROF_VARIANTS,
};

// If no italic font is loaded, try to slant italic characters on the fly by
// shifting the bottom third of each character to the left and the top third
// to the right.
// Commenting this out completely disables autoslanting.
// This will never be as good as using a proper oblique font face, but those
// are often hard to find for bitmapped fonts.
// Value of autoslant is how large a vertical window to use; this should
// usually be smaller than the cell height for good results, about one el high.
#define AUTOSLANT 14
// Vertical offset for autoslanting. Should be set about the same as the font's
// baseline or a bit lower.
#define AUTOSLANT_OFFSET 3

/* TERM value */
const char *term_name = "yaft-256color";

/* framubuffer device */
#if defined(__linux__)
	const char *fb_path = "/dev/fb0";
#elif defined(__FreeBSD__)
	const char *fb_path = "/dev/ttyv0";
#elif defined(__NetBSD__)
	const char *fb_path = "/dev/ttyE0";
#elif defined(__OpenBSD__)
	const char *fb_path = "/dev/ttyC0";
#elif defined(__ANDROID__)
	const char *fb_path = "/dev/graphics/fb0";
#endif

/* shell */
#if defined(__linux__) || defined(__MACH__)
	const char *shell_cmd = "/bin/bash";
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
	const char *shell_cmd = "/bin/csh";
#elif defined(__ANDROID__)
	const char *shell_cmd = "/system/bin/sh";
#endif
