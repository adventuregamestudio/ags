/*
  This is UNPUBLISHED PROPRIETARY SOURCE CODE;
  the contents of this file may not be disclosed to third parties,
  copied or duplicated in any form, in whole or in part, without
  prior express permission from Chris Jones.
*/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#ifdef _MANAGED
// ensure this doesn't get compiled to .NET IL
#pragma unmanaged
#endif

#include "cscomp.h"

int ccCompOptions = SCOPT_LEFTTORIGHT;
int currentline = 0;
int ccError = 0;
int ccErrorLine = 0;
char ccErrorString[400];
char ccErrorCallStack[400];
bool ccErrorIsUserError = false;
const char *ccCurScriptName = "";

void cc_error(char *descr, ...)
{
  ccErrorCallStack[0] = 0;
  ccErrorIsUserError = false;
  if (descr[0] == '!')
  {
    ccErrorIsUserError = true;
    descr++;
  }

  char displbuf[1000];
  va_list ap;

  va_start(ap, descr);
  vsprintf(displbuf, descr, ap);
  va_end(ap);

  if (currentline > 0) {
    
    if (ccGetCurrentInstance() == NULL) {
      sprintf(ccErrorString, "Error (line %d): %s", currentline, displbuf);
    }
    else {
      sprintf(ccErrorString, "Error: %s\n", displbuf);
      ccGetCallStack(ccGetCurrentInstance(), ccErrorCallStack, 5);
    }
  }
  else
    sprintf(ccErrorString, "Runtime error: %s", displbuf);

  ccError = 1;
  ccErrorLine = currentline;
}

void ccSetOption(int optbit, int onoroff)
{
  if (onoroff)
    ccCompOptions |= optbit;
  else
    ccCompOptions &= ~optbit;
}

int ccGetOption(int optbit)
{
  if (ccCompOptions & optbit)
    return 1;

  return 0;
}

const char* ccGetSectionNameAtOffs(ccScript *scri, long offs) {

  int i;
  for (i = 0; i < scri->numSections; i++) {
    if (scri->sectionOffsets[i] < offs)
      continue;
    break;
  }

  // if no sections in script, return unknown
  if (i == 0)
    return "(unknown section)";

  return scri->sectionNames[i - 1];
}

void ccGetCallStack(ccInstance *inst, char *buffer, int maxLines) {

  if (inst == NULL) {
    // not in a script, no call stack
    buffer[0] = 0;
    return;
  }

  sprintf(buffer, "in \"%s\", line %d\n", ccGetSectionNameAtOffs(inst->runningInst->instanceof, inst->pc), inst->line_number);

  char lineBuffer[300];
  int linesDone = 0;
  for (int j = inst->callStackSize - 1; (j >= 0) && (linesDone < maxLines); j--, linesDone++) {
    sprintf(lineBuffer, "from \"%s\", line %d\n", ccGetSectionNameAtOffs(inst->callStackCodeInst[j]->instanceof, inst->callStackAddr[j]), inst->callStackLineNumber[j]);
    strcat(buffer, lineBuffer);
    if (linesDone == maxLines - 1)
      strcat(buffer, "(and more...)\n");
  }

}

void ccFreeScript(ccScript * ccs)
{
  if (ccs->globaldata != NULL)
    free(ccs->globaldata);

  if (ccs->code != NULL)
    free(ccs->code);

  if (ccs->strings != NULL)
    free(ccs->strings);

  if (ccs->fixups != NULL && ccs->numfixups > 0)
    free(ccs->fixups);

  if (ccs->fixuptypes != NULL && ccs->numfixups > 0)
    free(ccs->fixuptypes);

  ccs->globaldata = NULL;
  ccs->code = NULL;
  ccs->strings = NULL;
  ccs->fixups = NULL;
  ccs->fixuptypes = NULL;

  int aa;
  for (aa = 0; aa < ccs->numimports; aa++) {
    if (ccs->imports[aa] != NULL)
      free(ccs->imports[aa]);
  }

  for (aa = 0; aa < ccs->numexports; aa++)
    free(ccs->exports[aa]);

  for (aa = 0; aa < ccs->numSections; aa++)
    free(ccs->sectionNames[aa]);

  if (ccs->sectionNames != NULL)
  {
    free(ccs->sectionNames);
    free(ccs->sectionOffsets);
    ccs->sectionNames = NULL;
    ccs->sectionOffsets = NULL;
  }


  if (ccs->imports != NULL)
  {
    free(ccs->imports);
    free(ccs->exports);
    free(ccs->export_addr);
	  ccs->imports = NULL;
    ccs->exports = NULL;
    ccs->export_addr = NULL;
  }
  ccs->numimports = 0;
  ccs->numexports = 0;
  ccs->numSections = 0;
}

