
#include <string.h>
#include "cc_treemap.h"


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

