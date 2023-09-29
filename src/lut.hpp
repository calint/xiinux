// reviewed: 2023-09-27
#pragma once
#include <cstdlib>
#include <cstring>

namespace xiinux {
template <class T> class lut final {
  class el final {
  public:
    const char *key_ = nullptr;
    T data_ = nullptr;
    el *nxt_ = nullptr;
    inline el(const char *key, T data) : key_{key}, data_{data} {}
    inline el(const el &) = delete;
    inline el &operator=(const el &) = delete;

    inline void delete_content_recurse(const bool delete_key) {
      if (data_)
        delete data_;
      if (delete_key)
        delete[] key_;
    }
  };
  el **array_ = nullptr;
  unsigned size_ = 0;
  mutable const el *has_found_el_ = nullptr;

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
    clear();
    free(array_);
  }

  inline bool has(const char *key) const {
    has_found_el_ = get_el_for_key(key);
    return has_found_el_ != nullptr;
  }

  inline T operator[](const char *key) const {
    const el *e = get_el_for_key(key);
    return e ? e->data_ : nullptr;
  }

  inline const T &get_ref_const(const char *key) const {
    const el *e = get_el_for_key(key);
    if (e)
      return e->data_;
    throw "lut:get_ref_const:not_found";
  }

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

  inline void delete_content(const bool delete_keys) {
    for (unsigned i = 0; i < size_; i++) {
      el *e = array_[i];
      while (e) {
        el *nxt = e->nxt_;
        e->delete_content_recurse(delete_keys);
        delete e;
        e = nxt;
      }
      array_[i] = nullptr;
    }
  }

private:
  inline const el *get_el_for_key(const char *key) const {
    if (has_found_el_) { // try the last found element from "has"
      const el *e = has_found_el_;
      has_found_el_ = nullptr;
      if (!strcmp(e->key_, key))
        return e;
    }
    const unsigned h = hash(key, size_);
    el *e = array_[h];
    while (e) {
      if (!strcmp(e->key_, key))
        return e;
      e = e->nxt_;
    }
    return nullptr;
  }
};
} // namespace xiinux
