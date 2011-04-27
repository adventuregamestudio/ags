/*
  AGS Quest For Glory character file import-export module
  
  Contains routines to import QG1 character files, and export QG2 character
  files.
  
  NOTE: This file contains privileged information and should NOT be distributed.
  If the AGS source code is released, this file should NOT be released with it.
  
  This file (c) 2002 Chris Jones, QFG is (c) 1989-1990 Sierra On-line.
*/
#include <stdio.h>
#include <string.h>

extern void ccAddExternalSymbol(char*,void*);

#define SCORE_BIT 64
#define CHECK_DATA 10
#define EXTRA_DATA 18
#define OLD_NUM_ATTRIBS 25
// chartype: 0=fighter, 1=magicuser, 2=theif, 3=paladin
struct QFG1Stats {
  short key;
  short chartype;
  short gold;
  short goldlow;
  short score;
  short equipment;
  short strength;
  short intelligence;
  short agility;
  short vitality;
  short luck;
  short weaponuse;
  short parry;
  short dodge;
  short stealth;
  short picklocks;
  short throwing;
  short climbing;
  short magic;
  short experience;
  short health;   // these 3 (health, stamina, mana) seem to be adjustments
  short stamina;  // the real values are worked out
  short mana;
  short open;
  short detmagic;
  short trigger;
  short dazzle;
  short zap;
  short calm;
  short flamedart;
  short fetch;
  short daggers;
  short healingpotions;
  short manapotions;
  short staminapotions;
  short ghostoil;
  short comm;  // actually a checksum but we overwrite it
  short check86;
  short checksum1;
  short checksum2;
  char  name[50];
};
QFG1Stats ourstats;
char*qgpassword = "root/ade";

static int convByte (short ascii) {
  if (ascii == 32)
    return 0;
  if ((ascii >= 48) && (ascii <= 57))
    return ascii - 48;
  return ascii - 87;
}

static int convWord (short word) {
  return convByte (word >> 8) + (convByte (word & 0xff) * 16);
}

static int importQFGChar (char*filnam, char*usrpass) {
  if (strcmp(usrpass,qgpassword) != 0)
    return 0;

  FILE*inp = fopen(filnam, "rb");
  if (inp == NULL)
    return 0;

  char name[60],other[150];
  int i=0, thisone;
  while (1) {
    thisone = fgetc(inp);
    if (thisone == 10) {
      name[i] = 0;
      break;
    }
    name[i] = thisone;
    i++;
    if (i >= 50) {
      fclose(inp);
      return 0;
    }
  }
  i = 0;
  while (1) {
    thisone = fgetc(inp);
    if (thisone == 10) {
      other[i] = 0;
      break;
    }
    other[i] = thisone;
    if (i >= 100) {
      fclose(inp);
      return 0;
    }
    i++;
  }
  fclose(inp);

  short stats[OLD_NUM_ATTRIBS+EXTRA_DATA+1];
  stats[0] = 0x53;
  for (i = 0; i < OLD_NUM_ATTRIBS + EXTRA_DATA; i++) {
    stats[i+1] = convWord((other[i*2+1] << 8) | other[i*2] );
  }
  for (i = OLD_NUM_ATTRIBS + EXTRA_DATA ; i > 0; i --) {
    stats[i] ^= stats[i-1] & 127;
  }
  int check1 = 0xce;
  for (i = 0; i < OLD_NUM_ATTRIBS + CHECK_DATA; i+=2) {
    stats[i+1] = stats[i+1] & 127;
    check1 += stats[i+1];
  }
  int check2 = 0;
  for (i = 1; i < OLD_NUM_ATTRIBS + CHECK_DATA; i+=2) {
    stats[i+1] = stats[i+1] & 127;
    check2 += stats[i+1];
  }
  check1 &= 127;
  check2 &= 127;
  memcpy(&ourstats, &stats[0], (OLD_NUM_ATTRIBS + EXTRA_DATA) * sizeof(short));
  if ((check1 != ourstats.checksum1) || (check2 != ourstats.checksum2)) {
    return 0;
  }

  ourstats.comm = (ourstats.intelligence * 3 + ourstats.luck) / 4;
  ourstats.gold = ourstats.gold * 100 + ourstats.goldlow;
  if (ourstats.equipment & SCORE_BIT)
    ourstats.score += 256;
  strcpy(ourstats.name, name);
  return 1;
}

