#include "script/cs_compiler.h"
#include "script/cc_options.h"
#include "script/cc_error.h"
#include "util/filestream.h"
#include "util/file.h"
#include "util/textstreamreader.h"
#include "util/string_compat.h"

using namespace AGS::Common;

/* needs this or linker fails */
extern int currentline; // in script/script_common

std::pair<String, String> cc_error_at_line(const char* error_msg)
{
    return std::make_pair(String::FromFormat("Error (line %d): %s", currentline, error_msg), String());
}

String cc_error_without_line(const char* error_msg)
{
    return String::FromFormat("Error (line unknown): %s", error_msg);
}


const char *HELP_STRING = "Usage: agscc <input.asc> <output.o> [OPTION...] \n"
"-H, --Headers HEADER  Header Files in order\n"
"-r, --roomscript      Use if is a room script\n"
"-h, --help            Print usage\n";

int main(int argc, char* argv[])
{
    bool isRoomScript = false;
    int h_i_start = 0;
    int h_i_end = 0;
    printf("agscc v0.1.0 - A command line compiler for preprocessed ags scripts\n"\
        "Copyright (c) 2021 AGS Team and contributors\n");

    for (int i = 1; i < argc; ++i)
    {
        const char *arg = argv[i];
        if (ags_stricmp(arg, "--help") == 0 || ags_stricmp(arg, "/?") == 0 || ags_stricmp(arg, "-?") == 0)
        {
            printf("\n%s\n", HELP_STRING);
            return 0; // display help and bail out
        }
        if (ags_stricmp(arg, "--roomscript") == 0 || ags_stricmp(arg, "-r") == 0)
        {
            isRoomScript = true;
        }

        if (ags_stricmp(arg, "--Headers") == 0 || ags_stricmp(arg, "-H") == 0)
        {
            h_i_start = i;
        }
        else if(h_i_start != 0)
        {
            h_i_end = i;
        }
    }
    if (argc < 3)
    {
        printf("Error: not enough arguments\n");
        printf("%s\n", HELP_STRING);
        return -1;
    }

    const char *src = argv[1];
    const char *dst = argv[2];
    printf("Input script file: %s\n", src);
    printf("Output script obj: %s\n", dst);

    std::vector<String> headers;
    if (h_i_start != 0 && h_i_end > h_i_start)
    {
        for(int i=h_i_start+1; i<=h_i_end; i++) {
            headers.emplace_back(argv[i]);
        }
    }

    //-----------------------------------------------------------------------//
    // Configure compiler
    //-----------------------------------------------------------------------//
    ccSetSoftwareVersion("version"); // need to set the version here somehow

    ccSetOption(SCOPT_EXPORTALL, 1);
    ccSetOption(SCOPT_LINENUMBERS, 1);
    // Don't allow them to override imports in the room script
    ccSetOption(SCOPT_NOIMPORTOVERRIDE, isRoomScript);

    ccSetOption(SCOPT_LEFTTORIGHT, 1 /*LeftToRightPrecedence*/);
    ccSetOption(SCOPT_OLDSTRINGS, 0 /*EnforceNewStrings*/);

    ccRemoveDefaultHeaders();

    //-----------------------------------------------------------------------//
    // Read input files
    //-----------------------------------------------------------------------//
    std::vector<String> heads;
    for(const auto& header: headers)
    {
        Stream *in = File::OpenFileRead(header);
        if (!in)
        {
            printf("Error: failed to open header for reading: %s\n", header.GetCStr());
            return -1;
        }
        TextStreamReader sr(in);
        heads.push_back(sr.ReadAll());
        in->Close();

        ccAddDefaultHeader((char*) heads.back().GetCStr(), (char*) header.GetCStr());
    }

    String script_input;
    if(src)
    {
        Stream *in = File::OpenFileRead(src);
        if (!in)
        {
            printf("Error: failed to open script for reading: %s\n", src);
            return -1;
        }
        TextStreamReader sr(in);
        script_input = sr.ReadAll();
        in->Close();
    }

    //-----------------------------------------------------------------------//
    // Compiler script
    //-----------------------------------------------------------------------//
    ccScript* script = ccCompileText(script_input.GetCStr(), src);
    if ((script == nullptr) || (ccError != 0))
    {
        printf("Error: compile failed at %s, line %d : %s\n", ccCurScriptName, ccErrorLine, ccErrorString.GetCStr());
        return -1;
    }

    //-----------------------------------------------------------------------//
    // Write script object
    //-----------------------------------------------------------------------//
    Stream *out = File::CreateFile(dst);
    if (!out || !(out->CanWrite()))
    {
        printf("Error: failed to open for writing: %s\n", dst);
        return -1;
    }
    script->Write(out);
    out->Close();

    return 0;
}