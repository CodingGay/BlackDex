#ifndef LITE_ITERATOR_H
#define LITE_ITERATOR_H

#include "xnucxx/LiteObject.h"

class LiteIteratorInterface : public LiteObject {
public:
  class Delegate {
  public:
    virtual bool initIterator(void *iterationContext) const = 0;

    virtual bool getNextObjectForIterator(void *iterationContext, LiteObject **nextObject) const = 0;
  };

public:
  virtual void reset() = 0;

  virtual LiteObject *getNextObject() = 0;
};

class LiteCollectionInterface;
class LiteCollectionIterator : public LiteIteratorInterface {
protected:
  const LiteCollectionInterface *collection;

  void *innerIterator;

public:
  explicit LiteCollectionIterator(const LiteCollectionInterface *collection) {
    initWithCollection(collection);
  }

  ~LiteCollectionIterator() {
    release();
  }

  // === LiteObject override ===
  void release() override;

  // === LiteIteratorInterface override ===
  void reset() override;

  LiteObject *getNextObject() override;

  bool initWithCollection(const LiteCollectionInterface *collection);
};

#endif
