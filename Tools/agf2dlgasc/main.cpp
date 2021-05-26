#include <stdio.h>
#include <vector>
#include "data/agfreader.h"
#include "data/dialogscriptconv.h"
#include "util/file.h"
#include "util/stream.h"
#include "util/string_compat.h"

using namespace AGS::Common;
using namespace AGS::DataUtil;
namespace AGF = AGS::AGF;


const char *HELP_STRING = "Usage: agf2dlgasc <in-game.agf> <out-dialog.asc>\n";

int main(int argc, char *argv[])
{
    printf("agf2dlgasc v0.1.0 - AGS game's dialog script generator\n"\
        "Copyright (c) 2021 AGS Team and contributors\n");
    for (int i = 1; i < argc; ++i)
    {
        const char *arg = argv[i];
        if (ags_stricmp(arg, "--help") == 0 || ags_stricmp(arg, "/?") == 0 || ags_stricmp(arg, "-?") == 0)
        {
            printf("%s\n", HELP_STRING);
            return 0; // display help and bail out
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
    printf("Input game AGF: %s\n", src);
    printf("Output script body: %s\n", dst);

    //-----------------------------------------------------------------------//
    // Read Game.agf
    //-----------------------------------------------------------------------//
    AGF::AGFReader reader;
    HError err = reader.Open(src);
    if (!err)
    {
        printf("Error: failed to open source AGF:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }

    GameRef game_obj;
    AGF::ReadGameRef(game_obj, reader);
    // We need to query for separate dialog nodes, because ReadGameRef()
    // does not load dialog scripts; we'll have to do that on our own.
    AGF::Dialogs p_dialogs;
    AGF::Dialog p_dialog;
    std::vector<AGF::DocElem> dlg_elems;
    p_dialogs.GetAll(reader.GetGameRoot(), dlg_elems);

    //-----------------------------------------------------------------------//
    // Convert dialog scripts one by one and merge into the single script body
    //-----------------------------------------------------------------------//
    // There's a bunch of standard functions that are always prepended to the script
    String body = DialogScriptDefault;
    // Now load dialogs one by one, convert and append
    for (size_t i = 0; i < game_obj.Dialogs.size(); ++i)
    {
        const DialogRef &dialog_obj = game_obj.Dialogs[i];
        AGF::DocElem dialog_el = dlg_elems[i];
        const String dialog_script = p_dialog.ReadScript(dialog_el);

        DialogScriptConverter conv(dialog_script, game_obj, dialog_obj);
        String script = conv.Convert();

        body.Append(String::FromFormat("%sDialog %d\"\n", NEW_SCRIPT_MARKER, dialog_obj.ID));
        if (conv.GetErrors().size() > 0)
        {
            printf("%s compilation output:\n", dialog_obj.ScriptName.GetCStr());
            printf("----------------------------------------\n");
            for (const auto &e : conv.GetErrors())
                printf("%s: Line %zu: %s\n", e.Error ? "Error" : "Warning", e.LineNumber, e.Message.GetCStr());
            printf("----------------------------------------\n");
            for (const auto &e : conv.GetErrors())
            {
                if (e.Error)
                {
                    printf("There were conversion errors.\nStop.\n");
                    return -1;
                }
            }
        }
        body.Append(script);
    }

    //-----------------------------------------------------------------------//
    // Write script body
    //-----------------------------------------------------------------------//
    Stream *out = File::CreateFile(dst);
    if (!out)
    {
        printf("Error: failed to open script header for writing.\n");
        return -1;
    }
    out->Write(body.GetCStr(), body.GetLength());
    delete out;
    printf("Script body written successfully.\nDone.\n");
    return 0;
}
