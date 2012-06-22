
#include "ac/eventblock.h"

#ifdef UNUSED_CODE
// [IKM] 2012-06-22: not really used anywhere
void add_to_eventblock(EventBlock *evpt, int evnt, int whatac, int val1, int data, short scorr) {
  evpt->list[evpt->numcmd] = evnt;
  evpt->respond[evpt->numcmd] = whatac;
  evpt->respondval[evpt->numcmd] = val1;
  evpt->data[evpt->numcmd] = data;
  evpt->score[evpt->numcmd] = scorr;
  evpt->numcmd++;
}
#endif // UNUSED_CODE
