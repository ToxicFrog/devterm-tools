#include <time.h>

/* See LICENSE for licence details. */
void erase_cell(struct terminal_t *term, int y, int x)
{
	struct cell_t *cellp;

	cellp             = &term->cells[y][x];
	cellp->glyphp     = term->glyph[DEFAULT_CHAR];
	cellp->color_pair = term->color_pair; /* bce */
	cellp->attribute  = ATTR_RESET;
	cellp->width      = HALF;
	cellp->has_pixmap = false;

	term->line_dirty[y] = true;
}

void copy_cell(struct terminal_t *term, int dst_y, int dst_x, int src_y, int src_x)
{
	struct cell_t *dst, *src;

	dst = &term->cells[dst_y][dst_x];
	src = &term->cells[src_y][src_x];

	if (src->width == NEXT_TO_WIDE) {
		return;
	} else if (src->width == WIDE && dst_x == (term->cols - 1)) {
		erase_cell(term, dst_y, dst_x);
	} else {
		*dst = *src;
		if (src->width == WIDE) {
			*(dst + 1) = *src;
			(dst + 1)->width = NEXT_TO_WIDE;
		}
		term->line_dirty[dst_y] = true;
	}
}

int set_cell(struct terminal_t *term, int y, int x, const struct glyph_t *glyphp)
{
	struct cell_t cell, *cellp;
	uint8_t color_tmp;

	cell.glyphp = glyphp;

	cell.color_pair.fg = term->color_pair.fg;
	cell.color_pair.bg = (term->attribute & attr_mask[ATTR_BLINK] && term->color_pair.bg <= 7) ?
		term->color_pair.bg + BRIGHT_INC: term->color_pair.bg;

	if (term->attribute & attr_mask[ATTR_REVERSE]) {
		color_tmp          = cell.color_pair.fg;
		cell.color_pair.fg = cell.color_pair.bg;
		cell.color_pair.bg = color_tmp;
	}

	cell.attribute  = term->attribute;
	cell.width      = glyphp->width;
	cell.has_pixmap = false;

	cellp    = &term->cells[y][x];
	*cellp   = cell;
	term->line_dirty[y] = true;

	if (cell.width == WIDE && x + 1 < term->cols) {
		cellp        = &term->cells[y][x + 1];
		*cellp       = cell;
		cellp->width = NEXT_TO_WIDE;
		return WIDE;
	}

	if (cell.width == HALF /* isolated NEXT_TO_WIDE cell */
		&& x + 1 < term->cols
		&& term->cells[y][x + 1].width == NEXT_TO_WIDE) {
		erase_cell(term, y, x + 1);
	}
	return HALF;
}

static inline void swap_lines(struct terminal_t *term, int i, int j)
{
	struct cell_t *tmp;

	tmp            = term->cells[i];
	term->cells[i] = term->cells[j];
	term->cells[j] = tmp;
}

void scroll(struct terminal_t *term, int from, int to, int offset)
{
	int abs_offset, scroll_lines;

	if (offset == 0 || from >= to)
		return;

	logging(DEBUG, "scroll from:%d to:%d offset:%d\n", from, to, offset);

	for (int y = from; y <= to; y++)
		term->line_dirty[y] = true;

	abs_offset = abs(offset);
	scroll_lines = (to - from + 1) - abs_offset;

	if (offset > 0) { /* scroll down */
		for (int y = from; y < from + scroll_lines; y++)
			swap_lines(term, y, y + offset);
		for (int y = (to - offset + 1); y <= to; y++)
			for (int x = 0; x < term->cols; x++)
				erase_cell(term, y, x);
	}
	else {            /* scroll up */
		for (int y = to; y >= from + abs_offset; y--)
			swap_lines(term, y, y - abs_offset);
		for (int y = from; y < from + abs_offset; y++)
			for (int x = 0; x < term->cols; x++)
				erase_cell(term, y, x);
	}
}

