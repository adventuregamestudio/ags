
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cc_symboltable.h"
#include "script/script_common.h"      // macro definitions
#include "cc_symboldef.h"   // macro definitions


symbolTable::symbolTable() { numsymbols=0; currentscope=0; usingTempBuffer = 0; stringStructSym = 0; }
int symbolTable::get_num_args(int funcSym) {
    return sscope[funcSym] % 100;
}
int symbolTable::get_type(int ii) {
    // just return the real type, regardless of pointerness/constness
    ii &= ~(STYPE_POINTER | STYPE_CONST | STYPE_DYNARRAY);

    if ((ii < 0) || (ii >= numsymbols)) return -1;
    return stype[ii];
}

int symbolTable::is_loadable_variable(int symm) {
    if ((stype[symm] == SYM_GLOBALVAR) || (stype[symm] == SYM_LOCALVAR)
        || (stype[symm] == SYM_CONSTANT))
        return 1;

    return 0;
}

void symbolTable::set_propfuncs(int symb, int propget, int propset) {
    soffs[symb] = (propget << 16) | propset;
}
int symbolTable::get_propget(int symb) {
    int toret = (soffs[symb] >> 16) & 0x00ffff;
    if (toret == 0xffff)
        return -1;
    return toret;
}
int symbolTable::get_propset(int symb) {
    int toret = soffs[symb] & 0x00ffff;
    if (toret == 0xffff)
        return -1;
    return toret;
}