// ***** QG2 EXPORT SECTION ******
#define NUM_ATTRIBS 30
// Bits in svMiscEquip
#define	FINESWORD_BIT 	1  // fine sword
#define	FLAMESWORD_BIT	2  // flaming sword
#define	COMPASS_BIT		3  // compass
#define	PIN_BIT			  4  // spahire pin
#define	LAMP_BIT			8  // brass lamp
#define	TOKEN_BIT		0x10  // EOF token
#define	GLASSES_BIT	0x20  // X-Ray Glasses
// things transferred from "Quest for Glory 1"
#define	BABA_BIT		0x30  // Flag set from QG1
#define	SWORD_BIT		0x40  // Sword
#define	CHAIN_BIT		0x80  //  chainmail
#define	PICK_BIT		0x100  // lockpick
#define	TOOL_BIT		0x200  // thief's toolkit
struct QFG2Stats {
  short key;
  short chartype;
  short highdinar;   // (dinar / 100)
  short lowdinar;   // (dinar % 100)
  short score;
  short equipment;
  short strength;
  short intelligence;
  short agility;
  short vitality;
  short luck;
  short weaponuse;
  short parry;
  short dodge;
  short stealth;
  short picklocks;
  short throwing;
  short climbing;
  short magic;
  short comm;
  short honor;
  short experience;
  short health;
  short stamina;
  short mana;
  short open;
  short detmagic;
  short trigger;
  short dazzle;
  short zap;
  short calm;
  short flamedart;
  short fetch;
  short forcebolt;
  short levitate;
  short reversal;
  short daggers;
  short healingpills;
  short manapills;
  short staminapills;
  short poisoncurepills;
  short bogusa0;
  short bogus3e;
  short checksum1;
  short checksum2;
  short bogus2f;
  short bogus90;
  short bogus19;
  short bogusa3;
  short checksumkey;  // 0xda
  char  name[50];
};
static QFG2Stats qg2stats;

static int exportQFG2Char (char*filename, char*keycode) {
  if (strcmp(keycode, qgpassword) != 0)
    return 0;
    
  qg2stats.key = 0x53;
  qg2stats.bogusa0 = 0xa0;  
  qg2stats.bogus3e = 0x3e;
  qg2stats.bogus2f = 0x2f;
  qg2stats.bogus90 = 0x90;
  qg2stats.bogus19 = 0x19;
  qg2stats.bogusa3 = 0xa3;
  
  FILE*outt = fopen(filename, "wb");
  if (outt == NULL)
    return 0;
  
  fwrite(qg2stats.name, strlen(qg2stats.name), 1, outt);
  fputc(0x0a, outt);
  
  int i;
  short check1 = 0xda;
  short*rkptr = (short*)&qg2stats;
  
  for (i = 0; i < NUM_ATTRIBS + CHECK_DATA; i += 2) {
    rkptr[i+1] = rkptr[i+1] & 255;
    check1 += rkptr[i+1];
  }
  
  short check2 = 0;
  for (i = 1; i < NUM_ATTRIBS + CHECK_DATA; i += 2) {
    rkptr[i+1] = rkptr[i+1] & 255;
    check2 += rkptr[i+1];
  }
  
  check1 &= 255;
  check2 &= 255;
  
  qg2stats.checksum1 = check1;
  qg2stats.checksum2 = check2;
  
  for (i = 0; i < NUM_ATTRIBS + EXTRA_DATA; i++) {
    rkptr[i+1] = rkptr[i+1] & 255;
    rkptr[i+1] ^= rkptr[i];
  }
  
  for (i = 1; i <= NUM_ATTRIBS + EXTRA_DATA; i++)
    fprintf(outt, "%2x", rkptr[i]);
  
  fputc(0x0a, outt);
  fclose(outt);
    
  return 1;
}

void QGRegisterFunctions() {
  ccAddExternalSymbol("QGImport",importQFGChar);
  ccAddExternalSymbol("qgstats", &ourstats);
  ccAddExternalSymbol("QG2Export", exportQFG2Char);
  ccAddExternalSymbol("qg2stats", &qg2stats);
}

/*void main() {
  strcpy(qg2stats.name, "Fake Character");
  qg2stats.chartype = 2;  // thief
  qg2stats.highdinar=3;   // (dinar / 100)
  qg2stats.lowdinar=50;   // (dinar % 100)
  qg2stats.score = 222;
  qg2stats.equipment = 0;
  qg2stats.strength = 180;
  qg2stats.intelligence=179;
  qg2stats.agility=178;
  qg2stats.vitality=177;
  qg2stats.luck=176;
  qg2stats.weaponuse=175;
  qg2stats.parry=174;
  qg2stats.dodge=173;
  qg2stats.stealth=172;
  qg2stats.picklocks=171;
  qg2stats.throwing=170;
  qg2stats.climbing=169;
  qg2stats.magic =168;
  qg2stats.comm = 100;
  qg2stats.honor = 50;
  qg2stats.experience = 60;
  qg2stats.health = 12;
  qg2stats.stamina = 11;
  qg2stats.mana = 10;
  qg2stats.daggers = 5;
  exportQFG2Char("hacked.sav",qgpassword);
}
*/