/* relative movement: cause scrolling */
void move_cursor(struct terminal_t *term, int y_offset, int x_offset)
{
	int x, y, top, bottom;

	x = term->cursor.x + x_offset;
	y = term->cursor.y + y_offset;

	top    = term->scroll.top;
	bottom = term->scroll.bottom;

	if (x < 0) {
		x = 0;
	} else if (x >= term->cols) {
		if (term->mode & MODE_AMRIGHT)
			term->wrap_occurred = true;
		x = term->cols - 1;
	}
	term->cursor.x = x;

	y = (y < 0) ? 0:
		(y >= term->lines) ? term->lines - 1: y;

	if (term->cursor.y == top && y_offset < 0) {
		y = top;
		scroll(term, top, bottom, y_offset);
	} else if (term->cursor.y == bottom && y_offset > 0) {
		y = bottom;
		scroll(term, top, bottom, y_offset);
	}
	term->cursor.y = y;
}

/* absolute movement: never scroll */
void set_cursor(struct terminal_t *term, int y, int x)
{
	int top, bottom;

	if (term->mode & MODE_ORIGIN) {
		top    = term->scroll.top;
		bottom = term->scroll.bottom;
		y += term->scroll.top;
	} else {
		top = 0;
		bottom = term->lines - 1;
	}

	x = (x < 0) ? 0: (x >= term->cols) ? term->cols - 1: x;
	y = (y < top) ? top: (y > bottom) ? bottom: y;

	term->cursor.x = x;
	term->cursor.y = y;
	term->wrap_occurred = false;
}

const struct glyph_t *drcs_glyph(struct terminal_t *term, uint32_t code)
{
	/* DRCSMMv1
		ESC ( SP <\xXX> <\xYY> ESC ( B
		<===> U+10XXYY ( 0x40 <= 0xXX <=0x7E, 0x20 <= 0xYY <= 0x7F )
	*/
	int row, cell; /* = ku, ten */

	row  = (0xFF00 & code) >> 8;
	cell = 0xFF & code;

	logging(DEBUG, "drcs row:0x%.2X cell:0x%.2X\n", row, cell);

	if ((0x40 <= row && row <= 0x7E) && (0x20 <= cell && cell <= 0x7F))
		return &term->drcs[(row - 0x40) * GLYPHS_PER_CHARSET + (cell - 0x20)];
	else
		return term->glyph[SUBSTITUTE_HALF];
}

const struct glyph_t *find_glyph(struct terminal_t *term, uint32_t code) {
	if (0x100000 <= code && code <= 0x10FFFD) /* unicode private area: plane 16 (DRCSMMv1) */
		return drcs_glyph(term, code);

	/* codepoint outside supported range or no glyph for this codepoint */
	if (code >= UCS2_CHARS || term->glyph[code] == NULL)
		return (wcwidth(code) == 2) ? term->glyph[SUBSTITUTE_WIDE]: term->glyph[SUBSTITUTE_HALF];

	return term->glyph[code];
}

void addch(struct terminal_t *term, uint32_t code)
{
	logging(DEBUG, "addch: U+%.4X : %lc\n", code, (wint_t)code);

	const struct glyph_t *glyphp = find_glyph(term, code);
	if (!glyphp) return;

	if ((term->wrap_occurred && term->cursor.x == term->cols - 1) /* folding */
		|| (glyphp->width == WIDE && term->cursor.x == term->cols - 1)) {
		set_cursor(term, term->cursor.y, 0);
		move_cursor(term, 1, 0);
	}
	term->wrap_occurred = false;

	move_cursor(term, 0, set_cell(term, term->cursor.y, term->cursor.x, glyphp));
}

void reset_esc(struct terminal_t *term)
{
	logging(DEBUG, "*esc reset*\n");

	term->esc.bp    = term->esc.buf;
	term->esc.state = STATE_RESET;
}

