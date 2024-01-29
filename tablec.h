#ifndef TABLEC_H
#define TABLEC_H

#ifdef __cplusplus
extern "C" {
#endif

#define tablec_init(_container, _tablec, _buckets, _capacity)                                  \
  _tablec->_container ## _HT_LENGTH = 0;                                                       \
  _tablec->_container ## _HT_CAPACITY = _capacity;                                             \
  _tablec->_container ## _HT_BUCKETS = _buckets;                                               \
                                                                                               \
  memset(_tablec->_container ## _HT_BUCKETS, 0, sizeof(_container ## _HT_BUCKET) * _capacity);

#define tablec_resize(_container, _tablec, _new_capacity, new_buckets)                                                          \
  do {                                                                                                                          \
    size_t old_capacity = _tablec->_container ## _HT_CAPACITY;                                                                  \
    size_t hash = 0;                                                                                                            \
                                                                                                                                \
    _tablec->_container ## _HT_BUCKETS = new_buckets;                                                                           \
                                                                                                                                \
    memset(new_buckets, 0, sizeof(_container ## HT_BUCKET) * _new_capacity);                                                    \
                                                                                                                                \
    while (old_capacity--) {                                                                                                    \
      if (_container ## _HT_CHECK_NULL(_tablec->_container ## _HT_BUCKETS[old_capacity]._container ## _HT_KEY)) continue;       \
                                                                                                                                \
      hash = 0;                                                                                                                 \
      hash = _container ## _HT_HASH(_tablec->_container ## _HT_BUCKETS[old_capacity]._container ## _HT_KEY, hash);              \
      hash %= _new_capacity;                                                                                                    \
                                                                                                                                \
      if (_container ## _HT_CHECK_NULL(new_buckets[hash]._container ## _HT_KEY)) {                                              \
        new_buckets[hash]._container ## _HT_KEY = _tablec->(_container ## _HT_BUCKETS)[old_capacity]._container ## _HT_KEY;     \
        new_buckets[hash]._container ## _HT_VALUE = _tablec->(_container ## _HT_BUCKETS)[old_capacity]._container ## _HT_VALUE; \
                                                                                                                                \
        continue;                                                                                                               \
      }                                                                                                                         \
                                                                                                                                \
      while (!(_container ## _HT_CHECK_NULL(new_buckets[hash]._container ## _HT_KEY)) && (hash < _tablec->capacity)) hash++;    \
                                                                                                                                \
      if (hash == _tablec->_container ## _HT_CAPACITY) break;                                                                   \
                                                                                                                                \
      if (_container ## _HT_CHECK_NULL(new_buckets[hash]._container ## _HT_KEY)) {                                              \
        new_buckets[hash]._container ## _HT_KEY = _tablec->(_container ## _HT_BUCKETS)[old_capacity]._container ## _HT_KEY;     \
        new_buckets[hash]._container ## _HT_VALUE = _tablec->(_container ## _HT_BUCKETS)[old_capacity]._container ## _HT_VALUE; \
      }                                                                                                                         \
    }                                                                                                                           \
                                                                                                                                \
    _tablec->_container ## _HT_BUCKETS = new_buckets;                                                                           \
  } while (0)

#define tablec_set(_container, _tablec, _key, _value)                                                     \
  do {                                                                                                    \
    size_t hash = 0;                                                                                      \
    int _allocated = 0;                                                                                   \
                                                                                                          \
    if ((_tablec->_container ## _HT_LENGTH - 1) == _tablec->_container ## _HT_CAPACITY) break;            \
                                                                                                          \
    hash = _container ## _HT_HASH(_key, hash);                                                            \
    hash %= _tablec->_container ## _HT_CAPACITY;                                                          \
                                                                                                          \
    while (hash != (size_t)_tablec->_container ## _HT_CAPACITY) {                                         \
      if (_container ## _HT_CHECK_NULL(_tablec->_container ## _HT_BUCKETS[hash]._container ## _HT_KEY)) { \
        _tablec->_container ## _HT_BUCKETS[hash]._container ## _HT_KEY = _key;                            \
        _tablec->_container ## _HT_BUCKETS[hash]._container ## _HT_VALUE = _value;                        \
        _allocated = 1;                                                                                   \
                                                                                                          \
        _tablec->_container ## _HT_LENGTH++;                                                              \
                                                                                                          \
        break;                                                                                            \
      }                                                                                                   \
                                                                                                          \
      hash++;                                                                                             \
    }                                                                                                     \
                                                                                                          \
    /* Bounce back if we reached the end of the table */                                                  \
    if (!_allocated) while (1) {                                                                          \
      if (_container ## _HT_CHECK_NULL(_tablec->_container ## _HT_BUCKETS[hash]._container ## _HT_KEY)) { \
        _tablec->_container ## _HT_BUCKETS[hash]._container ## _HT_KEY = _key;                            \
        _tablec->_container ## _HT_BUCKETS[hash]._container ## _HT_VALUE = _value;                        \
                                                                                                          \
        _tablec->_container ## _HT_LENGTH++;                                                              \
                                                                                                          \
        break;                                                                                            \
      }                                                                                                   \
                                                                                                          \
      if (hash == 0) break;                                                                               \
      hash--;                                                                                             \
    }                                                                                                     \
  } while (0)

#define tablec_del(_container, _tablec, _key)                                                                                                                                                      \
  do {                                                                                                                                                                                             \
    int _found = 0;                                                                                                                                                                                \
    size_t hash = _container ## _HT_HASH(key, hash);                                                                                                                                               \
                                                                                                                                                                                                   \
    hash %= _tablec->_container ## _HT_CAPACITY;                                                                                                                                                   \
                                                                                                                                                                                                   \
    while (hash != _tablec->_container ## _HT_CAPACITY) {                                                                                                                                          \
      if (!(_container ## _HT_CHECK_NULL(_tablec->(_container ## _HT_BUCKETS)[hash]._container ## _HT_KEY)) && _container ## _HT_COMPARE(&_tablec->(_container ## _HT_BUCKETS)[hash], key) == 0) { \
        _container ## _HT_ASSIGN(&_tablec->_container ## _HT_BUCKETS[hash]._container ## _HT_KEY, NULL);                                                                                           \
        _container ## _HT_ASSIGN(&_tablec->_container ## _HT_BUCKETS[hash]._container ## _HT_VALUE, NULL);                                                                                         \
        _found = 1;                                                                                                                                                                                \
                                                                                                                                                                                                   \
        _tablec->_container ## _HT_LENGTH--;                                                                                                                                                       \
                                                                                                                                                                                                   \
        break;                                                                                                                                                                                     \
      }                                                                                                                                                                                            \
                                                                                                                                                                                                   \
      hash++;                                                                                                                                                                                      \
    }                                                                                                                                                                                              \
                                                                                                                                                                                                   \
    /* Bounce back if we reached the end of the table */                                                                                                                                           \
    if (!_found) while (1) {                                                                                                                                                                       \
      if (!(_container ## _HT_CHECK_NULL(_tablec->(_container ## _HT_BUCKETS)[hash]._container ## _HT_KEY)) && _container ## _HT_COMPARE(&_tablec->(_container ## _HT_BUCKETS)[hash], key) == 0) { \
        _container ## _HT_ASSIGN(&_tablec->(_container ## _HT_BUCKETS)[hash]._container ## _HT_KEY, NULL);                                                                                         \
        _container ## _HT_ASSIGN(&_tablec->(_container ## _HT_BUCKETS)[hash]._container ## _HT_VALUE, NULL);                                                                                       \
                                                                                                                                                                                                   \
        _tablec->_container ## _HT_LENGTH--;                                                                                                                                                       \
                                                                                                                                                                                                   \
        break;                                                                                                                                                                                     \
      }                                                                                                                                                                                            \
                                                                                                                                                                                                   \
      if (hash == 0) break;                                                                                                                                                                        \
      hash--;                                                                                                                                                                                      \
    }                                                                                                                                                                                              \
  } while (0)

#define tablec_get(_container, _tablec, _key, _result)                                                                                                                                                               \
  do {                                                                                                                                                                                                               \
    int _found = 0;                                                                                                                                                                                                  \
    size_t hash =  _container ## _HT_HASH(_key, hash);                                                                                                                                                               \
    hash %= _tablec->_container ## _HT_CAPACITY;                                                                                                                                                                     \
                                                                                                                                                                                                                     \
    while (hash != (size_t)_tablec->_container ## _HT_CAPACITY) {                                                                                                                                                    \
      if (!(_container ## _HT_CHECK_NULL(_tablec->_container ## _HT_BUCKETS[hash]._container ## _HT_KEY)) && _container ## _HT_COMPARE(_tablec->_container ## _HT_BUCKETS[hash]._container ## _HT_KEY, _key)) {      \
        _result = &_tablec->_container ## _HT_BUCKETS[hash];                                                                                                                                                         \
        _found = 1;                                                                                                                                                                                                  \
                                                                                                                                                                                                                     \
        break;                                                                                                                                                                                                       \
      }                                                                                                                                                                                                              \
                                                                                                                                                                                                                     \
      hash++;                                                                                                                                                                                                        \
    }                                                                                                                                                                                                                \
                                                                                                                                                                                                                     \
    /* Bounce back if we reached the end of the table */                                                                                                                                                             \
    if (!_found) while (1) {                                                                                                                                                                                         \
      if (!(_container ## _HT_CHECK_NULL(_tablec->_container ## _HT_BUCKETS[hash]._container ## _HT_KEY)) && _container ## _HT_COMPARE(_tablec->_container ## _HT_BUCKETS[hash]._container ## _HT_KEY, _key)) {      \
        _result = &_tablec->_container ## _HT_BUCKETS[hash];                                                                                                                                                         \
        break;                                                                                                                                                                                                       \
      }                                                                                                                                                                                                              \
                                                                                                                                                                                                                     \
      if (hash == 0) break;                                                                                                                                                                                          \
      hash--;                                                                                                                                                                                                        \
    }                                                                                                                                                                                                                \
  } while (0)

#define tablec_full(_container, _tablec) (int)(_tablec->_container ## _HT_CAPACITY == _tablec->_container ## _HT_LENGTH) ? -1 : (int)(_tablec->_container ## _HT_CAPACITY - _tablec->_container ## _HT_LENGTH)

#ifdef __cplusplus
}

#endif

#endif /* TABLEC_H */
