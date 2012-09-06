/* MOUSELIB.CPP

Library of mouse functions for graphics and text mode

(c) 1994 Chris Jones

.    PLEASE NOTE: Running this program in the TC++ editor will NOT show
.    a mouse cursor in text mode. You must compile it and then exit from
.    the editor for the program to work properly.

.    ADDITIONAL NOTE: These functions work only when compiled to a OBJ
.    file and linked with another program (#include the CMOUSE.H file).
.    Just add MOUSELIB.CPP to the project.
*/
#ifndef __GNUC__
#error This is the 32-bit version
#endif
#define far
#include <dos.h>
#include <stdio.h>
//#include </tc/wgt/include/wgt4.h>
#define WGT2ALLEGRO_NOFUNCTIONS
#include "wgt2allg.h"
#include <conio.h>
#include <process.h>

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define MAXCURSORS 20
/*
int  minstalled();    // this returns number of buttons (or 0)
void minst();   // this exits if not installed
void mshow();
void mhide();
int  mgetbutton();
void mchangestyle(int,int);
void mgetpos();
void mgetgraphpos();
void msetpos(int,int);
void msetgraphpos(int,int);
void mconfine(int,int,int,int); // left top right bottom
void mgraphconfine(int,int,int,int);
int  mbutrelease(int);
void domouse(int=0);   // graphics mode cursor
void mfreemem();
void mloadcursor(char*);  // load from file
void mloadwcursor(char*);
void mnewcursor(char);
int  ismouseinbox(int,int,int,int);
void msethotspot(int,int);   // Graphics mode only. Useful for crosshair.
*/

extern long cliboffset(char*);
extern char lib_file_name[13];

struct REGPACK {
  unsigned short r_bx;
  unsigned short r_dx;
  unsigned short r_cx;
  unsigned short r_ax;
  };

/*#include <dpmi.h>
__dpmi_regs dprt;*/
REGS ure;
void intr(int numm,REGPACK*rrr) {
  ure.w.ax=rrr->r_ax;
  ure.w.bx=rrr->r_bx;
  ure.w.cx=rrr->r_cx;
  ure.w.dx=rrr->r_dx;
  int86(numm,&ure,&ure);
  rrr->r_ax=ure.w.ax;
  rrr->r_bx=ure.w.bx;
  rrr->r_cx=ure.w.cx;
  rrr->r_dx=ure.w.dx;
/*  memcpy(&dprt,&rrr,sizeof(__dpmi_regs));
  __dpmi_int(numm,&dprt);*/
  }

char *mouselibcopyr="MouseLib32 (c) 1994, 1998 Chris Jones";
struct REGPACK t;
const int NONE=-1,LEFT=0,RIGHT=1,MIDDLE=2;
const int BLUE_=0x7100,GREEN_=0x7200,GR_BL_=0x7300,RED_=0x7400,
  MAGENTA_=0x7500,BROWN_=0x7600,L_GREY_=0x7700,D_GREY_=0x7800,
  L_BLUE_=0x7900,L_GREEN_=0x7a00,L_GR_BL_=0x7b00,L_RED_=0x7c00,
  L_MAGENTA_=0x7d00, YELLOW_=0x7e00,WHITE_=0x7f00;
int aa;
char mouseturnedon=FALSE,currentcursor=0;
int mousex=0,mousey=0,numcurso=-1,hotx=0,hoty=0;
int boundx1=0,boundx2=99999,boundy1=0,boundy2=99999;
int disable_mgetgraphpos = 0;
char ignore_bounds = 0;
IBitmap *savebk,mousecurs[MAXCURSORS];
extern int vesa_xres,vesa_yres;
//IBitmap *ignore_mouseoff_bitmap = NULL;

REGS urr;
void mgetgraphpos() {

  if (disable_mgetgraphpos)
    return;

//  int bk=mousex;
/*  t.r_ax=0x0003;      // returns in pixels
  intr(0x33,&t);
  mousex=(int)t.r_cx; // for a 320 x 200 display
  mousey=(int)t.r_dx;*/
  urr.x.ax=3;
  int86(0x33,&urr,&urr);
  mousex=urr.x.cx;
  mousey=urr.x.dx;

/*  if (bk-mousex==1) { mousex/=2; mousex--; }
  else if (mousex-bk==1) { mousex/=2; mousex++; }
  else*/
  if (vesa_xres==320) mousex/=2;
  if (vesa_xres==960) mousex=(mousex*3)/2;
  if (vesa_yres==240) mousey=(mousey*12)/10;
  if (vesa_yres==480) mousey=(mousey*24)/10;
  if (vesa_yres==400) mousey*=2;
  if (vesa_yres==600) mousey*=3;
//  mousex+=hotx; mousey+=hoty;
  if (mousex>=vesa_xres) mousex=vesa_xres-1;

  if (ignore_bounds)
    return;

  if (mousex < boundx1) mousex = boundx1;
  if (mousey < boundy1) mousey = boundy1;
  if (mousex >= boundx2) mousex = boundx2 - 1;
  if (mousey >= boundy2) mousey = boundy2 - 1;
  }

