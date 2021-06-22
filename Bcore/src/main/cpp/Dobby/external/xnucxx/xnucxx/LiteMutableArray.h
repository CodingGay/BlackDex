#ifndef LITE_MUTABLE_ARRAY_H
#define LITE_MUTABLE_ARRAY_H

#include "xnucxx/LiteCollection.h"
#include "xnucxx/LiteIterator.h"

class LiteMutableArray : public LiteCollectionInterface {
protected:
  const LiteObject **array;

  unsigned int array_count;

  unsigned int array_capacity;

public:
  explicit LiteMutableArray(int count);

  ~LiteMutableArray();

  // === LiteObject override ==
  void release() override;

  // === LiteCollectionInterface override ==
  unsigned int getCount() override;

  unsigned int getCapacity() override;

  unsigned int ensureCapacity(unsigned int newCapacity) override;

  // === LiteIteratorInterface::Delegate override ==
  bool initIterator(void *iterator) const override;

  bool getNextObjectForIterator(void *iterator, LiteObject **ret) const override;

  virtual LiteObject *getObject(int index);

  virtual bool pushObject(const LiteObject *object);
};

#endif
