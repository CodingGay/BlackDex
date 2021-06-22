#ifndef LITE_COLLECTION_H
#define LITE_COLLECTION_H

#include "xnucxx/LiteObject.h"
#include "xnucxx/LiteIterator.h"

class LiteCollectionInterface : public LiteObject, public LiteIteratorInterface::Delegate {
public:
  virtual unsigned int getCount() = 0;

  virtual unsigned int getCapacity() = 0;

  virtual unsigned int ensureCapacity(unsigned int newCapacity) = 0;

  virtual bool initIterator(void *iterator) const = 0;

  virtual bool getNextObjectForIterator(void *iterator, LiteObject **ret) const = 0;
};

#endif