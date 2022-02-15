# scottfree
This is an experimental fork of the [Glk port of ScottFree](https://github.com/cspiegel/scottfree-glk) that adds support for the TI-99/4A Scott Adams game format.

The original Scott Adams games seem to be working now, but more testing is needed.

This is a cut-down version of the ScottFree interpreter fork used by [Spatterlight](https://github.com/angstsmurf/spatterlight), with the Spatterlight Glk code replaced by the [GlkTerm](https://github.com/erkyrath/glkterm) library, all support ZX Spectrum and C-64 versions and graphics removed. Fragments of the [Bunyon](https://github.com/thomamas/build_bunyon) interpreter are also present here and there in the code. I’m afraid it is not in a very clean or readable state.

This should build out of the box, although I have only tried it on my MacBook. GlkTerm source is included (requires curses), along with a makefile and an Xcode project. 
