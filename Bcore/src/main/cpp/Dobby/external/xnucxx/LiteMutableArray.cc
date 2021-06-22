#include "xnucxx/LiteMutableArray.h"
#include "xnucxx/LiteMemOpt.h"

LiteMutableArray::LiteMutableArray(int initCapacity) {
  unsigned int arraySize = 0;
  arraySize = initCapacity * sizeof(LiteObject *);
  array = (const LiteObject **)LiteMemOpt::alloc(arraySize);
  array_count = 0;
  array_capacity = initCapacity;
}

LiteMutableArray::~LiteMutableArray() {
  release();
}

LiteObject *LiteMutableArray::getObject(const int index) {
  return (LiteObject *)array[index];
}

bool LiteMutableArray::pushObject(const LiteObject *object) {
  unsigned int newCount = array_count + 1;
  if (newCount > array_capacity && newCount > ensureCapacity(newCount))
    return false;

  array[array_count] = object;
  array_count++;
  return true;
}

unsigned int LiteMutableArray::getCount() {
  return array_count;
}

unsigned int LiteMutableArray::getCapacity() {
  return array_capacity;
}

unsigned int LiteMutableArray::ensureCapacity(unsigned int newCapacity) {
  if (newCapacity <= array_capacity)
    return array_capacity;

#define CAPACITY_STEP 64
  newCapacity = (int)ALIGN(newCapacity + CAPACITY_STEP, CAPACITY_STEP);

  // alloc new buffer
  int newSize;
  const LiteObject **newArray;
  newSize = sizeof(LiteObject *) * newCapacity;
  newArray = (const LiteObject **)LiteMemOpt::alloc(newSize);
  if (newArray == nullptr) {
    return 0;
  }

  // clear buffer content
  _memset(newArray, 'A', newSize);

  // copy the origin content
  int originContentSize = sizeof(LiteObject *) * array_count;
  _memcpy(newArray, array, originContentSize);

  // free the origin
  int originArraySize = array_capacity * sizeof(LiteObject *);
  LiteMemOpt::free(array, originArraySize);

  // update info
  this->array = newArray;
  this->array_capacity = newCapacity;

  return newCapacity;
}

// impl iterator delegate
bool LiteMutableArray::initIterator(void *iterator) const {
  unsigned int *ndxPtr = (unsigned int *)iterator;
  *ndxPtr = 0;
  return true;
}

// impl iterator delegate
bool LiteMutableArray::getNextObjectForIterator(void *iterator, LiteObject **ret) const {
  unsigned int *ndxPtr = (unsigned int *)iterator;
  unsigned int ndx = (*ndxPtr)++;

  if (ndx < array_count) {
    *ret = (LiteObject *)array[ndx];
    return true;
  } else {
    *ret = nullptr;
    return false;
  }
}

void LiteMutableArray::release() {
  if (array != NULL) {
    unsigned int arraySize = 0;
    arraySize = array_capacity * sizeof(LiteObject *);
    LiteMemOpt::free(array, arraySize);

    array = NULL;
  }
}