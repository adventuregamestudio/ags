
#include "ac_characterinfo.h"

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

#ifdef ALLEGRO_BIG_ENDIAN
void CharacterInfo::ReadFromFile(FILE *fp)
{
    defview = getw(fp);
    talkview = getw(fp);
    view = getw(fp);
    room = getw(fp);
    prevroom = getw(fp);
    x = getw(fp);
    y = getw(fp);
    wait = getw(fp);
    flags = getw(fp);
    following = __getshort__bigendian(fp);
    followinfo = __getshort__bigendian(fp);
    idleview = getw(fp);
    idletime = __getshort__bigendian(fp);
    idleleft = __getshort__bigendian(fp);
    transparency = __getshort__bigendian(fp);
    baseline = __getshort__bigendian(fp);
    activeinv = getw(fp);
    talkcolor = getw(fp);
    thinkview = getw(fp);
    blinkview = __getshort__bigendian(fp);
    blinkinterval = __getshort__bigendian(fp);
    blinktimer = __getshort__bigendian(fp);
    blinkframe = __getshort__bigendian(fp);
    walkspeed_y = __getshort__bigendian(fp);
    pic_yoffs = __getshort__bigendian(fp);
    z = getw(fp);
    reserved[0] = getw(fp);
    reserved[1] = getw(fp);
    blocking_width = __getshort__bigendian(fp);
    blocking_height = __getshort__bigendian(fp);;
    index_id = getw(fp);
    pic_xoffs = __getshort__bigendian(fp);
    walkwaitcounter = __getshort__bigendian(fp);
    loop = __getshort__bigendian(fp);
    frame = __getshort__bigendian(fp);
    walking = __getshort__bigendian(fp);
    animating = __getshort__bigendian(fp);
    walkspeed = __getshort__bigendian(fp);
    animspeed = __getshort__bigendian(fp);
    fread(inv, sizeof(short), MAX_INV, fp);
    actx = __getshort__bigendian(fp);
    acty = __getshort__bigendian(fp);
    fread(name, sizeof(char), 40, fp);
    fread(scrname, sizeof(char), MAX_SCRIPT_NAME_LEN, fp);
    on = getc(fp);
    // MAX_INV is odd, so need to sweep up padding
    // skip over padding that makes struct a multiple of 4 bytes long
    fseek(fp, 4 - (((MAX_INV+2)*sizeof(short)+40+MAX_SCRIPT_NAME_LEN+1)%4), SEEK_CUR);
}
#endif
