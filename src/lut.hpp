#pragma once
//? replace use with std::unordered_map
namespace xiinux {
template <class T> class lut final {
private:
  class el {
  public:
    const char *key_ = nullptr;
    T data_ = nullptr;
    el *nxt_ = nullptr;
    inline el(const char *key, T data) : key_{key}, data_{data} {}
    inline void delete_content_recurse(const bool delete_key) {
      if (data_)
        delete data_;
      if (delete_key)
        delete[] key_;
    }
  };
  el **array_;
  unsigned size_;

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
  inline lut(const unsigned size = 8) : size_(size) {
    array_ = static_cast<el **>(calloc(size_t(size), sizeof(el *)));
  }
  inline ~lut() {
    clear();
    free(array_);
  }
  inline T operator[](const char *key) {
    const unsigned h = hash(key, size_);
    el *e = array_[h];
    while (e) {
      if (!strcmp(e->key_, key))
        return e->data_;
      e = e->nxt_;
    }
    return nullptr;
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
};
} // namespace xiinux
