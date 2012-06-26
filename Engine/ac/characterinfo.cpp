
#include <stdio.h>
#include <string.h>
#include "ac/characterinfo.h"
#include "bigend.h"

int CharacterInfo::get_effective_y() {
    return y - z;
}
int CharacterInfo::get_baseline() {
    if (baseline < 1)
        return y;
    return baseline;
}
int CharacterInfo::get_blocking_top() {
    if (blocking_height > 0)
        return y - blocking_height / 2;
    return y - 2;
}
int CharacterInfo::get_blocking_bottom() {
    // the blocking_bottom should be 1 less than the top + height
    // since the code does <= checks on it rather than < checks
    if (blocking_height > 0)
        return (y + (blocking_height + 1) / 2) - 1;
    return y + 3;
}

void CharacterInfo::ReadFromFile(FILE *fp)
{
    int reserved[2];
//#ifdef ALLEGRO_BIG_ENDIAN
    defview = getw(fp);
    talkview = getw(fp);
    view = getw(fp);
    room = getw(fp);
    prevroom = getw(fp);
    x = getw(fp);
    y = getw(fp);
    wait = getw(fp);
    flags = getw(fp);
    following = getshort(fp);//__getshort__bigendian(fp);
    followinfo = getshort(fp);//__getshort__bigendian(fp);
    idleview = getw(fp);
    idletime = getshort(fp);//__getshort__bigendian(fp);
    idleleft = getshort(fp);//__getshort__bigendian(fp);
    transparency = getshort(fp);//__getshort__bigendian(fp);
    baseline = getshort(fp);//__getshort__bigendian(fp);
    activeinv = getw(fp);
    talkcolor = getw(fp);
    thinkview = getw(fp);
    blinkview = getshort(fp);//__getshort__bigendian(fp);
    blinkinterval = getshort(fp);//__getshort__bigendian(fp);
    blinktimer = getshort(fp);//__getshort__bigendian(fp);
    blinkframe = getshort(fp);//__getshort__bigendian(fp);
    walkspeed_y = getshort(fp);//__getshort__bigendian(fp);
    pic_yoffs = getshort(fp);//__getshort__bigendian(fp);
    z = getw(fp);
    reserved[0] = getw(fp);
    reserved[1] = getw(fp);
    blocking_width = getshort(fp);//__getshort__bigendian(fp);
    blocking_height = getshort(fp);//__getshort__bigendian(fp);;
    index_id = getw(fp);
    pic_xoffs = getshort(fp);//__getshort__bigendian(fp);
    walkwaitcounter = getshort(fp);//__getshort__bigendian(fp);
    loop = getshort(fp);//__getshort__bigendian(fp);
    frame = getshort(fp);//__getshort__bigendian(fp);
    walking = getshort(fp);//__getshort__bigendian(fp);
    animating = getshort(fp);//__getshort__bigendian(fp);
    walkspeed = getshort(fp);//__getshort__bigendian(fp);
    animspeed = getshort(fp);//__getshort__bigendian(fp);
    fread(inv, sizeof(short), MAX_INV, fp);
    actx = getshort(fp);//__getshort__bigendian(fp);
    acty = getshort(fp);//__getshort__bigendian(fp);
    fread(name, sizeof(char), 40, fp);
    fread(scrname, sizeof(char), MAX_SCRIPT_NAME_LEN, fp);
    on = getc(fp);
    // MAX_INV is odd, so need to sweep up padding
    // skip over padding that makes struct a multiple of 4 bytes long
    fseek(fp, 4 - (((MAX_INV+2)*sizeof(short)+40+MAX_SCRIPT_NAME_LEN+1)%4), SEEK_CUR);
//#else
    //throw "CharacterInfo::ReadFromFile() is not implemented for little-endian platforms and should not be called.";
//#endif
}

void ConvertOldCharacterToNew (OldCharacterInfo *oci, CharacterInfo *ci) {
  COPY_CHAR_VAR (defview);
  COPY_CHAR_VAR (talkview);
  COPY_CHAR_VAR (view);
  COPY_CHAR_VAR (room);
  COPY_CHAR_VAR (prevroom);
  COPY_CHAR_VAR (x);
  COPY_CHAR_VAR (y);
  COPY_CHAR_VAR (wait);
  COPY_CHAR_VAR (flags);
  COPY_CHAR_VAR (following);
  COPY_CHAR_VAR (followinfo);
  COPY_CHAR_VAR (idleview);
  COPY_CHAR_VAR (idletime);
  COPY_CHAR_VAR (idleleft);
  COPY_CHAR_VAR (transparency);
  COPY_CHAR_VAR (baseline);
  COPY_CHAR_VAR (activeinv);
  COPY_CHAR_VAR (loop);
  COPY_CHAR_VAR (frame);
  COPY_CHAR_VAR (walking);
  COPY_CHAR_VAR (animating);
  COPY_CHAR_VAR (walkspeed);
  COPY_CHAR_VAR (animspeed);
  COPY_CHAR_VAR (actx);
  COPY_CHAR_VAR (acty);
  COPY_CHAR_VAR (on);
  strcpy (ci->name, oci->name);
  strcpy (ci->scrname, oci->scrname);
  memcpy (&ci->inv[0], &oci->inv[0], sizeof(short) * 100);
  // move the talking colour into the struct and remove from flags
  ci->talkcolor = (oci->flags & OCHF_SPEECHCOL) >> OCHF_SPEECHCOLSHIFT;
  ci->flags = ci->flags & (~OCHF_SPEECHCOL);
}
