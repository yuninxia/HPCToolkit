// SPDX-FileCopyrightText: Contributors to the HPCToolkit Project
//
// SPDX-License-Identifier: BSD-3-Clause

// -*-Mode: C++;-*-

#ifndef HPCTOOLKIT_PROFILE_UTIL_RAGGED_VECTOR_H
#define HPCTOOLKIT_PROFILE_UTIL_RAGGED_VECTOR_H

#include "once.hpp"
#include "locked_unordered.hpp"
#include "log.hpp"

#include "../stdshim/shared_mutex.hpp"

#include <cassert>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <vector>

namespace hpctoolkit {

class ProfilePipeline;

namespace util {

template<class...> class ragged_vector;
template<class...> class ragged_map;

namespace detail {
/// Structure definition for a set of ragged vectors. One of these must be
/// available to construct a ragged_vector, and stay available to ensure
/// validity.
template<class... InitArgs>
class ragged_struct {
public:
  ragged_struct() : complete(false), m_size(0), m_entries() {};
  ~ragged_struct() = default;

  using initializer_t = std::function<void(void*, InitArgs&&...)>;
  using destructor_t = std::function<void(void*)>;

private:
  struct entry {
    entry(initializer_t&& i, destructor_t&& d, std::size_t s, std::size_t o)
      : initializer(std::move(i)), destructor(std::move(d)), size(s), offset(o)
      {};
    initializer_t initializer;
    destructor_t destructor;
    std::size_t size;
    std::size_t offset;
  };

  // General member, which contains all the info that may be copied into either
  // kind of member. Automatically converts into the correct version when
  // needed.
  struct generic_member {
    generic_member(ragged_struct& b, std::size_t i, const entry& e)
      : m_base(b), m_index(i), m_entry(e) {};

    ragged_struct& m_base;
    std::size_t m_index;
    entry m_entry;
  };

  template<class T>
  class generic_typed_member : public generic_member {
  public:
    generic_typed_member(generic_member&& m) : generic_member(std::move(m)) {};
    ~generic_typed_member() = default;
  };

public:
  /// Member of a ragged_vector. These references are required for access into
  /// applicable ragged_vectors. Otherwise they're mostly opaque.
  class member {
  public:
    member() : m_base(nullptr) {};
    ~member() = default;
    member(const member&) = default;
    member(member&&) = default;

    member(generic_member&& g)
      : m_base(&g.m_base), m_index(g.m_index), m_offset(g.m_entry.offset),
        m_size(g.m_entry.size) {};

    member& operator=(const member&) = default;
    member& operator=(member&&) = default;
    member& operator=(generic_member&& g) { return operator=(member(std::move(g))); }

    std::size_t index() const noexcept { return m_index; }
    std::size_t offset() const noexcept { return m_offset; }
    std::size_t size() const noexcept { return m_size; }

  private:
    friend class ragged_struct;

    ragged_struct* m_base;
    std::size_t m_index;
    std::size_t m_offset;  // Copied out for faster accesses.
    std::size_t m_size;  // Also copied out for faster accesses.
  };

  /// Register an arbitrary block of memory for future ragged_vectors.
  /// `init` is called to initialize the block, `des` to destroy it.
  // MT: Internally Synchronized
  generic_member add(std::size_t sz, std::size_t align, initializer_t&& init, destructor_t&& des) {
    assert(!complete && "Cannot add entries to a frozen ragged_struct!");
    std::unique_lock<stdshim::shared_mutex> l(m_mtex);
    auto idx = m_entries.size();
    m_size = (m_size + (align - 1)) & -align;
    m_entries.emplace_back(std::move(init), std::move(des), sz, m_size);
    m_size += sz;
    return generic_member(*this, idx, m_entries[idx]);
  }

