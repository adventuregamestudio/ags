#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cs_compiler.h"
#include "cc_macrotable.h"
#include "cc_compiledscript.h"
#include "cc_symboltable.h"

#include "script/cc_error.h"
#include "script/cc_options.h"
#include "script/script_common.h"

#include "cs_prepro.h"
#include "cs_parser.h"

const char *ccSoftwareVersion = "1.0";

char **defaultheaders = NULL;
char **defaultHeaderNames = NULL;
static int numheaders = 0;
static int capacityHeaders = 0;

MacroTable predefinedMacros;

int ccAddDefaultHeader(char *nhead, char *nName)
{
    if (numheaders >= capacityHeaders)
    {
        capacityHeaders += 50;
        defaultheaders = (char **)realloc(defaultheaders, sizeof(char *) * capacityHeaders);
        defaultHeaderNames = (char **)realloc(defaultHeaderNames, sizeof(char *) * capacityHeaders);
    }

    defaultheaders[numheaders] = nhead;
    defaultHeaderNames[numheaders] = nName;
    numheaders++;
    return 0;
}

void ccRemoveDefaultHeaders()
{
    numheaders = 0;
}

void ccDefineMacro(const char *macro, const char *definition)
{
    predefinedMacros.add((char *)macro, (char *)definition);
}

void ccClearAllMacros()
{
    predefinedMacros.shutdown();
    predefinedMacros.init();
}

void ccSetSoftwareVersion(const char *versionNumber)
{
    ccSoftwareVersion = versionNumber;
}

ccScript *ccCompileText(const char *texo, const char *scriptName)
{
    ccCompiledScript *cctemp = new ccCompiledScript();
    cctemp->init();

    sym.reset();
    preproc_startup(&predefinedMacros);

    if (scriptName == NULL)
        scriptName = "Main script";

    ccError = 0;
    ccErrorLine = 0;

    for (size_t header = 0; header < numheaders; header++)
    {
        if (defaultHeaderNames[header] != NULL)
            ccCurScriptName = defaultHeaderNames[header];
        else
            ccCurScriptName = "Internal header file";

        cctemp->start_new_section(ccCurScriptName);
        cc_compile(defaultheaders[header], cctemp);
        if (ccError) break;
    }

    if (!ccError)
    {
        ccCurScriptName = scriptName;
        cctemp->start_new_section(ccCurScriptName);
        cc_compile(texo, cctemp);
    }
    preproc_shutdown();

    if (ccError)
    {
        cctemp->shutdown();
        delete cctemp;
        return NULL;
    }

    for (size_t entries_idx = 0; entries_idx < sym.entries.size(); entries_idx++)
    {
        int stype = sym.get_type(entries_idx);
        // blank out the name for imports that are not used, to save space
        // in the output file
        if (((sym.entries[entries_idx].flags & SFLG_IMPORTED) != 0) && ((sym.entries[entries_idx].flags & SFLG_ACCESSED) == 0))
        {

            if ((stype == SYM_FUNCTION) || (stype == SYM_GLOBALVAR))
            {
                // unused func/variable
                cctemp->imports[sym.entries[entries_idx].soffs][0] = 0;
            }
            else if (sym.entries[entries_idx].flags & SFLG_ATTRIBUTE)
            {
                // unused attribute -- get rid of the getter and setter
                int attr_get = sym.entries[entries_idx].get_attrget();
                if (attr_get > 0)
                    cctemp->imports[attr_get][0] = 0;
                int attr_set = sym.entries[entries_idx].get_attrset();
                if (attr_set > 0)
                    cctemp->imports[attr_set][0] = 0;
            }
        }

        if ((sym.get_type(entries_idx) != SYM_GLOBALVAR) &&
            (sym.get_type(entries_idx) != SYM_LOCALVAR))
            continue;

        if (sym.entries[entries_idx].flags & SFLG_IMPORTED)
            continue;
        if (ccGetOption(SCOPT_SHOWWARNINGS) == 0);
        else if ((sym.entries[entries_idx].flags & SFLG_ACCESSED) == 0)
        {
            printf("warning: variable '%s' is never used\n", sym.get_friendly_name(entries_idx).c_str());
        }
    }

    if (ccGetOption(SCOPT_EXPORTALL))
    {
        // export all functions
        for (size_t func_num = 0; func_num < cctemp->numfunctions; func_num++)
        {
            if (cctemp->add_new_export(cctemp->functions[func_num], EXPORT_FUNCTION,
                cctemp->funccodeoffs[func_num], cctemp->funcnumparams[func_num]) == -1)
            {
                cctemp->shutdown();
                return NULL;
            }

        }
    }

    cctemp->free_extra();
    return cctemp;
}
