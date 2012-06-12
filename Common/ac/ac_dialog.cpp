
#include "ac_dialog.h"

#ifdef ALLEGRO_BIG_ENDIAN
void DialogTopic::ReadFromFile(FILE *fp)
{
    fread(optionnames, 150*sizeof(char), MAXTOPICOPTIONS, fp);
    fread(optionflags, sizeof(int), MAXTOPICOPTIONS, fp);
    optionscripts = (unsigned char *) getw(fp);
    fread(entrypoints, sizeof(short), MAXTOPICOPTIONS, fp);
    startupentrypoint = __getshort__bigendian(fp);
    codesize = __getshort__bigendian(fp);
    numoptions = getw(fp);
    topicFlags = getw(fp);
}
#endif