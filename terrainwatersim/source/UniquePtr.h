#pragma once

#include <Foundation/Memory/AllocatorWrapper.h>

// predeclarations 
template <typename T, typename AllocatorWrapper>
class ezUniquePtr;
namespace ezInternal
{
  template<typename AllocatorWrapper, typename T>
  ezUniquePtr<T,AllocatorWrapper> CreateUniquePtr(T* pData);
};

/// \brief Simple unique smartpointer.
/// 
/// Data creation with an ezAllocator is enforced. Use EZ_NEW_UNIQUE to create new ezUniquePtr.
template <typename T, typename AllocatorWrapper = ezDefaultAllocatorWrapper>
class ezUniquePtr
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezUniquePtr);

public:
  /// \brief Constructor for empty unique ptr
  EZ_FORCE_INLINE ezUniquePtr() : m_pData(NULL)
  {
  }

  /// \brief Constructor from a temporary ezUniquePtr
  ///
  /// Will set the copy's pointer to NULL.
  EZ_FORCE_INLINE ezUniquePtr(ezUniquePtr<T, AllocatorWrapper>&& tempPtr) : m_pData(tempPtr.m_pData)
  {
    tempPtr.m_pData = NULL; // otherwise the data would be deleted
  }

  /// \brief Destroys the internal pointer
  EZ_FORCE_INLINE ~ezUniquePtr() // [tested]
  {
    EZ_DELETE(AllocatorWrapper::GetAllocator(), m_pData);
  }

  /// \brief Read access to the pointer via the arrow operator.
  EZ_FORCE_INLINE const T* operator ->() const // [tested]  
  {
    EZ_ASSERT(this->m_pData, "Try to access NULL.");
    return this->m_pData;
  }

  /// \brief Access to the pointer via the arrow operator.
  EZ_FORCE_INLINE T* operator ->()  // [tested]
  {
    EZ_ASSERT(this->m_pData, "Try to access NULL.");
    return this->m_pData;
  }

  /// \brief Performs a NULL check
  EZ_FORCE_INLINE operator bool() // [tested]
  {
    return this->m_pData != NULL;
  }

  /// \brief Direct read access to the Pointer.
  EZ_FORCE_INLINE const T* Get() const  // [tested]
  {
    return this->m_pData;
  }

  /// \brief Direct access to the Pointer.
  ///
  /// Use with care! You should really know what you're doing. Do not delete this pointer!
  EZ_FORCE_INLINE T* Get()  // [tested]
  {
    return this->m_pData;
  }

  /// \brief Swaps contents with another ezUniquePtr
  EZ_FORCE_INLINE void Swap(ezUniquePtr<T, AllocatorWrapper>& pOther) // [tested]
  {
    T* pTemp = this->m_pData;
    this->m_pData = pOther.m_pData;
    pOther.m_pData = pTemp;
  }
  /// \brief Swaps contents with another ezUniquePtr, rvalue ref
  EZ_FORCE_INLINE void Swap(ezUniquePtr<T, AllocatorWrapper>&& pOther)
  {
    EZ_DEFAULT_DELETE(this->m_pData);
    this->m_pData = pOther.m_pData;
    pOther.m_pData = NULL;
  }

  /// \brief Assignment with temporary unique ptr
  EZ_FORCE_INLINE void operator = (ezUniquePtr<T, AllocatorWrapper>&& tempPtr)
  {
    this->m_pData = tempPtr.m_pData;
    tempPtr.m_pData = NULL;
  }


  /// \brief Deletes data and sets pointer to NULL.
  EZ_FORCE_INLINE void Release()  // [tested]
  {
    EZ_DELETE(AllocatorWrapper::GetAllocator(), m_pData);
  }

private:
  /// \brief Internal creation constructor
  EZ_FORCE_INLINE ezUniquePtr(T* pData) :
     m_pData(pData)
  {}

  // creation function can access creation constructor
  template<typename AllocatorWrapper, typename T>
  friend ezUniquePtr<T, AllocatorWrapper> ezInternal::CreateUniquePtr(T*);

  /// The data pointer
  T* m_pData;
};

namespace ezInternal
{
  /// \brief Creates a new ezUniquePtr. Internal use only.
  template<typename AllocatorWrapper, typename T>
  ezUniquePtr<T,AllocatorWrapper> CreateUniquePtr(T* pData)
  {
    return ezUniquePtr<T,AllocatorWrapper>(pData);  // using std::move here would make things worse most likely: see http://thbecker.net/articles/rvalue_references/section_06.html
  }
}

/// \brief Creates a new ezUniquePtr instance of a given type using the given allocator-wrapper.
#define EZ_NEW_UNIQUE(AllocatorWrapper, Type, ...) \
  ezInternal::CreateUniquePtr<AllocatorWrapper, Type>( EZ_NEW(AllocatorWrapper::GetAllocator(), Type)(__VA_ARGS__) )

/// \brief Creates a new ezUniquePtr instance of a given type using the default Allocator
#define EZ_DEFAULT_NEW_UNIQUE(Type, ...) \
  EZ_NEW_UNIQUE(ezDefaultAllocatorWrapper, Type, __VA_ARGS__)