#ifndef UNIX_GLKTERM_H
#define UNIX_GLKTERM_H

void save_game(void);
void load_save_game(void);
void grab_input(void);
void print_syntax(void);
void do_switches(int argc, char *argv[]);
glui32 get_keypress(void);

extern winid_t mainwin;
extern winid_t statuswin;

#endif
