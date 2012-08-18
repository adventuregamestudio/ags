
#ifdef UNUSED_CODE

void dump_code(FILE*dto,ccScript*cctemp) {
  fprintf(dto,"script data size: %d\n",cctemp->globaldatasize);
  fprintf(dto,"string area size: %d\n",cctemp->stringssize);
//  fprintf(dto,"SP (should be zero): %d\n",cctemp->cur_sp);
  fprintf(dto,"code size: %d\n",cctemp->codesize);
  fprintf(dto,"SCRIPT VIRTUAL-CODE FOLLOWS:\n");
  // dump code in readable form
  int t;
  for (t=0;t<cctemp->codesize;t++) {
    int l,thisop=cctemp->code[t],isreg=0;
    char*toprint=sccmdnames[thisop];
    if (toprint[0]=='$') {
      isreg=1;
      toprint++;
      }
    if (toprint[0]=='$') {
      isreg|=2;
      toprint++;
      }
    fprintf(dto,"%s",toprint);
    for (l=0;l<sccmdargs[thisop];l++) {
      t++;
      if (l>0) fprintf(dto,",");
      if ((l==0) && (isreg & 1))
        fprintf(dto," %s",regnames[cctemp->code[t]]);
      else if ((l==1) && (isreg & 2))
        fprintf(dto," %s",regnames[cctemp->code[t]]);
      else fprintf(dto," %d",cctemp->code[t]);
      }
    fprintf(dto,"\n");
    }

  }








#define MAX_NESTED_IFDEFS 10
char nested_if_include[MAX_NESTED_IFDEFS];
int nested_if_level = 0;

int deletingCurrentLine() {
  if ((nested_if_level > 0) && (nested_if_include[nested_if_level - 1] == 0))
    return 1;

  return 0;
}



void get_next_word(char*stin,char*wo, bool includeDots = false) {
  if (stin[0] == '\"') {
    // a string
    int breakout = 0;
//    char*oriwo=wo;
    wo[0] = stin[0];
    wo++; stin++;
    while (!breakout) {
      if ((stin[0] == '\"') && (stin[-1] != '\\')) breakout=1;
      if (stin[0] == 0) breakout = 1;
      wo[0] = stin[0];
      wo++; stin++;
      }
    wo[0] = 0;
//    printf("Word: '%s'\n",oriwo);
    return;
    }
  else if (stin[0] == '\'') {
    // a character constant
    int breakout = 0;
    wo[0] = stin[0];
    wo++; stin++;
    while (!breakout) {
      if ((stin[0] == '\'') && (stin[-1] != '\\')) breakout=1;
      if (stin[0] == 0) breakout = 1;
      wo[0] = stin[0];
      wo++; stin++;
      }
    wo[0] = 0;
    return;
  }

  while ((is_alphanum(stin[0])) || ((includeDots) && (stin[0] == '.'))) {
    wo[0] = stin[0];
    wo++;
    stin++;
  }

  wo[0]=0;
  return;
}

