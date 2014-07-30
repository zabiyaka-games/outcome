/* spinlock.hpp
Provides yet another spinlock
(C) 2013-2014 Niall Douglas http://www.nedprod.com/
File Created: Sept 2013


Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#ifndef BOOST_SPINLOCK_HPP
#define BOOST_SPINLOCK_HPP

#include <cassert>
#include <vector>

#if !defined(BOOST_SPINLOCK_USE_BOOST_ATOMIC) && defined(BOOST_NO_CXX11_HDR_ATOMIC)
# define BOOST_SPINLOCK_USE_BOOST_ATOMIC
#endif
#if !defined(BOOST_SPINLOCK_USE_BOOST_THREAD) && defined(BOOST_NO_CXX11_HDR_THREAD)
# define BOOST_SPINLOCK_USE_BOOST_THREAD
#endif

#ifdef BOOST_SPINLOCK_USE_BOOST_ATOMIC
# include <boost/atomic.hpp>
#else
# include <atomic>
#endif
#ifdef BOOST_SPINLOCK_USE_BOOST_THREAD
# include <boost/thread.hpp>
#else
# include <thread>
# include <chrono>
#endif

// For lock_guard
#include <mutex>
// For dump
#include <ostream>


// Turn this on if you have a compiler which understands __transaction_relaxed
//#define BOOST_HAVE_TRANSACTIONAL_MEMORY_COMPILER

#ifndef BOOST_SMT_PAUSE
# if defined(_MSC_VER) && _MSC_VER >= 1310 && ( defined(_M_IX86) || defined(_M_X64) )
extern "C" void _mm_pause();
#  pragma intrinsic( _mm_pause )
#  define BOOST_SMT_PAUSE _mm_pause();
# elif defined(__GNUC__) && ( defined(__i386__) || defined(__x86_64__) )
#  define BOOST_SMT_PAUSE __asm__ __volatile__( "rep; nop" : : : "memory" );
# endif
#endif

#ifndef BOOST_NOEXCEPT
# if !defined(_MSC_VER) || _MSC_VER >= 1900
#  define BOOST_NOEXCEPT noexcept
# endif
#endif
#ifndef BOOST_NOEXCEPT
# define BOOST_NOEXCEPT
#endif

#ifndef BOOST_NOEXCEPT_OR_NOTHROW
# if !defined(_MSC_VER) || _MSC_VER >= 1900
#  define BOOST_NOEXCEPT_OR_NOTHROW noexcept
# endif
#endif
#ifndef BOOST_NOEXCEPT_OR_NOTHROW
# define BOOST_NOEXCEPT_OR_NOTHROW throw()
#endif

#ifndef BOOST_CONSTEXPR
# if !defined(_MSC_VER) || _MSC_VER >= 1900
#  define BOOST_CONSTEXPR constexpr
# endif
#endif
#ifndef BOOST_CONSTEXPR
# define BOOST_CONSTEXPR
#endif

#ifndef BOOST_CONSTEXPR_OR_CONST
# if !defined(_MSC_VER) || _MSC_VER >= 1900
#  define BOOST_CONSTEXPR_OR_CONST constexpr
# endif
#endif
#ifndef BOOST_CONSTEXPR_OR_CONST
# define BOOST_CONSTEXPR_OR_CONST const
#endif

namespace boost
{
  namespace spinlock
  {
    // Map in an atomic implementation
    template <class T>
    class atomic
#ifdef BOOST_SPINLOCK_USE_BOOST_ATOMIC
      : public boost::atomic<T>
    {
      typedef boost::atomic<T> Base;
#else
      : public std::atomic<T>
    {
      typedef std::atomic<T> Base;
#endif

    public:
      atomic() : Base() {}
      BOOST_CONSTEXPR atomic(T v) BOOST_NOEXCEPT : Base(std::forward<T>(v)) {}

#ifdef BOOST_NO_CXX11_DELETED_FUNCTIONS
      //private:
      atomic(const Base &) /* =delete */;
#else
      atomic(const Base &) = delete;
#endif
    };//end boost::afio::atomic
#ifdef BOOST_SPINLOCK_USE_BOOST_ATOMIC
    using boost::memory_order;
    using boost::memory_order_relaxed;
    using boost::memory_order_consume;
    using boost::memory_order_acquire;
    using boost::memory_order_release;
    using boost::memory_order_acq_rel;
    using boost::memory_order_seq_cst;