  /// Typed version of member, to allow for (more) compile-type type safety.
  template<class T>
  class typed_member : public member {
  public:
    typed_member() : member() {};
    typed_member(const member& m) : member(m) {};
    typed_member(member&& m) : member(std::move(m)) {};
    typed_member(generic_typed_member<T>&& g) : member(std::move(g)) {};
    typed_member& operator=(generic_typed_member<T>&& g) {
      member::operator=(std::move(g));
      return *this;
    }
    ~typed_member() = default;
  };

  /// Register a typed memory block for future ragged_vectors. The constructor
  /// and destructor of T are used for initialization and destruction.
  // MT: Internally Synchronized
  template<class T, class... Args>
  generic_typed_member<T> add(Args&&... args) {
    return add(sizeof(T), alignof(T), [args...](void* v, InitArgs&&... iargs){
      new(v) T(std::forward<InitArgs>(iargs)..., args...);
    }, [](void* v){
      ((T*)v)->~T();
    });
  }

  /// Register a typed memory block for future ragged_vectors. Unlike add(...),
  /// this uses the default constructor and the given function for initialization.
  // MT: Internally Synchronized
  template<class T, class... Args>
  generic_typed_member<T> add_default(std::function<void(T&, InitArgs&&...)>&& init) {
    return add(sizeof(T), alignof(T), [init](void* v, InitArgs&&... iargs){
      new(v) T;
      init(*(T*)v, std::forward<InitArgs>(iargs)...);
    }, [](void* v){
      ((T*)v)->~T();
    });
  }

  /// Register a typed memory block for future ragged_vectors. Unlike add(...),
  /// this uses the default constructor for initialization.
  // MT: Internally Synchronized
  template<class T, class... Args>
  generic_typed_member<T> add_default() {
    return add(sizeof(T), alignof(T), [](void* v, InitArgs&&... iargs){
      new(v) T;
    }, [](void* v){
      ((T*)v)->~T();
    });
  }

  /// Register a typed memory block for future ragged_vectors. Uses the
  /// constructor add() would use, but still calls a custom function after.
  // MT: Internally Synchronized
  template<class T, class... Args>
  generic_typed_member<T> add_initializer(
      std::function<void(T&, InitArgs&&...)>&& init, Args&&... args) {
    return add(sizeof(T), alignof(T), [init, args...](void* v, InitArgs&&... iargs){
      new(v) T(std::forward<InitArgs>(iargs)..., args...);
      init(*(T*)v, std::forward<InitArgs>(iargs)...);
    }, [](void* v){
      ((T*)v)->~T();
    });
  }

  /// Freeze additions to the struct, allowing ragged_vectors to be generated.
  // MT: Externally Synchronized
  void freeze() noexcept { complete = true; }

  /// Check whether the ragged_struct is frozen and ready for actual use.
  // MT: Externally Synchronized (after freeze())
  void valid() {
    assert(complete && "Cannot use a ragged_struct before freezing!");
  }

  /// Check whether a member (or typed_member) is valid with respect to this
  /// ragged_struct. Throws if its not.
  /// NOTE: For ragged_* only.
  // MT: Safe (const)
  void valid(const member& m) const {
    assert(m.m_base && "Attempt to use an empty member!");
    assert(m.m_base == this && "Attempt to use an incompatible member!");
  }

  /// Initialize the given member in the given data block. Adding `direct` means
  /// the data block starts with that member (sparse).
  /// NOTE: For ragged_* only.
  // MT: Externally Synchronized (after freeze())
  void initialize(const member& m, void* d, InitArgs&&... args) {
    valid();
    m_entries[m.m_index].initializer((char*)d + m.m_offset, std::forward<InitArgs>(args)...);
  }

  /// Destroy the given member in the given data block.
  // MT: Externally Synchronized (after freeze())
  void destruct(const member& m, void* d) {
    valid();
    m_entries[m.m_index].destructor((char*)d + m.m_offset);
  }

  /// Access the entry table, with or without locking.
  // MT: Externally Synchronized (after freeze())
  const std::vector<entry>& entries() {
    valid();
    return m_entries;
  }