void msetcursorlimit (int x1, int y1, int x2, int y2) {
  // like graphconfine, but don't actually pass it to the driver
  // - stops the Windows cursor showing when out of the area
  boundx1 = x1;
  boundy1 = y1;
  boundx2 = x2;
  boundy2 = y2;
}

int hotxwas=0,hotywas=0;
void domouse(int str) {
/* TO USE THIS ROUTINE YOU MUST LOAD A MOUSE CURSOR USING mloadcursor.
.
.  YOU MUST ALSO REMEMBER TO CALL mfreemem AT THE END OF THE PROGRAM. */
/*  short *sstr=(short*)&mousecurs[currentcursor][0];
  int poow=(short)sstr[0],pooh=(short)sstr[1];*/
  int poow=wgetblockwidth(mousecurs[currentcursor]);
  int pooh=wgetblockheight(mousecurs[currentcursor]);
  int smx=mousex-hotxwas,smy=mousey-hotywas;
//  mousex-=hotx; mousey-=hoty;
  mgetgraphpos();
  mousex-=hotx; mousey-=hoty;
  if (mousex+poow>=vesa_xres) poow=vesa_xres-mousex;
  if (mousey+pooh>=vesa_yres) pooh=vesa_yres-mousey;
  wclip(0,0,vesa_xres-1,vesa_yres-1);
  if ((str==0) & (mouseturnedon==TRUE)) {
    if ((mousex!=smx) | (mousey!=smy)) { // the mouse has moved
      wputblock(smx,smy,savebk,0); wfreeblock(savebk);
      savebk=wnewblock(mousex,mousey,mousex+poow,mousey+pooh);
      wputblock(mousex,mousey,mousecurs[currentcursor],1);
      }
    }
  else if ((str==1) & (mouseturnedon==FALSE)) {
    // the mouse is just being turned on
    savebk=wnewblock(mousex,mousey,mousex+poow,mousey+pooh);
    wputblock(mousex,mousey,mousecurs[currentcursor],1);  mouseturnedon=TRUE;
    }
  else if ((str==2) & (mouseturnedon==TRUE)) { // the mouse is being turned off
//    if (abuf != ignore_mouseoff_bitmap)
    wputblock(smx,smy,savebk,0);
    wfreeblock(savebk);
    mouseturnedon=FALSE;
    }
  mousex+=hotx; mousey+=hoty;
  hotxwas=hotx; hotywas=hoty;
  }

int ismouseinbox(int lf,int tp,int rt,int bt) {
  if ((mousex>=lf) & (mousex<=rt) & (mousey>=tp) & (mousey<=bt)) return TRUE;
  else return FALSE;
  }

void mfreemem() {
  for (int re=0;re<numcurso;re++) {
    if (mousecurs[re]!=NULL) wfreeblock(mousecurs[re]);
    }
  }

void mnewcursor(char cursno) {
  domouse(2); currentcursor=cursno; domouse(1);
  }

/*void mloadcursor(char *cursf) { int f,b;
  FILE*ou=fopen(cursf,"rb");
  if (ferror(ou)) { textmode(C80);  printf("Mouse Cursor File Not Found\n");
    exit(1); }
  numcurso=fgetc(ou)-35;
  for (f=0;f<783;f++) fgetc(ou);  // palette & stuff
  if (numcurso>MAXCURSORS) numcurso=MAXCURSORS;


  int hhht,wwwd;
  for (int za=0;za<numcurso;za++) {
    wsetcolor(0); abuf->FillRect(CRect(0,0,16,16);
    fgetc(ou); fgetc(ou); fgetc(ou);
    hhht=fgetc(ou)-35; fgetc(ou);
    wwwd=fgetc(ou)-35; fgetc(ou);
    for (b=0;b<hhht;b++) {
      for (f=0;f<wwwd;f++) { wsetcolor(fgetc(ou)-35);
	abuf->PutPixel(f,b);  }
      }
    mousecurs[za]=wnewblock(0,0,wwwd-1,hhht-1);
    }
  if (ferror(ou)) {
    textmode(C80);  printf("I/O error.");    exit(2);
    }
  fclose(ou);
  }*/

void mloadwcursor(char*namm) { color dummypal[256];
  if (wloadsprites(&dummypal[0],namm,mousecurs,0,MAXCURSORS)) {
    printf("C_Load_wCursor: Error reading mouse cursor file\n"); exit(1); }
/*  color dummypal[256]; int f; FILE*ou;
  if (cliboffset(namm)>0) ou=fopen(lib_file_name,"rb");
  else ou=fopen(namm,"rb");
  fseek(ou,((cliboffset(namm)>0) ? cliboffset(namm) : 0),SEEK_SET);
  if (ferror(ou)) { textmode(C80);  printf("Mouse Cursor File Not Found\n");
    exit(1); }
  int vers=fgetc(ou);
  for (f=0;f<782;f++) fgetc(ou);
  if (vers>=4) numcurso=getw(ou)+1;
  else { textmode(C80); printf("Version 4.0 or later sprite file required.\n"); exit(3); }
  if (ferror(ou)) {  textmode(C80);  printf("I/O error.");    exit(2); }
  if (numcurso>MAXCURSORS) numcurso=MAXCURSORS;

  int hhht,wwwd,b;
  for (int za=0;za<numcurso;za++) {
    if (getw(ou)==0) { continue; }
    wwwd=getw(ou); hhht=getw(ou);
    wsetcolor(0); abuf->FillRect(CRect(0,0,wwwd,hhht);
    for (b=0;b<hhht;b++) {
      for (f=0;f<wwwd;f++) { wsetcolor(fgetc(ou));
	abuf->PutPixel(f,b);  }
      }
    mousecurs[za]=wnewblock(0,0,wwwd-1,hhht-1);
    }
//  wloadsprites(dummypal,namm,mousecurs,0,numcurso);*/
  }

