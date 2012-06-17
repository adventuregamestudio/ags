#ifndef __AC_DYNAMICSPRITE_H
#define __AC_DYNAMICSPRITE_H

void add_dynamic_sprite (int gotSlot, block redin, bool hasAlpha = false);
void free_dynamic_sprite (int gotSlot);

extern char check_dynamic_sprites_at_exit;

#endif // __AC_DYNAMICSPRITE_H