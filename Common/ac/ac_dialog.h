#ifndef __AC_DIALOG_H
#define __AC_DIALOG_H



#define MAXTOPICOPTIONS     30
#define DFLG_ON             1  // currently enabled
#define DFLG_OFFPERM        2  // off forever (can't be trurned on)
#define DFLG_NOREPEAT       4  // character doesn't repeat it when clicked
#define DFLG_HASBEENCHOSEN  8  // dialog option is 'read'
#define DTFLG_SHOWPARSER    1  // show parser in this topic
#define DCMD_SAY            1
#define DCMD_OPTOFF         2
#define DCMD_OPTON          3
#define DCMD_RETURN         4
#define DCMD_STOPDIALOG     5
#define DCMD_OPTOFFFOREVER  6
#define DCMD_RUNTEXTSCRIPT  7
#define DCMD_GOTODIALOG     8
#define DCMD_PLAYSOUND      9
#define DCMD_ADDINV         10
#define DCMD_SETSPCHVIEW    11
#define DCMD_NEWROOM        12
#define DCMD_SETGLOBALINT   13
#define DCMD_GIVESCORE      14
#define DCMD_GOTOPREVIOUS   15
#define DCMD_LOSEINV        16
#define DCMD_ENDSCRIPT      0xff
#define DCHAR_NARRATOR  999
#define DCHAR_PLAYER    998
#define MAX_DIALOG          500
struct DialogTopic {
    char          optionnames[MAXTOPICOPTIONS][150];
    int           optionflags[MAXTOPICOPTIONS];
    unsigned char *optionscripts;
    short         entrypoints[MAXTOPICOPTIONS];
    short         startupentrypoint;
    short         codesize;
    int           numoptions;
    int           topicFlags;

#ifdef ALLEGRO_BIG_ENDIAN
    void ReadFromFile(FILE *fp);
#endif
};


#endif // __AC_DIALOG_H