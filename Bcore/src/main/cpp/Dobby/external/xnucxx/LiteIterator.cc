#include "xnucxx/LiteIterator.h"
#include "xnucxx/LiteCollection.h"
#include "xnucxx/LiteMemOpt.h"

void LiteCollectionIterator::reset() {
  collection->initIterator(innerIterator);
}

bool LiteCollectionIterator::initWithCollection(const LiteCollectionInterface *inCollection) {
  int *ndxPtr = (int *)LiteMemOpt::alloc(sizeof(int));
  innerIterator = (void *)ndxPtr;

  inCollection->initIterator(this->innerIterator);
  collection = inCollection;

  return true;
}

LiteObject *LiteCollectionIterator::getNextObject() {
  LiteObject *retObj;
  collection->getNextObjectForIterator(this->innerIterator, &retObj);
  return retObj;
}

void LiteCollectionIterator::release() {
  if (innerIterator) {
    LiteMemOpt::free(innerIterator, sizeof(int));

    innerIterator = NULL;
  }
}