bool push_esc(struct terminal_t *term, uint8_t ch)
{
	long offset;

	if ((term->esc.bp - term->esc.buf) >= term->esc.size) { /* buffer limit */
		logging(DEBUG, "escape sequence length >= %d, term.esc.buf reallocated\n", term->esc.size);
		offset = term->esc.bp - term->esc.buf;
		term->esc.buf = erealloc(term->esc.buf, term->esc.size * 2);
		term->esc.bp  = term->esc.buf + offset;
		term->esc.size *= 2;
	}

	/* ref: http://www.vt100.net/docs/vt102-ug/appendixd.html */
	*term->esc.bp++ = ch;
	if (term->esc.state == STATE_ESC) {
		/* format:
			ESC  I.......I F
				 ' '  '/'  '0'  '~'
			0x1B 0x20-0x2F 0x30-0x7E
		*/
		if ('0' <= ch && ch <= '~')        /* final char */
			return true;
		else if (SPACE <= ch && ch <= '/') /* intermediate char */
			return false;
	} else if (term->esc.state == STATE_CSI) {
		/* format:
			CSI       P.......P I.......I F
			ESC  '['  '0'  '?'  ' '  '/'  '@'  '~'
			0x1B 0x5B 0x30-0x3F 0x20-0x2F 0x40-0x7E
		*/
		if ('@' <= ch && ch <= '~')
			return true;
		else if (SPACE <= ch && ch <= '?')
			return false;
	} else {
		/* format:
			OSC       I.....I F
			ESC  ']'          BEL  or ESC  '\'
			0x1B 0x5D unknown 0x07 or 0x1B 0x5C
			DCS       I....I  F
			ESC  'P'          BEL  or ESC  '\'
			0x1B 0x50 unknown 0x07 or 0x1B 0x5C
		*/
		if (ch == BEL || (ch == BACKSLASH
			&& (term->esc.bp - term->esc.buf) >= 2 && *(term->esc.bp - 2) == ESC))
			return true;
		else if ((ch == ESC || ch == CR || ch == LF || ch == BS || ch == HT)
			|| (SPACE <= ch && ch <= '~'))
			return false;
	}

	/* invalid sequence */
	reset_esc(term);
	return false;
}

void reset_charset(struct terminal_t *term)
{
	term->charset.code = term->charset.count = term->charset.following_byte = 0;
	term->charset.is_valid = true;
}

void reset(struct terminal_t *term)
{
	term->mode  = MODE_RESET;
	term->mode |= (MODE_CURSOR | MODE_AMRIGHT);
	term->wrap_occurred = false;

	term->scroll.top    = 0;
	term->scroll.bottom = term->lines - 1;

	term->cursor.x = term->cursor.y = 0;

	term->state.mode      = term->mode;
	term->state.cursor    = term->cursor;
	term->state.attribute = ATTR_RESET;

	term->color_pair.fg = DEFAULT_FG;
	term->color_pair.bg = DEFAULT_BG;

	term->attribute = ATTR_RESET;

	for (int line = 0; line < term->lines; line++) {
		for (int col = 0; col < term->cols; col++) {
			erase_cell(term, line, col);
			if ((col % TABSTOP) == 0)
				term->tabstop[col] = true;
			else
				term->tabstop[col] = false;
		}
		term->line_dirty[line] = true;
	}

	reset_esc(term);
	reset_charset(term);
}

void redraw(struct terminal_t *term)
{
	for (int i = 0; i < term->lines; i++)
		term->line_dirty[i] = true;
}

void alloc_cells(struct terminal_t *term) {
	term->cells = (struct cell_t **) ecalloc(term->lines, sizeof(struct cell_t *));
	for (int i = 0; i < term->lines; i++)
		term->cells[i] = (struct cell_t *) ecalloc(term->cols, sizeof(struct cell_t));
}

void free_cells(struct terminal_t *term) {
	for (int i = 0; i < term->lines; i++)
		free(term->cells[i]);
	free(term->cells);
}

void term_die(struct terminal_t *term)
{
	free(term->line_dirty);
	free(term->tabstop);
	free(term->esc.buf);
	free(term->sixel.pixmap);
	free_cells(term);
}

void init_variant(struct terminal_t *term, int variant, struct variant_t glyphs[], size_t size) {
	for (uint32_t gi = 0; gi < size; gi++) {
		struct glyph_t* base_glyph = (struct glyph_t*)term->glyph[glyphs[gi].code];
		if (!base_glyph) continue;
		base_glyph->variants[variant] = &(glyphs[gi].bitmap[0]);
	}
}

