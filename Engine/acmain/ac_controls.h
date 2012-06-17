
void check_controls();
int my_readkey();

extern int restrict_until;

#define ALLEGRO_KEYBOARD_HANDLER
// KEYBOARD HANDLER
#if defined(LINUX_VERSION) || defined(MAC_VERSION)
extern int myerrno;
#else
extern int errno;
#define myerrno errno
#endif