void pre_process_directive(char*dirpt,FMEM*outfl) {
  char*shal=dirpt;
  // seek to the end of the first word (eg. #define)
  while ((!is_whitespace(shal[0])) && (shal[0]!=0)) shal++;
/*
  if (shal[0]==0) {
    cc_error("unknown preprocessor directive");
    return;
  }
  */
  // extract the directive name
  int shalwas = shal[0];
  shal[0] = 0;
  char dname[150];
  strcpy(dname,&dirpt[1]);
  strlwr(dname);
  shal[0] = shalwas;
  // skip to the next word on the line
  skip_whitespace(&shal);

  // write a blank line to the output so that line numbers are correct
  fmem_puts("",outfl);

  if ((stricmp(dname, "ifdef") == 0) ||
      (stricmp(dname, "ifndef") == 0) ||
      (stricmp(dname, "ifver") == 0) ||
      (stricmp(dname, "ifnver") == 0)) {

    if (nested_if_level >= MAX_NESTED_IFDEFS) {
      cc_error("too many nested #ifdefs");
      return;
    }
    int isVersionCheck = 0;
    int wantDefined = 1;

    if (dname[2] == 'n')
      wantDefined = 0;

    if (strstr(dname, "ver") != NULL)
      isVersionCheck = 1;

    char macroToCheck[100];
    get_next_word(shal, macroToCheck, true);

    if (macroToCheck[0] == 0) {
      cc_error("Expected token after space");
      return;
    }

    if ((isVersionCheck) && (!is_digit(macroToCheck[0]))) {
      cc_error("Expected version number");
      return;
    }

    if ((nested_if_level > 0) && (nested_if_include[nested_if_level - 1] == 0)) {
      // if nested inside a False one, then this one must be false
      // as well
      nested_if_include[nested_if_level] = 0;
    }
    else if ((isVersionCheck) && (strcmp(ccSoftwareVersion, macroToCheck) >= 0)) {
      nested_if_include[nested_if_level] = wantDefined;
    }
    else if ((!isVersionCheck) && (macros.find_name(macroToCheck) >= 0)) {
      nested_if_include[nested_if_level] = wantDefined;
    }
    else {
      nested_if_include[nested_if_level] = !wantDefined;
    }
    nested_if_level++;
  }
  else if (stricmp(dname, "endif") == 0) {
    if (nested_if_level < 1) {
      cc_error("#endif without #if");
      return;
    }
    nested_if_level--;
  }
  else if (deletingCurrentLine()) {
    // inside a non-defined #ifdef block, don't process anything
  }
  else if (stricmp(dname, "error") == 0) {
    cc_error("User error: %s", shal);
    return;
  }
  else if (stricmp(dname,"define")==0) {
    char nambit[100];
    int nin=0;
    while ((!is_whitespace(shal[0])) && (shal[0]!=0)) {
      nambit[nin]=shal[0];
      nin++;
      shal++;
    }
    nambit[nin]=0;
    skip_whitespace(&shal);

    macros.add(nambit,shal);
  }
  else if (stricmp(dname, "undef") == 0) {
    char macroToCheck[100];
    get_next_word(shal, macroToCheck);

    if (macroToCheck[0] == 0) {
      cc_error("Expected: macro");
      return;
    }

    int idx = macros.find_name(macroToCheck);
    if (idx < 0) {
      cc_error("'%s' not defined", macroToCheck);
      return;
    }

    macros.remove(idx);
  }
  else if ((stricmp(dname, "sectionstart") == 0) ||
           (stricmp(dname, "sectionend") == 0))
  {
    // do nothing - markers for the editor, just remove them
  }
/*  else if (stricmp(dname,"include")==0) {
    if ((shal[0]=='\"') | (shal[0]=='<')) shal++;
    char inclnam[100];
    int incp=0;
    while ((is_alphanum(shal[0])!=0) | (shal[0]=='.')) {
      inclnam[incp]=shal[0]; shal++; incp++; }
    inclnam[incp]=0;
    // TODO:  Should actually include the file/header INCLNAM
    printf("unimplemented #include of '%s'\n",inclnam);
//    fmem_puts(inclnam,outfl);
    }*/
  else
    cc_error("unknown preprocessor directive '%s'",dname);
}