void benchmark_blits(struct terminal_t *term, struct framebuffer_t *fb) {
	// Dirty the page; otherwise we get very unrealistic results that don't
	// reflect real-world performance.
	memset(fb->buf, 0x00, fb->info.screen_size);
	// First copy is perplexingly slow so get it out of the way before doing
	// real performance testing.
	memcpy(fb->fp, fb->buf, fb->info.screen_size);

	clock_t fullscreen_time = clock();
	for (int i = 0; i < 16; ++i) {
		blit_whole_fb(fb);
	}
	fullscreen_time = (clock() - fullscreen_time)/16;

	if (!is_rotated_90(fb)) {
		clock_t line_time = clock();
		for (int i = 0; i < 16; ++i) {
			for (int line = 0; line < term->lines; ++line) {
				blit_line(fb, line);
			}
		}
		line_time = (clock() - line_time)/16;
		term->dirty_threshold = term->lines * fullscreen_time / line_time;
		logging(DEBUG, "Benchmarks: full=%d line=%d ratio: %d/%d lines\n",
			fullscreen_time, line_time, term->dirty_threshold, term->lines);
		return;
	}

	clock_t cell_time = clock();
	for (int i = 0; i < 16; ++i) {
		for (int line = 0; line < term->lines; ++line) {
			for (int col = 0; col < term->cols; ++col) {
				blit_cell_rotated(fb, line, col);
			}
		}
	}
	cell_time = (clock() - cell_time)/16;
	term->dirty_threshold = term->lines * fullscreen_time / cell_time;
	logging(DEBUG, "Benchmarks: full=%d cell=%d ratio: %d/%d lines\n",
		fullscreen_time, cell_time, term->dirty_threshold, term->lines);
}

bool term_init(struct terminal_t *term, struct framebuffer_t *fb)
{
	extern const uint32_t color_list[COLORS]; /* global */

	if (is_rotated_90(fb)) {
		term->width  = fb->info.height;
		term->height = fb->info.width;
	} else {
		term->width  = fb->info.width;
		term->height = fb->info.height;
	}

	term->cols  = term->width / CELL_WIDTH;
	term->lines = term->height / CELL_HEIGHT;

	term->esc.size = ESCSEQ_SIZE;

	logging(DEBUG, "terminal cols:%d lines:%d\n", term->cols, term->lines);

	/* allocate memory */
	term->line_dirty   = (bool *) ecalloc(term->lines, sizeof(bool));
	term->tabstop      = (bool *) ecalloc(term->cols, sizeof(bool));
	term->esc.buf      = (char *) ecalloc(1, term->esc.size);
	term->sixel.pixmap = (uint8_t *) ecalloc(term->width * term->height, BYTES_PER_PIXEL);

	alloc_cells(term);

	if (!term->line_dirty || !term->tabstop || !term->cells
		|| !term->esc.buf || !term->sixel.pixmap) {
		term_die(term);
		return false;
	}

	/* initialize palette */
	for (int i = 0; i < COLORS; i++)
		term->virtual_palette[i] = color_list[i];
	term->palette_modified = false;

	/* initialize glyph map */
	for (uint32_t code = 0; code < UCS2_CHARS; code++)
		term->glyph[code] = NULL;

	for (uint32_t gi = 0; gi < sizeof(glyphs) / sizeof(struct glyph_t); gi++)
		term->glyph[glyphs[gi].code] = &glyphs[gi];

	init_variant(term, GV_BOLD, glyphs_bold, sizeof(glyphs_bold) / sizeof(struct variant_t));
	init_variant(term, GV_ITALIC, glyphs_italic, sizeof(glyphs_italic) / sizeof(struct variant_t));
	init_variant(term, GV_BOLDITALIC, glyphs_bolditalic, sizeof(glyphs_bolditalic) / sizeof(struct variant_t));

	if (!term->glyph[DEFAULT_CHAR]
		|| !term->glyph[SUBSTITUTE_HALF]
		|| !term->glyph[SUBSTITUTE_WIDE]) {
		logging(ERROR, "couldn't find essential glyph:\
			DEFAULT_CHAR(U+%.4X):%p SUBSTITUTE_HALF(U+%.4X):%p SUBSTITUTE_WIDE(U+%.4X):%p\n",
			DEFAULT_CHAR, term->glyph[DEFAULT_CHAR],
			SUBSTITUTE_HALF, term->glyph[SUBSTITUTE_HALF],
			SUBSTITUTE_WIDE, term->glyph[SUBSTITUTE_WIDE]);
		return false;
	}

	// Test how fast different drawing operations are on this platform, and
	// figure out whether there's a point where it's cheaper to redraw the
	// whole screen even if only part of it has been dirtied.
	benchmark_blits(term, fb);

	/* reset terminal */
	reset(term);

	return true;
}