#else
    using std::memory_order;
    using std::memory_order_relaxed;
    using std::memory_order_consume;
    using std::memory_order_acquire;
    using std::memory_order_release;
    using std::memory_order_acq_rel;
    using std::memory_order_seq_cst;
#endif
    // Map in a this_thread implementation
#ifdef BOOST_SPINLOCK_USE_BOOST_THREAD
    namespace this_thread=boost::this_thread;
    namespace chrono { using boost::chrono::milliseconds; }
#else
    namespace this_thread=std::this_thread;
    namespace chrono { using std::chrono::milliseconds; }
#endif


    /*! \struct lockable_ptr
     * \brief Lets you use a pointer to memory as a spinlock :)
     */
    template<typename T> struct lockable_ptr : atomic<T *>
    {
      lockable_ptr(T *v=nullptr) : atomic<T *>(v) { }
      //! Returns the memory pointer part of the atomic
      T *get() BOOST_NOEXCEPT_OR_NOTHROW
      {
        union
        {
          T *v;
          size_t n;
        } value;
        value.v=atomic<T *>::load();
        value.n&=~(size_t)1;
        return value.v;
      }
      //! Returns the memory pointer part of the atomic
      const T *get() const BOOST_NOEXCEPT_OR_NOTHROW
      {
        union
        {
          T *v;
          size_t n;
        } value;
        value.v=atomic<T *>::load();
        value.n&=~(size_t)1;
        return value.v;
      }
      T &operator*() BOOST_NOEXCEPT_OR_NOTHROW { return *get(); }
      const T &operator*() const BOOST_NOEXCEPT_OR_NOTHROW { return *get(); }
      T *operator->() BOOST_NOEXCEPT_OR_NOTHROW { return get(); }
      const T *operator->() const BOOST_NOEXCEPT_OR_NOTHROW { return get(); }
    };
    template<typename T> struct spinlockbase
    {
    protected:
      atomic<T> v;
    public:
      typedef T value_type;
      spinlockbase() BOOST_NOEXCEPT_OR_NOTHROW : v(0)
      { }
      spinlockbase(const spinlockbase &) = delete;
      //! Atomically move constructs
      spinlockbase(spinlockbase &&o) BOOST_NOEXCEPT_OR_NOTHROW
      {
        v.store(o.v.exchange(0, memory_order_acq_rel));
      }
      //! Returns the raw atomic
      T load(memory_order o=memory_order_seq_cst) BOOST_NOEXCEPT_OR_NOTHROW { return v.load(o); }
      //! Sets the raw atomic
      void store(T a, memory_order o=memory_order_seq_cst) BOOST_NOEXCEPT_OR_NOTHROW { v.store(a, o); }
      //! If atomic is zero, sets to 1 and returns true, else false.
      bool try_lock() BOOST_NOEXCEPT_OR_NOTHROW
      {
        if(v.load(memory_order_acquire)) // Avoid unnecessary cache line invalidation traffic
          return false;
        T expected=0;
        return v.compare_exchange_weak(expected, 1, memory_order_acquire, memory_order_consume);
      }
      //! If atomic equals expected, sets to 1 and returns true, else false with expected updated to actual value.
      bool try_lock(T &expected) BOOST_NOEXCEPT_OR_NOTHROW
      {
        T t;
        if((t=v.load(memory_order_acquire))) // Avoid unnecessary cache line invalidation traffic
        {
          expected=t;
          return false;
        }
        return v.compare_exchange_weak(expected, 1, memory_order_acquire, memory_order_consume);
      }
      //! Sets the atomic to zero
      void unlock() BOOST_NOEXCEPT_OR_NOTHROW
      {
        v.store(0, memory_order_release);
      }
      bool int_yield(size_t) BOOST_NOEXCEPT_OR_NOTHROW { return false; }
    };
    template<typename T> struct spinlockbase<lockable_ptr<T>>
    {
    private:
      lockable_ptr<T> v;
    public:
      typedef T *value_type;
      spinlockbase() BOOST_NOEXCEPT_OR_NOTHROW { }
      spinlockbase(const spinlockbase &) = delete;
      //! Atomically move constructs
      spinlockbase(spinlockbase &&o) BOOST_NOEXCEPT_OR_NOTHROW
      {
        v.store(o.v.exchange(nullptr, memory_order_acq_rel));
      }
      //! Returns the memory pointer part of the atomic
      T *get() BOOST_NOEXCEPT_OR_NOTHROW { return v.get(); }
      T *operator->() BOOST_NOEXCEPT_OR_NOTHROW { return get(); }
      //! Returns the raw atomic
      T *load(memory_order o=memory_order_seq_cst) BOOST_NOEXCEPT_OR_NOTHROW { return v.load(o); }
      //! Sets the memory pointer part of the atomic preserving lockedness
      void set(T *a) BOOST_NOEXCEPT_OR_NOTHROW
      {
        union
        {
          T *v;
          size_t n;
        } value;
        T *expected;
        do
        {
          value.v=v.load();
          expected=value.v;
          bool locked=value.n&1;
          value.v=a;
          if(locked) value.n|=1;
        } while(!v.compare_exchange_weak(expected, value.v));
      }
      //! Sets the raw atomic
      void store(T *a, memory_order o=memory_order_seq_cst) BOOST_NOEXCEPT_OR_NOTHROW { v.store(a, o); }
      bool try_lock() BOOST_NOEXCEPT_OR_NOTHROW
      {
        union
        {
          T *v;
          size_t n;
        } value;
        value.v=v.load();
        if(value.n&1) // Avoid unnecessary cache line invalidation traffic
          return false;
        T *expected=value.v;
        value.n|=1;
        return v.compare_exchange_weak(expected, value.v);
      }
      void unlock() BOOST_NOEXCEPT_OR_NOTHROW
      {
        union
        {
          T *v;
          size_t n;
        } value;
        value.v=v.load();
        assert(value.n&1);
        value.n&=~(size_t)1;
        v.store(value.v);
      }
      bool int_yield(size_t) BOOST_NOEXCEPT_OR_NOTHROW { return false; }
    };
    //! \brief How many spins to loop, optionally calling the SMT pause instruction on Intel
    template<size_t spins, bool use_pause=true> struct spins_to_loop
    {
      template<class parenttype> struct policy : parenttype
      {
        static BOOST_CONSTEXPR_OR_CONST size_t spins_to_loop=spins;
        policy() {}
        policy(const policy &) = delete;
        policy(policy &&o) BOOST_NOEXCEPT : parenttype(std::move(o)) { }
        bool int_yield(size_t n) BOOST_NOEXCEPT_OR_NOTHROW
        {
          if(parenttype::int_yield(n)) return true;
          if(n>=spins) return false;
          if(use_pause)
          {
#ifdef BOOST_SMT_PAUSE
            BOOST_SMT_PAUSE;
#endif
          }
          return true;
        }
      };
    };
    //! \brief How many spins to yield the current thread's timeslice
    template<size_t spins> struct spins_to_yield
    {
      template<class parenttype> struct policy : parenttype
      {
        static BOOST_CONSTEXPR_OR_CONST size_t spins_to_yield=spins;
        policy() {}
        policy(const policy &) = delete;
        policy(policy &&o) BOOST_NOEXCEPT : parenttype(std::move(o)) { }
        bool int_yield(size_t n) BOOST_NOEXCEPT_OR_NOTHROW
        {
          if(parenttype::int_yield(n)) return true;
          if(n>=spins) return false;
          this_thread::yield();
          return true;
        }
      };
    };
    //! \brief How many spins to sleep the current thread
    struct spins_to_sleep
    {
      template<class parenttype> struct policy : parenttype
      {
        policy() {}
        policy(const policy &) = delete;
        policy(policy &&o) BOOST_NOEXCEPT : parenttype(std::move(o)) { }
        bool int_yield(size_t n) BOOST_NOEXCEPT_OR_NOTHROW
        {
          if(parenttype::int_yield(n)) return true;
          this_thread::sleep_for(chrono::milliseconds(1));
          return true;
        }
      };
    };
    //! \brief A spin policy which does nothing
    struct null_spin_policy
    {
      template<class parenttype> struct policy : parenttype
      {
      };
    };
    /*! \class spinlock
    
    Meets the requirements of BasicLockable and Lockable. Also provides a get() and set() for the
    type used for the spin lock.

    So what's wrong with boost/smart_ptr/detail/spinlock.hpp then, and why
    reinvent the wheel?

    1. Non-configurable spin. AFIO needs a bigger spin than smart_ptr provides.

    2. AFIO is C++ 11, and therefore can implement this in pure C++ 11 atomics.

    3. I don't much care for doing writes during the spin. It generates an
    unnecessary amount of cache line invalidation traffic. Better to spin-read
    and only write when the read suggests you might have a chance.
    
    4. This spin lock can use a pointer to memory as the spin lock. See locked_ptr<T>.
    */
    template<typename T, template<class> class spinpolicy2=spins_to_loop<125>::policy, template<class> class spinpolicy3=spins_to_yield<250>::policy, template<class> class spinpolicy4=spins_to_sleep::policy> class spinlock : public spinpolicy4<spinpolicy3<spinpolicy2<spinlockbase<T>>>>
    {
      typedef spinpolicy4<spinpolicy3<spinpolicy2<spinlockbase<T>>>> parenttype;
    public:
      spinlock() { }
      spinlock(const spinlock &) = delete;
      spinlock(spinlock &&o) BOOST_NOEXCEPT : parenttype(std::move(o)) { }
      void lock() BOOST_NOEXCEPT_OR_NOTHROW
      {
        for(size_t n=0;; n++)
        {
          if(parenttype::try_lock())
            return;
          parenttype::int_yield(n);
        }
      }
      //! Locks if the atomic is not the supplied value, else returning false
      bool lock(T only_if_not_this) BOOST_NOEXCEPT_OR_NOTHROW
      {
        for(size_t n=0;; n++)
        {
          T expected=0;
          if(parenttype::try_lock(expected))
            return true;
          if(expected==only_if_not_this)
            return false;
          parenttype::int_yield(n);
        }
      }
    };

    //! \brief Determines if a lockable is locked. Type specialise this for performance if your lockable allows examination.
    template<class T> inline bool is_lockable_locked(T &lockable) BOOST_NOEXCEPT_OR_NOTHROW
    {
      if(lockable.try_lock())
      {
        lockable.unlock();
        return true;
      }
      return false;
    }
    // For when used with a spinlock
    template<class T, template<class> class spinpolicy2, template<class> class spinpolicy3, template<class> class spinpolicy4> inline T is_lockable_locked(spinlock<T, spinpolicy2, spinpolicy3, spinpolicy4> &lockable) BOOST_NOEXCEPT_OR_NOTHROW
    {
      return lockable.load(memory_order_acquire);
    }
    // For when used with a locked_ptr
    template<class T, template<class> class spinpolicy2, template<class> class spinpolicy3, template<class> class spinpolicy4> inline bool is_lockable_locked(spinlock<lockable_ptr<T>, spinpolicy2, spinpolicy3, spinpolicy4> &lockable) BOOST_NOEXCEPT_OR_NOTHROW
    {
      return ((size_t) lockable.load(memory_order_acquire))&1;
    }