int in_comment=0;
void remove_comments(char*trf) {
  char tbuffer[MAX_LINE_LENGTH];
  if (in_comment) {  // already in a multi-line comment
    char*cmend=strstr(trf,"*/");
    if (cmend!=NULL) {
      cmend+=2;
      strcpy(tbuffer,cmend);
      strcpy(trf,tbuffer);
      in_comment=0;
      }
    else {
      trf[0]=0;
      return;   // still in the comment
      }
    }
  char*strpt = trf;
  char*comment_start = trf;
  while (strpt[0]!=0) {
    if ((strpt[0] == '\"') && (in_comment == 0)) {
      strpt++;
      while ((strpt[0] != '\"') && (strpt[0]!=0)) strpt++;
      if (strpt[0]==0) break;
      }

    if ((strncmp(strpt,"//",2)==0) && (in_comment == 0)) {
      strpt[0]=0;  // chop off the end of the line
      break;
      }
    if ((strncmp(strpt,"/*",2)==0) && (in_comment == 0)) {
      in_comment = 1;
      comment_start = strpt;
      strpt += 2;
    }
    if ((strncmp(strpt,"*/",2)==0) && (in_comment == 1)) {
      comment_start[0]=0;
      strcpy(tbuffer,trf);
      int carryonat = strlen(tbuffer);
      strcat(tbuffer,&strpt[2]);
      strcpy(trf,tbuffer);
      strpt = &trf[carryonat];
      in_comment = 0;
    }
    else if (strpt[0] != 0)
      strpt++;
  }
  if (in_comment) comment_start[0] = 0;

  // remove leading spaces and tabs on line
  strpt = trf;
  while ((strpt[0] == ' ') || (strpt[0] == '\t'))
    strpt++;
  strcpy(tbuffer,strpt);
  strcpy(trf,tbuffer);

  // remove trailing spaces on line
  if (strlen(trf) > 0) {
    while (trf[strlen(trf)-1]==' ') trf[strlen(trf)-1]=0;
    }
  }

void reset_compiler() {
  ccError=0;
  in_comment=0;
  currentline=0;
  }


void pre_process_line(char*lin) {

  if (deletingCurrentLine()) {
    lin[0] = 0;
    return;
  }

  char*oldl=(char*)malloc(strlen(lin)+5);
  strcpy(oldl,lin);
  char*opt=oldl;
  char charBefore = 0;

  while (opt[0]!=0) {
    while (is_alphanum(opt[0])==0) {
      if (opt[0]==0) break;
      lin[0]=opt[0]; lin++; opt++; }

    if (opt > oldl)
      charBefore = opt[-1];
    else
      charBefore = 0;

    char thisword[MAX_LINE_LENGTH];
    get_next_word(opt,thisword);
    opt+=strlen(thisword);

    int fni = -1;
    if (charBefore != '.') {
      // object.member -- if member is a #define, it shouldn't be replaced
      fni = macros.find_name(thisword);
    }

    if (fni >= 0) {
      strcpy(lin,macros.macro[fni]);
      lin+=strlen(macros.macro[fni]);
    }
    else {
      strcpy(lin,thisword);
      lin+=strlen(thisword);
    }

  }
  free(oldl);
  lin[0]=0;
  }

// preprocess: takes source INPU as input, and copies the preprocessed output
//   to outp. The output has all comments, #defines, and leading spaces
//   removed from all lines.
void cc_preprocess(const char *inpu,char*outp) {
  reset_compiler();
  FMEM*temp=fmem_create();
  FMEM*iii=fmem_open(inpu);
  char linebuffer[1200];
  currentline=0;
  nested_if_level = 0;

  while (!fmem_eof(iii)) {
    fmem_gets(iii,linebuffer);
    currentline++;
    if (strlen(linebuffer) >= MAX_LINE_LENGTH) {  // stop array overflows in subroutines
      cc_error("line too long (%d chars max)", MAX_LINE_LENGTH);
      break; 
    }

    remove_comments(linebuffer);
    // need to check this, otherwise re-defining a macro causes
    // a big problem:
    // #define a int, #define a void  results in #define int void
    if (linebuffer[0]!='#')
      pre_process_line(linebuffer);
    if (ccError) break;
    if (linebuffer[0]=='#')
      pre_process_directive(linebuffer,temp);
    else {  // don't output the #define lines
//      printf("%s\n",linebuffer);
      fmem_puts(linebuffer,temp);
      }
    if (ccError) break;
    }

  strcpy(outp,temp->data);
  fmem_close(temp);
  fmem_close(iii);

  if ((nested_if_level) && (!ccError))
    cc_error("missing #endif");
 
}


#endif