void fputstring(char *sss, FILE *ddd) {
  int b = 0;
  while (sss[b] != 0) {
    fputc(sss[b], ddd);
    b++;
  }
  fputc(0,ddd);
}

void fgetstring_limit(char *sss, FILE *ddd, int bufsize) {
  int b = -1;
  do {
    if (b < bufsize - 1)
      b++;
    sss[b] = fgetc(ddd);
    if (feof(ddd))
      return;
  } while (sss[b] != 0);
}

void fgetstring(char *sss, FILE *ddd) {
  fgetstring_limit (sss, ddd, 50000000);
}

// *** TREEMAP CODE **** //

ICompareStrings ccCompareStringsNormal;

ccTreeMap::ccTreeMap() {
  left = NULL;
  right = NULL;
  text = NULL;
  value = -1;
}

ccTreeMap *ccTreeMap::findNode(const char *key, ICompareStrings *comparer) {
  if (text == NULL) {/*
    // if we are removing items, this entry might have been
    // removed, but we still need to check left and right
    ccTreeMap *tnode;
    if (left != NULL)
      tnode = left->findNode(key, comparer);
    if ((right != NULL) && (tnode == NULL))
      tnode = right->findNode(key, comparer);
    return tnode;*/
    return NULL;
  }

  int cmpv = comparer->compare(key, text);
  if (cmpv == 0)
    return this;

  if (cmpv < 0) {
    if (left == NULL)
      return NULL;
    return left->findNode(key, comparer);
  }
  else {
    if (right == NULL)
      return NULL;
    return right->findNode(key, comparer);
  }
}

int ccTreeMap::findValue(const char* key, ICompareStrings *comparer) {
  ccTreeMap *result = findNode(key, comparer);
  if (result == NULL)
    return -1;
  return result->value;
}

int ccTreeMap::findValue(const char* key) {
  return findValue(key, &ccCompareStringsNormal);
}

void ccTreeMap::Clone(ccTreeMap *node) {
  this->text = node->text;
  this->left = node->left;
  this->right = node->right;
  this->value = node->value;
}

void ccTreeMap::removeNode() {

  // clean up any empty nodes
  if ((left != NULL) && (left->text == NULL)) {
    delete left;
    left = NULL;
  }
  if ((right != NULL) && (right->text == NULL)) {
    delete right;
    right = NULL;
  }

  // delete this node
  if ((left == NULL) && (right == NULL)) {
    // leaf node -- remove it
    text = NULL;
    value = -1;
    return;
  }

  if (left == NULL) {
    // has a right child only -- just move the child up into it
    ccTreeMap *oldNode = right;
    Clone(oldNode);
    oldNode->destroyNonRecursive();
    return;
  }

  if (right == NULL) {
    // has a left child only -- just move the child up into it
    ccTreeMap *oldNode = left;
    Clone(oldNode);
    oldNode->destroyNonRecursive();
    return;
  }

  // at this point, the node to be deleted has both a left
  // and right child
  // locate the rightmost descendant of the left child of the node
  ccTreeMap *searching = left;
  while ((searching->right != NULL) && (searching->right->text != NULL))
    searching = searching->right;

  // pull up the node we found into the deleted one's position
  text = searching->text;
  value = searching->value;

  // remove it (dealing with any left child tree appropriately)
  searching->removeNode();
}

void ccTreeMap::removeEntry(const char *key) {

  ccTreeMap *node = findNode(key, &ccCompareStringsNormal);
  if (node == NULL)
    return;

  node->removeNode();
}

void ccTreeMap::addEntry(const char* ntx, int p_value) {
  if ((ntx == NULL) || (ntx[0] == 0))
    // don't add if it's an empty string or if it's already here
    return;

  if (text == NULL) {
    text = ntx;
    value = p_value;
    return;
  }

  int cmpval = ccCompareStringsNormal.compare(ntx, text);
  if (cmpval == 0) {
    value = p_value;
  }
  else if (cmpval < 0) {
    // Earlier in alphabet, add to left
    if (left == NULL)
      left = new ccTreeMap();

    left->addEntry(ntx, p_value);
  }
  else if (cmpval > 0) {
    // Later in alphabet, add to right
    if (right == NULL)
      right = new ccTreeMap();

    right->addEntry(ntx, p_value);
  }
}

void ccTreeMap::destroyNonRecursive() {
  left = NULL;
  right = NULL;
  text = NULL;
  delete this;
}

void ccTreeMap::clear() {
  if (left) {
    left->clear();
    delete left;
  }
  if (right) {
    right->clear();
    delete right;
  }
  left = NULL;
  right = NULL;
  text = NULL;
  value = -1;
}

ccTreeMap::~ccTreeMap() {
  clear();
}

