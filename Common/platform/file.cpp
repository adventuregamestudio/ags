
#include "platform/file.h"

#if defined(LINUX_VERSION) || defined(MAC_VERSION)
#include <sys/stat.h>
long int filelength(int fhandle)
{
    struct stat statbuf;
    fstat(fhandle, &statbuf);
    return statbuf.st_size;
}
#endif

// Two functions to match those defined by bigend version by McCrea
#if !defined ALLEGRO_BIG_ENDIAN
short int   getshort(FILE * f)
{
    short i;
    fread(&i, sizeof(short), 1, f);
    return i; 
}

void        putshort(short int i, FILE * f)
{
    fwrite(&i, sizeof(short), 1, f);
}
#endif // !ALLEGRO_BIG_ENDIAN
