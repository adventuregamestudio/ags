#ifndef __CROOM_FUNC_ROOM_H
#define __CROOM_FUNC_ROOM_H

#define BLOCKTYPE_MAIN        1
#define BLOCKTYPE_SCRIPT      2
#define BLOCKTYPE_COMPSCRIPT  3
#define BLOCKTYPE_COMPSCRIPT2 4
#define BLOCKTYPE_OBJECTNAMES 5
#define BLOCKTYPE_ANIMBKGRND  6
#define BLOCKTYPE_COMPSCRIPT3 7     // new CSCOMP script instead of SeeR
#define BLOCKTYPE_PROPERTIES  8
#define BLOCKTYPE_OBJECTSCRIPTNAMES 9
#define BLOCKTYPE_EOF         0xff



extern int ff;
/*void roomstruct::allocall();
*/
//void roomstruct::freemessage();
/*void roomstruct::freeall();
}*/
/*void roomstruct::freeall();*/

struct room_file_header {
    short version PCKD;

#ifdef ALLEGRO_BIG_ENDIAN
    void ReadFromFile(FILE *fp)
    {
        version = __getshort__bigendian(fp);
    }
#endif
};

extern int _acroom_bpp;  // bytes per pixel of currently loading room

// returns bytes per pixel for bitmap's color depth
extern int bmp_bpp(BITMAP*bmpt);



void deserialize_interaction_scripts(FILE *iii, InteractionScripts *scripts);

extern void load_main_block(roomstruct *rstruc, char *files, FILE *opty, room_file_header rfh);
extern void load_room(char *files, roomstruct *rstruc, bool gameIsHighRes);

#endif // __CROOM_FUNC_ROOM_H