void symbolTable::reset() {
    int rr;
    for (rr=0;rr<numsymbols;rr++) free(sname[rr]);
    numsymbols=0;
    currentscope=0;
    stringStructSym = 0;
    symbolTree.clear();
    add_ex("___dummy__sym0",999,0);
    normalIntSym = add_ex("int",SYM_VARTYPE,4);
    add_ex("char",SYM_VARTYPE,1);
    add_ex("long",SYM_VARTYPE,4);
    add_ex("short",SYM_VARTYPE,2);
    normalStringSym = add_ex("string",SYM_VARTYPE,4);
    normalVoidSym = add_ex("void",SYM_VARTYPE,0);
    normalFloatSym = add_ex("float", SYM_VARTYPE, 4);
    add_ex("=",SYM_ASSIGN,0);
    add_ex(";",SYM_SEMICOLON,0);
    add_ex(",",SYM_COMMA,0);
    add_ex("(",SYM_OPENPARENTHESIS,0);
    add_ex(")",SYM_CLOSEPARENTHESIS,0);
    add_ex("{",SYM_OPENBRACE,0);
    add_ex("}",SYM_CLOSEBRACE,0);
    add_ex("+=",SYM_MASSIGN,SCMD_ADDREG);
    add_ex("-=",SYM_MASSIGN,SCMD_SUBREG);
    add_ex("*=",SYM_MASSIGN,SCMD_MULREG);
    add_ex("/=",SYM_MASSIGN,SCMD_DIVREG);
    add_ex("&=",SYM_MASSIGN,SCMD_BITAND);
    add_ex("|=",SYM_MASSIGN,SCMD_BITOR);
    add_ex("^=",SYM_MASSIGN,SCMD_XORREG);
    add_ex("<<=",SYM_MASSIGN,SCMD_SHIFTLEFT);
    add_ex(">>=",SYM_MASSIGN,SCMD_SHIFTRIGHT);
    add_ex("++",SYM_SASSIGN,SCMD_ADD);
    add_ex("--",SYM_SASSIGN,SCMD_SUB);
    // the second argument to the operators is their precedence: 1 is highest
    add_operator("!",1, SCMD_NOTREG);
    add_operator("*",2, SCMD_MULREG);
    add_operator("/",3, SCMD_DIVREG);
    add_operator("%",4, SCMD_MODREG);
    add_operator("+",5, SCMD_ADDREG);
    add_operator("-",5, SCMD_SUBREG);
    add_operator("<<",7, SCMD_SHIFTLEFT);
    add_operator(">>",8, SCMD_SHIFTRIGHT);
    add_operator("&",9, SCMD_BITAND);
    add_operator("|",10, SCMD_BITOR);
    add_operator("^",11, SCMD_XORREG);
    add_operator("==",12, SCMD_ISEQUAL);
    add_operator("!=",13, SCMD_NOTEQUAL);
    add_operator(">",14, SCMD_GREATER);
    add_operator("<",15, SCMD_LESSTHAN);
    add_operator(">=",16, SCMD_GTE);
    add_operator("<=",17, SCMD_LTE);
    add_operator("&&",18, SCMD_AND);
    add_operator("||",19, SCMD_OR);
    add_ex("new", SYM_NEW, 1);
    add_ex("[",SYM_OPENBRACKET,0);
    add_ex("]",SYM_CLOSEBRACKET,0);
    add_ex(".",SYM_DOT,0);
    add_ex("if",SYM_IF,0);
    add_ex("else",SYM_ELSE,0);
    add_ex("while",SYM_WHILE,0);
    add_ex("for",SYM_FOR,0);
    add_ex("break",SYM_BREAK,0);
    add_ex("continue",SYM_CONTINUE,0);
    add_ex("do",SYM_DO,0);
    add_ex("...",SYM_VARARGS,0);
    add_ex("struct",SYM_STRUCT,0);
    add_ex("import",SYM_IMPORT,0);
    add_ex("_tryimport",SYM_IMPORT,0);
    add_ex("export",SYM_EXPORT,0);
    add_ex("return", SYM_RETURN, 0);
    add_ex("readonly",SYM_READONLY,0);
    add_ex("::", SYM_MEMBERACCESS, 0);
    add_ex("attribute", SYM_PROPERTY, 0);
    add_ex("enum", SYM_ENUM, 0);
    add_ex("managed", SYM_MANAGED, 0);
    nullSym = add_ex("null", SYM_NULL, 0);
    add_ex("extends", SYM_EXTENDS, 0);
    add_ex("static", SYM_STATIC, 0);
    add_ex("protected", SYM_PROTECTED, 0);
    add_ex("writeprotected", SYM_WRITEPROTECTED, 0);
    add_ex("const", SYM_CONST, 0);
    add_ex("internalstring", SYM_STRINGSTRUCT, 0);
    add_ex("autoptr", SYM_AUTOPTR, 0);
    add_ex("noloopcheck", SYM_LOOPCHECKOFF, 0);
    add_ex("builtin", SYM_BUILTIN, 0);
}
int symbolTable::operatorToVCPUCmd(int opprec) {
    //return ssize[opprec] + 8;
    return vartype[opprec];
}
int symbolTable::find(const char*ntf) {
    return symbolTree.findValue(ntf);
    /*
    int ss;

    for (ss=0;ss<numsymbols;ss++) {
    if (strcmp(ntf,sname[ss]) == 0)
    return ss;
    }
    return -1;*/
}
char*symbolTable::get_name(int idx) {

    if (idx & STYPE_CONST) {
        // return "const" version of name, using alternating buffer
        // so that this can be called twice by the caller
        idx &= ~STYPE_CONST;

        int bufferIdx = usingTempBuffer;
        usingTempBuffer = (usingTempBuffer + 1) % 2;
        sprintf(tempBuffer[bufferIdx], "const %s", get_name(idx));
        return &tempBuffer[bufferIdx][0];
    }

    if (idx & STYPE_DYNARRAY) {
        // dynamic array
        idx &= ~(STYPE_DYNARRAY | STYPE_POINTER);
        if ((idx >= 0) && (idx < numsymbols)) {
            int bufferIdx = usingTempBuffer;
            usingTempBuffer = (usingTempBuffer + 1) % 2;
            sprintf(tempBuffer[bufferIdx], "%s[]", get_name(idx));
            return &tempBuffer[bufferIdx][0];
        }
        return NULL;
    }

    if (idx & STYPE_POINTER) {
        // it's a pointer -- return the secret pointer version
        // of the name
        idx &= ~STYPE_POINTER;
        if ((idx >= 0) && (idx < numsymbols)) {
            return &sname[idx][strlen(sname[idx]) + 1];
        }
        return NULL;
    }

    if ((idx >= 0) && (idx < numsymbols))
        return sname[idx];
    return NULL;
}
int symbolTable::add(char*nta) {
    return add_ex(nta,0,0);
}
int symbolTable::add_ex(char*nta,int typo,char sizee) {
    if (find(nta) >= 0) return -1;
    if (numsymbols >= MAXSYMBOLS) return -1;
    sname[numsymbols]=(char*)malloc(strlen(nta) * 2 + 5);
    // put the name, followed by the pointer-equivalent
    strcpy(sname[numsymbols], nta);
    strcpy(&sname[numsymbols][strlen(nta) + 1], nta);
    strcat(&sname[numsymbols][strlen(nta) + 1], "*");

    stype[numsymbols]=typo;
    ssize[numsymbols]=sizee;
    sscope[numsymbols] = 0;
    arrsize[numsymbols] = 0;
    flags[numsymbols] = 0;
    vartype[numsymbols] = 0;
    extends[numsymbols] = 0;
    symbolTree.addEntry(sname[numsymbols], numsymbols);
    numsymbols++;
    return numsymbols-1;
}
int symbolTable::add_operator(char *nta, int priority, int vcpucmd) {
    int nss = add_ex(nta, SYM_OPERATOR, priority);
    if (nss >= 0)
        vartype[nss] = vcpucmd;
    return nss;
}

symbolTable sym;