  /// Get a member for the given index. NOTE: For ragged_* only.
  // MT: Externally Synchronized (after freeze())
  member memberFor(std::size_t i) {
    valid();
    return generic_member(*this, i, m_entries[i]);
  }

  /// Get the final total size of the struct.
  // MT: Externally Synchronized (after freeze())
  std::size_t size() {
    valid();
    return m_size;
  }

private:
  bool complete;
  stdshim::shared_mutex m_mtex;
  std::size_t m_size;
  std::vector<entry> m_entries;
};
}

/// Ragged vectors are vectors with a runtime-defined structure. Unlike normal
/// vectors, their size can't be changed after allocation. Like normal vectors,
/// the items are allocated in one big block without any other mess.
template<class... InitArgs>
class ragged_vector {
public:
  using struct_t = detail::ragged_struct<InitArgs...>;
  using member_t = typename struct_t::member;
  template<class T>
  using typed_member_t = typename struct_t::template typed_member<T>;

  template<class... Args>
  ragged_vector(struct_t& rs, Args&&... args)
    : m_base(rs), flags(m_base.entries().size()), data(new char[m_base.size()]) {
    auto base_r = std::ref(m_base);
    auto data_r = data.get();
    init = [base_r, data_r, args...](const member_t& m){
      base_r.get().initialize(m, data_r, args...);
    };
  }

  ragged_vector(const ragged_vector& o) = delete;
  ragged_vector(ragged_vector&& o)
    : m_base(o.m_base), flags(std::move(o.flags)), init(std::move(o.init)),
      data(std::move(o.data)) {};

  template<class... Args>
  ragged_vector(ragged_vector&& o, Args&&... args)
    : m_base(o.m_base), flags(std::move(o.flags)), data(std::move(o.data)) {
    auto base_r = std::ref(m_base);
    auto data_r = data.get();
    init = [base_r, data_r, args...](const member_t& m){
      base_r.get().initialize(m, data_r, args...);
    };
  }

  ~ragged_vector() noexcept {
    if(!data) return;  // Not our problem anymore.
    const auto& es = m_base.entries();
    for(std::size_t i = 0; i < es.size(); i++) {
      if(flags[i].query()) m_base.destruct(m_base.memberFor(i), data.get());
    }
  }

  /// Eagerly initialize every entry in the vector. If this is not called,
  /// members will be initialized lazily.
  // MT: Internally Synchronized
  void initialize() noexcept {
    const auto& es = m_base.entries();
    for(std::size_t i = 0; i < es.size(); i++)
      flags[i].call_nowait(init, m_base.memberFor(i));
  }

  /// Get a pointer to an entry in the vector. Throws if the member is invalid.
  // MT: Internally Synchronized
  [[nodiscard]] void* at(const member_t& m) noexcept {
    m_base.valid(m);
    flags[m.index()].call(init, m);
    return &data[m.offset()];
  }

  /// Typed version of at(), that works with typed_members.
  // MT: Internally Synchronized
  template<class T>
  [[nodiscard]] T* at(const typed_member_t<T>& m) {
    return static_cast<T*>(at((const member_t&)m));
  }

  // operator[], for ease of use.
  // MT: Internally Synchronized
  template<class T>
  [[nodiscard]] T& operator[](const typed_member_t<T>& m) { return *at(m); }
  template<class K>
  [[nodiscard]] auto& operator[](const K& k) { return *at(k(*this)); }

  struct_t& base() { return m_base; }

private:
  struct_t& m_base;
  struct cleanup {
    void operator()(char* d) { delete[] d; }
  };
  std::vector<util::OnceFlag> flags;
  std::function<void(const member_t&)> init;
  std::unique_ptr<char[], cleanup> data;
};

}
}

#endif  // HPCTOOLKIT_PROFILE_UTIL_RAGGED_VECTOR_H
