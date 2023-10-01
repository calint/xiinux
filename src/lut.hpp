// reviewed: 2023-09-27
#pragma once
#include <cstdlib>
#include <cstring>

namespace xiinux {
template <class T, bool DEL_KEY = false, bool DEL_DATA = false,
          bool DATA_IS_ARRAY = false>
class lut final {
  class el final {
  public:
    const char *key_ = nullptr;
    T data_ = nullptr;
    el *nxt_ = nullptr;
    inline el(const char *key, T data) : key_{key}, data_{data} {}
    inline el(const el &) = delete;
    inline el &operator=(const el &) = delete;

    inline ~el() {
      printf("lut free elem %p: %s\n", static_cast<void *>(this), key_);
      if constexpr (DEL_DATA) {
        if (data_) {
          if constexpr (DATA_IS_ARRAY) {
            delete[] data_;
          } else {
            delete data_;
          }
        }
      }
      if (DEL_KEY)
        delete[] key_;
    }
  };

  el **array_ = nullptr;
  unsigned size_ = 0;

  // note. size must be 2^n because size-1 will be used for bitwise 'and'
  static inline unsigned hash(const char *key, const unsigned size) {
    unsigned i = 0;
    const char *p = key;
    while (*p)
      i += unsigned(*p++);
    i &= size - 1;
    return i;
  }

public:
  // note. size must be 2^n because size-1 will be used for bitwise 'and'
  inline lut(const unsigned size = 8) : size_{size} {
    array_ = static_cast<el **>(calloc(size_t(size), sizeof(el *)));
  }
  inline lut(const lut &) = delete;
  inline lut &operator=(const lut &) = delete;

  inline ~lut() {
    del();
    clear();
    free(array_);
  }

  inline T get(const char *key) const {
    const unsigned h = hash(key, size_);
    el *e = array_[h];
    while (e) {
      if (!strcmp(e->key_, key))
        return e->data_;
      e = e->nxt_;
    }
    return nullptr;
  }

  inline T operator[](const char *key) const { return get(key); }

  inline void put(const char *key, T data, bool allow_overwrite = true) {
    const unsigned h = hash(key, size_);
    el *e = array_[h];
    if (!e) {
      array_[h] = new el(key, data);
      return;
    }
    while (e) {
      if (!strcmp(e->key_, key)) {
        if (!allow_overwrite)
          throw "lut:put:overwrite";
        if constexpr (DEL_DATA) {
          if constexpr (DATA_IS_ARRAY) {
            delete[] e->data_;
          } else {
            delete e->data_;
          }
        }
        if constexpr (DEL_KEY) {
          delete[] key;
        }
        e->data_ = data;
        return;
      }
      if (!e->nxt_) {
        e->nxt_ = new el(key, data);
        return;
      }
      e = e->nxt_;
    }
    throw "lut:put:unreachable";
  }

  inline void clear() {
    for (unsigned i = 0; i < size_; i++) {
      el *e = array_[i];
      while (e) {
        el *nxt = e->nxt_;
        delete e;
        e = nxt;
      }
      array_[i] = nullptr;
    }
  }

private:
  inline void del() {
    for (unsigned i = 0; i < size_; i++) {
      el *e = array_[i];
      while (e) {
        el *nxt = e->nxt_;
        delete e;
        e = nxt;
      }
      array_[i] = nullptr;
    }
  }
};

template <bool DEL_KEY = false, bool DEL_DATA = false,
          bool DATA_IS_ARRAY = false>
using lut_cstr = lut<const char *, DEL_KEY, DEL_DATA, DATA_IS_ARRAY>;

} // namespace xiinux
