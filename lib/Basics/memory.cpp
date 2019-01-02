////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2016 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Dr. Frank Celler
////////////////////////////////////////////////////////////////////////////////

#include "Basics/Common.h"

#ifdef ARANGODB_ENABLE_FAILURE_TESTS
#include <new>
#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief threshold for producing malloc warnings
///
/// this is only active if zone debug is enabled. Any malloc operations that
/// try to alloc more memory than the threshold will be logged so we can check
/// why so much memory is needed
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief wrapper for malloc calls
////////////////////////////////////////////////////////////////////////////////

#define BuiltInMalloc(n) malloc(n)
#define BuiltInRealloc(ptr, n) realloc(ptr, n)

#ifdef ARANGODB_ENABLE_FAILURE_TESTS
#define MALLOC_WRAPPER(n) FailMalloc(n)
#define REALLOC_WRAPPER(ptr, n) FailRealloc(ptr, n)
#else
#define MALLOC_WRAPPER(n) BuiltInMalloc(n)
#define REALLOC_WRAPPER(ptr, n) BuiltInRealloc(ptr, n)
#endif

/// @brief configuration parameters for memory error tests
#ifdef ARANGODB_ENABLE_FAILURE_TESTS
static size_t FailMinSize = 0;
static double FailProbability = 0.0;
static double FailStartStamp = 0.0;
static thread_local int AllowMemoryFailures = -1;
#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief checks the size of the memory that is requested
/// prints a warning if size is above a threshold
////////////////////////////////////////////////////////////////////////////////

#ifdef ARANGODB_ENABLE_FAILURE_TESTS
/// @brief timestamp for failing malloc
static inline double CurrentTimeStamp(void) {
  struct timeval tv;
  gettimeofday(&tv, 0);

  return (tv.tv_sec) + (tv.tv_usec / 1000000.0);
}
#endif

#ifdef ARANGODB_ENABLE_FAILURE_TESTS
/// @brief whether or not a malloc operation should intentionally fail
static bool ShouldFail(size_t n) {
  if (FailMinSize > 0 && FailMinSize > n) {
    return false;
  }

  if (FailProbability == 0.0) {
    return false;
  }

  if (AllowMemoryFailures != 1) {
    return false;
  }

  if (FailStartStamp > 0.0 && CurrentTimeStamp() < FailStartStamp) {
    return false;
  }

  if (FailProbability < 1.0 && FailProbability * RAND_MAX < rand()) {
    return false;
  }

  return true;
}
#endif

#ifdef ARANGODB_ENABLE_FAILURE_TESTS
/// @brief intentionally failing malloc - used for failure tests
static void* FailMalloc(size_t n) {
  // we can fail, so let's check whether we should fail intentionally...
  if (ShouldFail(n)) {
    // intentionally return NULL
    errno = ENOMEM;
    return nullptr;
  }

  return BuiltInMalloc(n);
}
#endif

#ifdef ARANGODB_ENABLE_FAILURE_TESTS
/// @brief intentionally failing realloc - used for failure tests
static void* FailRealloc(void* old, size_t n) {
  // we can fail, so let's check whether we should fail intentionally...
  if (ShouldFail(n)) {
    // intentionally return NULL
    errno = ENOMEM;
    return nullptr;
  }

  return BuiltInRealloc(old, n);
}
#endif

#ifdef ARANGODB_ENABLE_FAILURE_TESTS
/// @brief initialize failing malloc
static void InitFailMalloc(void) {
  // get failure probability
  char* value = getenv("ARANGODB_FAILMALLOC_PROBABILITY");

  if (value != nullptr) {
    double v = strtod(value, nullptr);
    if (v >= 0.0 && v <= 1.0) {
      FailProbability = v;
    }
  }

  // get startup delay
  value = getenv("ARANGODB_FAILMALLOC_DELAY");

  if (value != nullptr) {
    double v = strtod(value, nullptr);
    if (v > 0.0) {
      FailStartStamp = CurrentTimeStamp() + v;
    }
  }

  // get minimum size for failures
  value = getenv("ARANGODB_FAILMALLOC_MINSIZE");

  if (value != nullptr) {
    unsigned long long v = strtoull(value, nullptr, 10);
    if (v > 0) {
      FailMinSize = (size_t)v;
    }
  }
}
#endif

