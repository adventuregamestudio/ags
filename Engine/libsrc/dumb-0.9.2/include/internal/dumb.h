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
 * internal/dumb.h - DUMB's internal declarations.    / / \  \
 *                                                   | <  /   \_
 * This header file provides access to the           |  \/ /\   /
 * internal structure of DUMB, and is liable          \_  /  > /
 * to change, mutate or cease to exist at any           | \ / /
 * moment. Include it at your own peril.                |  ' /
 *                                                       \__/
 * ...
 *
 * I mean it, people. You don't need access to anything in this file. If you
 * disagree, contact the authors. In the unlikely event that you make a good
 * case, we'll add what you need to dumb.h. Thanking you kindly.
 */

#ifndef INTERNAL_DUMB_H
#define INTERNAL_DUMB_H


typedef struct DUH_SIGTYPE_DESC_LINK
{
	struct DUH_SIGTYPE_DESC_LINK *next;
	DUH_SIGTYPE_DESC *desc;
}
DUH_SIGTYPE_DESC_LINK;


typedef struct DUH_SIGNAL
{
	sigdata_t *sigdata;
	DUH_SIGTYPE_DESC *desc;
}
DUH_SIGNAL;


struct DUH
{
	long length;

	int n_signals;
	DUH_SIGNAL **signal;
};


DUH_SIGTYPE_DESC *_dumb_get_sigtype_desc(long type);


#endif /* INTERNAL_DUMB_H */

