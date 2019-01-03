
#include "cs_prepro.h"

void preproc_startup(MacroTable *preDefinedMacros) {
    macros.init();
    if (preDefinedMacros)
        macros.merge(preDefinedMacros);
}

void preproc_shutdown() {
    macros.shutdown();
}
