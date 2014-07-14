/*  _______         ____    __         ___    ___
* \    _  \       \    /  \  /       \   \  /   /       '   '  '
*  |  | \  \       |  |    ||         |   \/   |         .      .
*  |  |  |  |      |  |    ||         ||\  /|  |
*  |  |  |  |      |  |    ||         || \/ |  |         '  '  '
*  |  |  |  |      |  |    ||         ||    |  |         .      .
*  |  |_/  /        \  \__//          ||    |  |
* /_______/ynamic    \____/niversal  /__\  /____\usic   /|  .  . ibliotheque
*                                                      /  \
*                                                     / .  \
* makeduh.c - Function to construct a DUH from       / / \  \
*             its components.                       | <  /   \_
*                                                   |  \/ /\   /
* By entheh.                                         \_  /  > /
*                                                      | \ / /
*                                                      |  ' /
*                                                       \__/
*/

#include <stdlib.h>

#include "dumb.h"
#include "internal/dumb.h"



static DUH_SIGNAL *make_signal(DUH_SIGTYPE_DESC *desc, sigdata_t *sigdata)
{
    DUH_SIGNAL *signal;

    ASSERT((desc->start_sigrenderer && desc->end_sigrenderer) || (!desc->start_sigrenderer && !desc->end_sigrenderer));
    ASSERT(desc->sigrenderer_get_samples && desc->sigrenderer_get_current_sample);

    signal = malloc(sizeof(*signal));

    if (!signal) {
        if (desc->unload_sigdata)
            if (sigdata)
                (*desc->unload_sigdata)(sigdata);
        return NULL;
    }

    signal->desc = desc;
    signal->sigdata = sigdata;

    return signal;
}



DUH *make_duh(long length, int n_signals, DUH_SIGTYPE_DESC *desc[], sigdata_t *sigdata[])
{
    DUH *duh = malloc(sizeof(*duh));
    int i;
    int fail;

    if (duh) {
        duh->n_signals = n_signals;

        duh->signal = malloc(n_signals * sizeof(*duh->signal));

        if (!duh->signal) {
            free(duh);
            duh = NULL;
        }
    }

    if (!duh) {
        for (i = 0; i < n_signals; i++)
            if (desc[i]->unload_sigdata)
                if (sigdata[i])
                    (*desc[i]->unload_sigdata)(sigdata[i]);
        return NULL;
    }

    fail = 0;

    for (i = 0; i < n_signals; i++) {
        duh->signal[i] = make_signal(desc[i], sigdata[i]);
        if (!duh->signal[i])
            fail = 1;
    }

    if (fail) {
        unload_duh(duh);
        return NULL;
    }

    duh->length = length;

    return duh;
}
