# yaft (R01 fork)

This is a fork of [yaft](https://github.com/uobikiemukot/yaft) with some changes to make it usable on the [CPI DevTerm](https://www.clockworkpi.com/home-devterm) with the R01 RISC-V core.

You can find a statically-linked binary which should run out of the box on the R01 [here](../rootfs/bin/yaft). You will need to run it in rotated mode: `env YAFT=clockwise yaft`.

## Rationale

Even simple graphical environments are visibly sluggish on the R01. They're useful for some things, but most of the tools I use daily work in the terminal anyways, so why not abandon X11 for day to day use entirely?

The linux `fbcon` driver is fairly barebones, so I wanted a framebuffer terminal emulator to replace it with; `yaft` struck a sweet spot between low system requirements, doing most of what I need, and being easy to modify to support the rest.

## Differences from Upstream

This fork is based on [yaft master](https://github.com/uobikiemukot/yaft/commits/master/) with the [hackerb9 patch series](https://github.com/uobikiemukot/yaft/compare/master...hackerb9:yaft:master) applied. In addition, it has the following changes:

### New Features

- Bold text (`SGR 1`) will use a separate boldface font, rather than just lightening the foreground colour.
- Italic text (`SGR 3`) will use a separate italic (or bolditalic) font, if one is loaded; otherwise it will fake it by slanting the glyphs of the regular or bold font.
- Strikethrough text (`SGR 9`) is supported.
- Use of `ctrl`, `alt`, and `shift` in conjunction with the arrow keys, `pgup`, `pgdn`, `home`, `end`, `ins`, and `del` is now supported, and sends the same escape sequences as xterm.
- Glyphs in the Unicode private use area (e.g. Nerd Font) are now usable, as long as they are still within UCS2. A glyph width of 1 cell is assumed.
- Support for a wider range of programs using RGB colour:
	- ITU (`:`-separated) direct colour mode commands are supported.
	- The colourspace ID is optional, but permitted, in both ITU and legacy xterm modes.

### Configuration Changes

- The default font is now [Tamzen 10x20](https://github.com/sunaku/tamzen-font), which gives a 128x24 terminal size on the DevTerm.
	- Both regular and bold versions are included.
	- Nerd Font symbols, rescaled to fit in 10x20, are supported (except for the Material Design icons).
	- Dylex 10x20 is used as a fallback for symbols missing from Tamzen.
- The default cursor is now amber.

### Fixes & Optimizations

- Makefile now supports cross-compilation in Nix.
- On startup, it will benchmark different drawing modes (per-cell, per-line, per-screen); when refreshing, it will choose the most performant based on how much of the screen it needs to redraw.
	- This is mostly irrelevant in normal mode, but in rotated mode improves drawing performance 2-2.5×.
- `DECSET 1049` (alternate screen buffer) now properly saves/restores the old screen contents, clears the screen, and homes the cursor. This fixes some fullscreen programs like `atuin`.