int mgetbutton() {
  t.r_ax=0x0005;
  t.r_bx=0x0000;
  intr(0x33,&t);
  if (t.r_bx<=0) {
    t.r_ax=0x0005;
    t.r_bx=0x0001;
    intr(0x33,&t);
    if (t.r_bx>0) {
      aa=RIGHT; goto retn; }
    aa=NONE;
    goto retn;
    }
  t.r_ax=0x0005;
  t.r_bx=0x0001;
  intr(0x33,&t);
  if (t.r_bx<=0) {
    aa=LEFT; goto retn; }
  aa=MIDDLE;
retn:
  return aa;
  }

int mbutrelease(int buno) {
  aa=FALSE;
  t.r_ax=0x0006;
  t.r_bx=buno;
  intr(0x33,&t);
  if (t.r_bx>0) aa=TRUE;
  else aa=FALSE;
  return aa;
  }

const int MB_ARRAY[3]={1,2,4};
int misbuttondown(int buno) {
  int tmpvr;
  t.r_ax=3;
  intr(0x33,&t);
  tmpvr=t.r_bx;
  if (tmpvr & MB_ARRAY[buno]) return TRUE;
  return FALSE;
  }
int mgetbuttonmask() {
  t.r_ax=3;
  intr(0x33,&t);
  return t.r_bx;
  }

void mconfine(int x1,int y1,int x2,int y2) {
  t.r_ax=0x0008;
  t.r_cx=(y1-1)*8;
  t.r_dx=(y2-1)*8;
  intr(0x33,&t);
  t.r_ax=0x0007;
  t.r_cx=(x1-1)*8;
  t.r_dx=(x2-1)*8;
  intr(0x33,&t);
  }

void mgraphconfine(int x1,int y1,int x2,int y2) {
  t.r_ax=0x0008;  // vertical restriction
  t.r_cx=y1;
  t.r_dx=y2;
  intr(0x33,&t);
  t.r_ax=0x0007;  // horizontal restriction
  t.r_cx=x1*2;    // for 320 x 200
  t.r_dx=x2*2;
  intr(0x33,&t);
  }

void mgetpos() {
  t.r_ax=0x0003;
  intr(0x33,&t);     // returns in 1-80, 1-25
  mousex=t.r_cx/8+1;
  mousey=t.r_dx/8+1;
  }

void msetpos(int xa,int ya) {
  t.r_ax=0x0004;
  t.r_cx=(xa-1)*8;
  t.r_dx=(ya-1)*8;
  intr(0x33,&t);
  }

void msetgraphpos(int xa,int ya) { //xa-=hotx; ya-=hoty;
  if ((xa<vesa_xres) & (ya<vesa_yres) & (ya>0) & (xa>0)) {
    t.r_ax=0x0004;
    t.r_cx=xa*2;
    t.r_dx=ya;
    if (vesa_xres==640) t.r_cx=xa;
    if (vesa_xres==960) t.r_cx=(xa/3)*2;
    if (vesa_yres==240) t.r_dx=(ya*10)/12;
    if (vesa_yres==400) t.r_dx/=2;
    if (vesa_yres==600) t.r_dx/=3;
    intr(0x33,&t);
    }
  }

void mchangestyle(int nsty,int colo=0x7700) {
  t.r_ax=0x0a;            // nsty is ASCII value of character to change to
  t.r_bx=0;               // colo is 7_00, where _ is color no in hex.
  t.r_cx=colo;
  t.r_dx=nsty;
  intr(0x33,&t);
  }

void mshow() {
  t.r_ax=0x0001;    // show mouse cursor
  intr(0x33,&t);
  }

void mhide() {
  t.r_ax=0x0002;  // Hide the mouse cursor
  intr(0x33,&t);
  }

void msethotspot(int xx,int yy) { //mousex-=hotx; mousey-=hoty;
  hotx=xx; hoty=yy; //mousex+=hotx; mousey+=hoty;
  }

int minstalled() {
  t.r_ax=0x0000;
  intr(0x33,&t);
  if (t.r_ax==0) return 0;
  return t.r_bx;  // num buttons
  }

void minst() {
  t.r_ax=0x0000;
  intr(0x33,&t);
  if (t.r_ax==0) {
    cprintf("Mouse is not installed.");
    exit(0);
    }
  }