#ifndef BOOST_BEGIN_TRANSACT_LOCK
#ifdef BOOST_HAVE_TRANSACTIONAL_MEMORY_COMPILER
#undef BOOST_USING_INTEL_TSX
#define BOOST_BEGIN_TRANSACT_LOCK(lockable) __transaction_relaxed { (void) boost::spinlock::is_lockable_locked(lockable); {
#define BOOST_BEGIN_TRANSACT_LOCK_ONLY_IF_NOT(lockable, only_if_not_this) __transaction_relaxed { if((only_if_not_this)!=boost::spinlock::is_lockable_locked(lockable)) {
#define BOOST_END_TRANSACT_LOCK(lockable) } }
#define BOOST_BEGIN_NESTED_TRANSACT_LOCK(N) __transaction_relaxed
#define BOOST_END_NESTED_TRANSACT_LOCK(N)
#endif // BOOST_BEGIN_TRANSACT_LOCK
#endif

#ifndef BOOST_BEGIN_TRANSACT_LOCK
#define BOOST_BEGIN_TRANSACT_LOCK(lockable) { std::lock_guard<decltype(lockable)> __tsx_transaction(lockable);
#define BOOST_BEGIN_TRANSACT_LOCK_ONLY_IF_NOT(lockable, only_if_not_this) if(lockable.lock(only_if_not_this)) { std::lock_guard<decltype(lockable)> __tsx_transaction(lockable, std::adopt_lock);
#define BOOST_END_TRANSACT_LOCK(lockable) }
#define BOOST_BEGIN_NESTED_TRANSACT_LOCK(N)
#define BOOST_END_NESTED_TRANSACT_LOCK(N)
#endif // BOOST_BEGIN_TRANSACT_LOCK

    /* \class concurrent_unordered_map
    \brief Provides an unordered_map which is thread safe and wait free to use and whose find, insert/emplace and erase functions are usually wait free.

    */
    template<class Key, class T, class Hash=std::hash<Key>, class Pred=std::equal_to<Key>, class Alloc=std::allocator<std::pair<const Key, T>>> class concurrent_unordered_map
    {
    public:
      typedef Key key_type;
      typedef T mapped_type;
      typedef std::pair<const key_type, mapped_type> value_type;
      typedef Hash hasher;
      typedef Pred key_equal;
      typedef Alloc allocator_type;

      typedef value_type& reference;
      typedef const value_type& const_reference;
      typedef value_type* pointer;
      typedef const value_type *const_pointer;
      typedef std::size_t size_type;
      typedef std::ptrdiff_t difference_type;
    private:
      spinlock<bool> _rehash_lock;
      struct non_zero_hasher
      {
        hasher _hasher;
        size_t operator()(const key_type &k) const BOOST_NOEXCEPT
        {
          size_t ret=_hasher(k);
          return ret==0 ? (size_t) -1 : ret;
        }
      } _hasher;
      key_equal _key_equal;
      float _max_load_factor;
      struct item_type
      {
        std::pair<key_type, mapped_type> p;
        size_t hash;  // never zero if in use
        item_type() : hash(0) { }
        item_type(size_t _hash, value_type &&_p) BOOST_NOEXCEPT : p(std::move(_p)), hash(_hash) { }
        template<class... Args> item_type(size_t _hash, Args &&... args) BOOST_NOEXCEPT : p(std::forward<Args>(args)...), hash(_hash) { }
        item_type(item_type &&o) BOOST_NOEXCEPT : p(std::move(o.p)), hash(o.hash) { o.hash=0; }
      };
      typedef typename allocator_type::template rebind<item_type>::other item_type_allocator_type;
      struct bucket_type_impl
      {
        spinlock<unsigned char> lock;  // = 2 if you need to reload the bucket list
        atomic<unsigned> count; // count is used items in there
        std::vector<item_type, item_type_allocator_type> items;
        bucket_type_impl() : count(0), items(0) { }
        bucket_type_impl(bucket_type_impl &&) BOOST_NOEXCEPT : count(0) { }
      };
#if 0
      struct bucket_type : bucket_type_impl
      {
        char pad[64-sizeof(bucket_type_impl)];
        bucket_type()
        {
          static_assert(sizeof(bucket_type)==64, "bucket_type is not 64 bytes long!");
        }
      };
#else
      typedef bucket_type_impl bucket_type;
#endif
      std::vector<bucket_type> _buckets;
      typename std::vector<bucket_type>::iterator _get_bucket(size_t k) BOOST_NOEXCEPT
      {
        //k ^= k + 0x9e3779b9 + (k<<6) + (k>>2); // really need to avoid sequential keys tapping the same cache line
        //k ^= k + 0x9e3779b9; // really need to avoid sequential keys tapping the same cache line
        size_type i=k % _buckets.size();
        return _buckets.begin()+i;
      }
      static float _calc_max_load_factor() BOOST_NOEXCEPT
      {
        return 1.0f;
#if 0
        // We are intentionally very tolerant to load factor, so set to
        // however many item_type's fit into 128 bytes
        float ret=128/sizeof(item_type);
        if(ret<1) ret=0;
        return ret;
#endif
      }
    public:
      class iterator : public std::iterator<std::forward_iterator_tag, value_type, difference_type, pointer, reference>
      {
        concurrent_unordered_map *_parent;
        bucket_type *_bucket_data; // used for sanity check that he hasn't rehashed
        typename std::vector<bucket_type>::iterator _itb;
        size_t _offset, _pending_incr; // used to avoid erase() doing a costly increment unless necessary
        friend class concurrent_unordered_map;
        iterator(const concurrent_unordered_map *parent) : _parent(const_cast<concurrent_unordered_map *>(parent)), _bucket_data(_parent->_buckets.data()), _itb(_parent->_buckets.begin()), _offset((size_t) -1), _pending_incr(1) { }
        iterator(const concurrent_unordered_map *parent, std::nullptr_t) : _parent(const_cast<concurrent_unordered_map *>(parent)), _bucket_data(_parent->_buckets.data()), _itb(_parent->_buckets.end()), _offset((size_t) -1), _pending_incr(0) { }
        void catch_up()
        {
          while(_pending_incr && _itb!=_parent->_buckets.end())
          {
            assert(_bucket_data==_parent->_buckets.data());
            if(_bucket_data!=_parent->_buckets.data())
              abort(); // stale iterator
            bucket_type &b=*_itb;
            BOOST_BEGIN_TRANSACT_LOCK_ONLY_IF_NOT(b.lock, 2)
            {
              for(_offset++; _offset<b.items.size(); _offset++)
                if(b.items[_offset].hash)
                  if(!(--_pending_incr)) break;
              if(_pending_incr)
              {
                while(_offset>=b.items.size() && _itb!=_parent->_buckets.end())
                {
                  ++_itb;
                  _offset=(size_t) -1;
                }
              }
            }
            BOOST_END_TRANSACT_LOCK(b.lock)
          }
        }
      public:
        iterator() : _parent(nullptr), _bucket_data(nullptr), _offset((size_t) -1), _pending_incr(0) { }
        bool operator!=(const iterator &o) const BOOST_NOEXCEPT{ return _itb!=o._itb || _offset!=o._offset || _pending_incr|=o._pending_incr; }
        bool operator==(const iterator &o) const BOOST_NOEXCEPT{ return _itb==o._itb && _offset==o._offset && _pending_incr==o._pending_incr; }
        iterator &operator++()
        {
          if(_itb==_parent->_buckets.end())
            return *this;
          ++_pending_incr;
          return *this;
        }
        iterator operator++(int) { iterator t(*this); operator++(); return t; }
        value_type &operator*() { catch_up(); assert(_itb!=_parent->_buckets.end()); if(_itb==_parent->_buckets.end()) abort(); return _itb->items[_offset]->p.get(); }
      };
      typedef iterator const_iterator;
      // local_iterator
      // const_local_iterator
      concurrent_unordered_map() : _max_load_factor(_calc_max_load_factor()), _buckets(13) { }
      concurrent_unordered_map(size_t n) : _max_load_factor(_calc_max_load_factor()), _buckets(n>0 ? n : 1) { }
      ~concurrent_unordered_map() { clear(); }
    private:
      // Awaiting implementation
      concurrent_unordered_map(const concurrent_unordered_map &);
      concurrent_unordered_map(concurrent_unordered_map &&);
      concurrent_unordered_map &operator=(const concurrent_unordered_map &);
      concurrent_unordered_map &operator=(concurrent_unordered_map &&);
    public:
      //! O(bucket count/item count/2)
      bool empty() const BOOST_NOEXCEPT
      {
        bool done;
        do
        {
          done=true;
          for(auto &b : _buckets)
          {
            // If the lock is other state we need to reload bucket list
            if(b.lock.load(memory_order_acquire)==2)
            {
              done=false;
              break;
            }
            if(b.count.load(memory_order_acquire))
              return false;
          }
        } while(!done);
        return true;
      }
      //! O(bucket count)
      size_type size() const BOOST_NOEXCEPT
      {
        size_type ret=0;
        bool done;
        do
        {
          done=true;
          for(auto &b : _buckets)
          {
            // If the lock is other state we need to reload bucket list
            if(b.lock.load(memory_order_acquire)==2)
            {
              done=false;
              break;
            }
            ret+=b.count.load(memory_order_acquire);
          }
        } while(!done);
        return ret;
      }
      size_type max_size() const BOOST_NOEXCEPT { return (size_type) -1; }
      iterator begin() BOOST_NOEXCEPT { return iterator(this); }
      const_iterator begin() const BOOST_NOEXCEPT { return const_iterator(this); }
      const_iterator cbegin() const BOOST_NOEXCEPT { return const_iterator(this); }
      iterator end() BOOST_NOEXCEPT { return iterator(this, nullptr); }
      const_iterator end() const BOOST_NOEXCEPT { return const_iterator(this, nullptr); }
      const_iterator cend() const BOOST_NOEXCEPT { return const_iterator(this, nullptr); }
      iterator find(const key_type &k)
      {
        iterator ret=end();
        size_t h=_hasher(k);
        bool done=false;
        do
        {
          auto itb=_get_bucket(h);
          bucket_type &b=*itb;
          size_t offset=0;
          if(b.count.load(memory_order_acquire))
          {
            // Should run completely concurrently with other finds
            BOOST_BEGIN_TRANSACT_LOCK_ONLY_IF_NOT(b.lock, 2)
            {
              for(;;)
              {
                for(; offset<b.items.size() && b.items[offset].hash!=h; offset++);
                if(offset==b.items.size())
                  break;
                if(_key_equal(k, b.items[offset].p.first))
                {
                  ret._itb=itb;
                  ret._offset=offset;
                  done=true;
                  break;
                }
                else offset++;
              }
              done=true;
            }
            BOOST_END_TRANSACT_LOCK(b.lock)
          }
        } while(!done);
        return ret;
      }
      //const_iterator find(const keytype &k) const;
      //mapped_type &at(const key_type &k);
      //const mapped_type &at(const key_type &k) const;
      //mapped_type &operator[](const key_type &k);
      //mapped_type &operator[](key_type &&k);
      //size_type count(const key_type &k) const;
      //std::pair<iterator, iterator> equal_range(const key_type &k);
      //std::pair<const_iterator, const_iterator> equal_range(const key_type &k) const;
      template<class KeyType, class ValueType> std::pair<iterator, bool> emplace(KeyType &&k, ValueType &&v)
      {
        std::pair<iterator, bool> ret(end(), true);
        size_t h=_hasher(k);
        bool done=false;
        do
        {
          auto itb=_get_bucket(h);
          bucket_type &b=*itb;
          size_t emptyidx=(size_t) -1;
          // First search for equivalents and empties. Start from the top to avoid cache line sharing with find
          if(b.count.load(memory_order_acquire))
          {
            BOOST_BEGIN_TRANSACT_LOCK_ONLY_IF_NOT(b.lock, 2)
              for(size_t offset=b.items.size()-1;;)
              {
                for(; offset!=(size_t) -1 && b.items[offset].hash!=h; offset--)
                  if(emptyidx==(size_t) -1 && !b.items[offset].hash)
                    emptyidx=offset;
                if(offset==(size_t) -1)
                  break;
                if(_key_equal(k, b.items[offset].p.first))
                {
                  ret.first._itb=itb;
                  ret.first._offset=offset;
                  ret.second=false;
                  done=true;
                  break;
                }
                else offset--;
              }
            BOOST_END_TRANSACT_LOCK(b.lock)
          }
          else if(!b.items.empty())
            emptyidx=0;

          if(!done)
          {
            // If the lock is other state we need to reload bucket list
            if(!b.lock.lock(2))
              continue;
            std::lock_guard<decltype(b.lock)> g(b.lock, std::adopt_lock); // Will abort all concurrency
            // If we earlier found an empty use that
            if(emptyidx!=(size_t) -1)
            {
              if(emptyidx<b.items.size() && !b.items[emptyidx].hash)
              {
                ret.first._itb=itb;
                ret.first._offset=emptyidx;
                b.items[emptyidx].p=std::make_pair(std::forward<KeyType>(k), std::forward<ValueType>(v));
                b.items[emptyidx].hash=h;
                b.count.fetch_add(1, memory_order_acquire);
                done=true;
              }
            }
            else
            {
              if(b.items.size()==b.items.capacity())
              {
                size_t newcapacity=b.items.capacity()*2;
                b.items.reserve(newcapacity ? newcapacity : 1);
              }
              ret.first._itb=itb;
              ret.first._offset=b.items.size();
              b.items.push_back(item_type(h, std::forward<KeyType>(k), std::forward<ValueType>(v)));
              b.count.fetch_add(1, memory_order_acquire);
              done=true;
            }
          }
        } while(!done);
        return ret;
      }
      //template<class... Args> iterator emplace_hint(const_iterator position, Args &&... args);
      std::pair<iterator, bool> insert(const value_type &v) { return emplace(v.first, v.second); }
      template<class P> std::pair<iterator, bool> insert(P &&v) { return emplace(std::forward<typename P::first_type>(v.first), std::forward<typename P::second_type>(v.second)); }
      //iterator insert(const_iterator hint, const value_type &v);
      //template<class P> iterator insert(const_iterator hint, P &&v);
      //template<class InputIterator> void insert(InputIterator first, InputIterator last);
      //void insert(std::initializer_list<value_type> i);
      iterator erase(const_iterator it)
      {
        iterator ret(end());
        //assert(it!=end());
        if(it==ret) return ret;
        bucket_type &b=*it._itb;
        std::pair<const key_type, mapped_type> former;
        {
          // If the lock is other state we need to reload bucket list
          if(!b.lock.lock(2))
            abort();
          std::lock_guard<decltype(b.lock)> g(b.lock, std::adopt_lock); // Will abort all concurrency
          if(it._offset<b.items.size() && b.items[it._offset].hash)
          {
            former=std::move(b.items[it._offset].p); // Move into former, hopefully avoiding a free()
            b.items[it._offset].hash=0;
            if(it._offset==b.items.size()-1)
            {
              // Shrink table to minimum
              while(!b.items.empty() && !b.items.back().hash)
                b.items.pop_back(); // Will abort all concurrency
            }
            b.count.fetch_sub(1, memory_order_acquire);
            ret=it;
            ++ret;
          }
        }
        return ret;
      }
      size_type erase(const key_type &k)
      {
        size_type ret=0;
        size_t h=_hasher(k);
        bool done=false;
        std::pair<key_type, mapped_type> former;
        do
        {
          auto itb=_get_bucket(h);
          bucket_type &b=*itb;
          size_t offset=0;
          if(b.count.load(memory_order_acquire))
          {
            // If the lock is other state we need to reload bucket list
            if(!b.lock.lock(2))
              continue;
            std::lock_guard<decltype(b.lock)> g(b.lock, std::adopt_lock); // Will abort all concurrency
            {
              for(;;)
              {
                for(; offset<b.items.size() && b.items[offset].hash!=h; offset++);
                if(offset==b.items.size())
                  break;
                if(_key_equal(k, b.items[offset].p.first))
                {
                  former=std::move(b.items[offset].p); // Move into former, hopefully avoiding a free()
                  b.items[offset].hash=0;
                  if(offset==b.items.size()-1)
                  {
                    // Shrink table to minimum
                    while(!b.items.empty() && !b.items.back().hash)
                      b.items.pop_back(); // Will abort all concurrency
                  }
                  b.count.fetch_sub(1, memory_order_acquire);
                  ++ret;
                  done=true;
                  break;
                }
                else offset++;
              }
              done=true;
            }
          }
        } while(!done);
        return ret;
      }
      // iterator erase(const_iterator first, const_iterator last);
      void clear() BOOST_NOEXCEPT
      {
        bool done;
        do
        {
          done=true;
          for(auto &b : _buckets)
          {
            // If the lock is other state we need to reload bucket list
            if(!b.lock.lock(2))
            {
              done=false;
              break;
            }
            std::lock_guard<decltype(b.lock)> g(b.lock, std::adopt_lock); // Will abort all concurrency
            b.items.clear();
          }
        } while(!done);
      }
      void swap(concurrent_unordered_map &o)
      {
        std::lock(_rehash_lock, o._rehash_lock);
        std::lock_guard<decltype(_rehash_lock)> g1(_rehash_lock, std::adopt_lock);
        std::lock_guard<decltype(o._rehash_lock)> g2(o._rehash_lock, std::adopt_lock);
        std::swap(_hasher, o._hasher);
        std::swap(_key_equal, o._key_equal);
        _buckets.swap(o._buckets);
      }
      size_type bucket_count() const BOOST_NOEXCEPT { return _buckets.size(); }
      size_type max_bucket_count() const BOOST_NOEXCEPT { return _buckets.max_size(); }
      size_type bucket_size(size_type n) const
      {
        bucket_type &b=_buckets[n];
        return b.items.count.load(memory_order_acquire);
      }
      size_type bucket(const key_type &k) const
      {
        return _hasher(k) % _buckets.size();
      }
      float load_factor() const BOOST_NOEXCEPT { return (float) size()/bucket_count(); }
      float max_load_factor() const BOOST_NOEXCEPT { return _max_load_factor; }
      void max_load_factor(float m) { _max_load_factor=m; }
      void rehash(size_type n)
      {
        // TODO: Lots more to do here
        _buckets.resize(n);
      }
      void reserve(size_type n)
      {
        rehash((size_type)(n/_max_load_factor));
      }
      hasher hash_function() const { return _hasher._hasher; }
      key_equal key_eq() const { return _key_equal; }
      // allocator_type get_allocator() const BOOST_NOEXCEPT;
      void dump_buckets(std::ostream &s) const
      {
        for(size_t n=0; n<_buckets.size(); n++)
        {
          s << "Bucket " << n << ": size=" << _buckets[n].items.size() << " count=" << _buckets[n].count << std::endl;
        }
      }
    }; // concurrent_unordered_map
  }
}

namespace std
{
  template <class Key, class T, class Hash, class Pred, class Alloc>
  void swap(boost::spinlock::concurrent_unordered_map<Key, T, Hash, Pred, Alloc>& lhs, boost::spinlock::concurrent_unordered_map<Key, T, Hash, Pred, Alloc>& rhs)
  {
    lhs.swap(rhs);
  }
}
#endif // BOOST_SPINLOCK_HPP