#ifdef ARANGODB_ENABLE_FAILURE_TESTS
/// @brief overloaded, intentionally failing operator new
void* operator new(size_t size) {
  if (ShouldFail(size)) {
    throw std::bad_alloc();
  }

  void* pointer = malloc(size);

  if (pointer == nullptr) {
    throw std::bad_alloc();
  }

  return pointer;
}

/// @brief overloaded, intentionally failing operator new, non-throwing
void* operator new(size_t size, std::nothrow_t const&) noexcept {
  if (ShouldFail(size)) {
    return nullptr;
  }
  return malloc(size);
}

/// @brief overloaded, intentionally failing operator new[]
void* operator new[](size_t size) {
  if (ShouldFail(size)) {
    throw std::bad_alloc();
  }

  void* pointer = malloc(size);

  if (pointer == nullptr) {
    throw std::bad_alloc();
  }

  return pointer;
}

/// @brief overloaded, intentionally failing operator new[], non-throwing
void* operator new[](size_t size, std::nothrow_t const&) noexcept {
  if (ShouldFail(size)) {
    return nullptr;
  }
  return malloc(size);
}

/// @brief overloaded operator delete
void operator delete(void* pointer) noexcept {
  if (pointer) {
    free(pointer);
  }
}

/// @brief overloaded operator delete
void operator delete(void* pointer, std::nothrow_t const&)noexcept {
  if (pointer) {
    free(pointer);
  }
}

/// @brief overloaded operator delete
void operator delete[](void* pointer) noexcept {
  if (pointer) {
    free(pointer);
  }
}

/// @brief overloaded operator delete
void operator delete[](void* pointer, std::nothrow_t const&) noexcept {
  if (pointer) {
    free(pointer);
  }
}
#endif

/// @brief basic memory management for allocate
void* TRI_Allocate(size_t n) {
  void* m = static_cast<char*>(MALLOC_WRAPPER(n));

  if (m == nullptr) {
    TRI_set_errno(TRI_ERROR_OUT_OF_MEMORY);
    return nullptr;
  }

#ifdef ARANGODB_ENABLE_MAINTAINER_MODE
  // prefill with 0xA5 (magic value, same as Valgrind will use)
  memset(m, 0xA5, n);
#endif

  return m;
}

/// @brief basic memory management for reallocate
void* TRI_Reallocate(void* m, size_t n) {
  if (m == nullptr) {
    return TRI_Allocate(n);
  }

  void* p = REALLOC_WRAPPER(m, n);

  if (p == nullptr) {
    TRI_set_errno(TRI_ERROR_OUT_OF_MEMORY);
    return nullptr;
  }

  return p;
}

/// @brief basic memory management for deallocate
void TRI_Free(void* m) {
#ifdef ARANGODB_ENABLE_MAINTAINER_MODE
  if (m == nullptr) {
    fprintf(stderr, "freeing nil ptr\n");
    // crash intentionally
    TRI_ASSERT(false);
  }
#endif

  free(m);
}

#ifdef ARANGODB_ENABLE_FAILURE_TESTS
void TRI_AllowMemoryFailures() { AllowMemoryFailures = 1; }
#endif

#ifdef ARANGODB_ENABLE_FAILURE_TESTS
void TRI_DisallowMemoryFailures() { AllowMemoryFailures = 0; }
#endif

/// @brief securely zero memory
void TRI_ZeroMemory(void* m, size_t size) {
#ifdef _WIN32
  SecureZeroMemory(m, size);
#else
  // use volatile in order to not optimize away the zeroing
  volatile char* ptr = reinterpret_cast<volatile char*>(m);
  volatile char* end = ptr + size;
  while (ptr < end) {
    *ptr++ = '\0';
  }
#endif
}

/// @brief initialize memory subsystem
void TRI_InitializeMemory() {
#ifdef ARANGODB_ENABLE_FAILURE_TESTS
  InitFailMalloc();
#endif
}
