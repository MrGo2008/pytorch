#pragma once

#include <ATen/Allocator.h>
#include <ATen/Error.h>
#include <ATen/Generator.h>
#include <ATen/Registry.h>

#include <cstddef>
#include <functional>
#include <memory>

// Forward declare these CUDA types here to avoid including CUDA headers in
// ATen headers, which would make ATen always require CUDA to build.
struct THCState;
struct CUstream_st;
typedef struct CUstream_st* cudaStream_t;
struct cudaDeviceProp;

#ifndef __HIP_PLATFORM_HCC__
// pyHIPIFY rewrites this as:
//
//    struct cusparseContext;
//    typedef struct cusparseContext *hipsparseHandle_t;
//
// however, this forward declaration is wrong
// the way that the HIP headers define hipsparseHandle_t is
//
//    typedef cusparseHandle_t hipsparseHandle_t
//
// so the rewrite is wrong.
struct cusparseContext;
typedef struct cusparseContext *cusparseHandle_t;
#endif

namespace at {
class Context;
}

// NB: Class must live in `at` due to limitations of Registry.h.
namespace at {

// The CUDAHooksInterface is an omnibus interface for any CUDA functionality
// which we may want to call into from CPU code (and thus must be dynamically
// dispatched, to allow for separate compilation of CUDA code).  How do I
// decide if a function should live in this class?  There are two tests:
//
//  1. Does the *implementation* of this function require linking against
//     CUDA libraries?
//
//  2. Is this function *called* from non-CUDA ATen code?
//
// (2) should filter out many ostensible use-cases, since many times a CUDA
// function provided by ATen is only really ever used by actual CUDA code.
//
// TODO: Consider putting the stub definitions in another class, so that one
// never forgets to implement each virtual function in the real implementation
// in CUDAHooks.  This probably doesn't buy us much though.
struct AT_API CUDAHooksInterface {
  // This should never actually be implemented, but it is used to
  // squelch -Werror=non-virtual-dtor
  virtual ~CUDAHooksInterface() {}

  // Initialize THCState and, transitively, the CUDA state
  virtual std::unique_ptr<THCState, void (*)(THCState*)> initCUDA() const {
    AT_ERROR("cannot initialize CUDA without ATen_cuda library");
  }

  virtual std::unique_ptr<Generator> initCUDAGenerator(Context*) const {
    AT_ERROR("cannot initialize CUDA generator without ATen_cuda library");
  }

  virtual bool hasCUDA() const {
    return false;
  }

  virtual bool hasCuDNN() const {
    return false;
  }

  virtual cudaStream_t getCurrentCUDAStream(THCState*) const {
    AT_ERROR("cannot getCurrentCUDAStream() without ATen_cuda library");
  }

#ifndef __HIP_PLATFORM_HCC__
  virtual cusparseHandle_t getCurrentCUDASparseHandle(THCState*) const {
    AT_ERROR("cannot getCurrentCUDASparseHandle() without ATen_cuda library");
  }
#endif

  virtual cudaStream_t getCurrentCUDAStreamOnDevice(THCState*, int64_t device)
      const {
    AT_ERROR("cannot getCurrentCUDAStream() without ATen_cuda library");
  }

  virtual struct cudaDeviceProp* getCurrentDeviceProperties(THCState*) const {
    AT_ERROR("cannot getCurrentDeviceProperties() without ATen_cuda library");
  }

  virtual struct cudaDeviceProp* getDeviceProperties(THCState*, int device)
      const {
    AT_ERROR("cannot getDeviceProperties() without ATen_cuda library");
  }

  virtual int64_t current_device() const {
    return -1;
  }

  virtual std::unique_ptr<Allocator> newPinnedMemoryAllocator() const {
    AT_ERROR("pinned memory requires CUDA");
  }

  virtual void registerCUDATypes(Context*) const {
    AT_ERROR("cannot registerCUDATypes() without ATen_cuda library");
  }

  virtual bool compiledWithCuDNN() const {
    return false;
  }

  virtual bool supportsDilatedConvolutionWithCuDNN() const {
    return false;
  }

  virtual long versionCuDNN() const {
    AT_ERROR("cannot query cuDNN version without ATen_cuda library");
  }

  virtual double batchnormMinEpsilonCuDNN() const {
    AT_ERROR(
        "cannot query batchnormMinEpsilonCuDNN() without ATen_cuda library");
  }

  virtual int64_t cuFFTGetPlanCacheMaxSize() const {
    AT_ERROR("cannot access cuFFT plan cache without ATen_cuda library");
  }

  virtual void cuFFTSetPlanCacheMaxSize(int64_t max_size) const {
    AT_ERROR("cannot access cuFFT plan cache without ATen_cuda library");
  }

  virtual int64_t cuFFTGetPlanCacheSize() const {
    AT_ERROR("cannot access cuFFT plan cache without ATen_cuda library");
  }

  virtual void cuFFTClearPlanCache() const {
    AT_ERROR("cannot access cuFFT plan cache without ATen_cuda library");
  }

  virtual int getNumGPUs() const {
    return 0;
  }
};

// NB: dummy argument to suppress "ISO C++11 requires at least one argument
// for the "..." in a variadic macro"
struct AT_API CUDAHooksArgs {};

AT_DECLARE_REGISTRY(CUDAHooksRegistry, CUDAHooksInterface, CUDAHooksArgs)
#define REGISTER_CUDA_HOOKS(clsname) \
  AT_REGISTER_CLASS(CUDAHooksRegistry, clsname, clsname)

namespace detail {
AT_API const CUDAHooksInterface& getCUDAHooks();

/// This class exists to let us access `cudaSetDevice`, `cudaGetDevice` and CUDA
/// error handling functions, when CUDA is available. These functions will first
/// default to no-ops. When the `ATen` GPU library is loaded, they will be set to
/// the `cudaSetDevice`/`cudaGetDevice` functions. This allows us to access them
/// with only a single pointer indirection, while virtual dispatch would require
/// two (one for the virtual call, one for `cudaSetDevice`/`cudaGetDevice`).
struct AT_API DynamicCUDAInterface {
  static void (*set_device)(int32_t);
  static void (*get_device)(int32_t*);
  static void (*unchecked_set_device)(int32_t);
};
} // namespace detail
} // namespace at
