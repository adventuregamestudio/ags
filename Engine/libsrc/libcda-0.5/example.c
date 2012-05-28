/* 
 * Example program for libcda.
 *
 * The contents have been coded to be self-explanatory.
 * If they are not, please email me.
 *
 * Peter Wang <tjaden@users.sf.net>
 */

#include <stdio.h>
#include "libcda.h"


void show_cmds(void)
{
    printf("\nAvailable commands:\n");
    printf("? - help (this)\n");
    printf("p - play track\n");
    printf("r - play range of tracks\n");
    printf("f - play from track to end of disc\n");
    printf("P - show currently playing track\n");
    printf("w - pause (wait)\n");
    printf("W - resume\n");
    printf("S - show paused status\n");
    printf("s - stop\n");
    printf("i - show track info\n");
    printf("a - check track is audio\n");
    printf("v - set volume\n");
    printf("V - get volume\n");
    printf("e - eject drive\n");
    printf("c - close drive\n");
    printf("q - quit\n\n");
}


int input_int(char *prompt)
{
    int a;
    printf("%s: ", prompt);
    fflush(stdout);
    scanf("%d", &a);
    return a;
}


int main()
{
    char cmd[20];
    int done = 0;
    int ret, trk, a, b;

    if (cd_init() < 0) {
	printf("Error initialising libcda (%s)\n", cd_error);
	return 1;
    }
	
    show_cmds();

    do {  
	printf(">>> ");
	fflush(stdout);
	scanf("%s", cmd);

	switch (cmd[0]) {
	    case '?':
		show_cmds();
		break;

	    case 'p':
		trk = input_int("Track");
		ret = cd_play(trk);
		if (ret != 0)
		    printf("Error occurred (%s)\n", cd_error);
		break;

	    case 'r':
		a = input_int("First track");
		b = input_int("Last track");
		ret = cd_play_range(a, b);
		if (ret != 0)
		    printf("Error occurred (%s)\n", cd_error);
		break;

	    case 'f':
		trk = input_int("Start track");
		ret = cd_play_from(trk);
		if (ret != 0)
		    printf("Error occurred (%s)\n", cd_error);
		break;

	    case 'P':
		trk = cd_current_track();
		if (trk)
		    printf("Playing track %d\n", trk);
		else
		    printf("Not playing\n");
		break;

	    case 'w':
		cd_pause();
		break;

	    case 'W':
		cd_resume();
		break;

	    case 'S':
		ret = cd_is_paused();
		if (ret)
		    printf("Paused\n");
		else
		    printf("Not paused\n");
		break;

	    case 's':
		cd_stop();
		break;

	    case 'i':
		ret = cd_get_tracks(&a, &b);
		if (ret != 0)
		    printf("Error occurred (%s)\n", cd_error);
		else
		    printf("First track: %d\nLast track: %d\n", a, b);
		break;

	    case 'a':
		trk = input_int("Track");
		ret = cd_is_audio(trk);
		if (ret < 0)
		    printf("Error occurred (%s)\n", cd_error);
		else
		    printf("Track %d is %s\n", trk, (ret ? "audio" : "data"));
		break;

	    case 'v':
		a = input_int("Left (0-255)");
		b = input_int("Right (0-255)");
		cd_set_volume(a, b);
		break;

	    case 'V':
		cd_get_volume(&a, &b);
		printf("Left volume: %d\nRight volume: %d\n", a, b);
		break;

	    case 'e':
		cd_eject();
		break;

	    case 'c':
		cd_close();
		break;

	    case 'q':
		done = 1;
		break;

	    default:
		printf("Unrecognised command: `%c'\n", cmd[0]);
	}
    } while (!done);

    cd_exit();
    
    return 0;
}
