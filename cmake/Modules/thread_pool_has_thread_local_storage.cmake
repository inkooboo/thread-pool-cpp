# Check for true C++11 thread_local support
function(thread_pool_has_thread_local_storage varName)
  include(CheckCXXSourceCompiles)
  check_cxx_source_compiles("static thread_local int tls; int main(void) { return 0; }"
    THREAD_POOL_HAS_THREAD_LOCAL_STORAGE
    )
  set(${varName} ${THREAD_POOL_HAS_THREAD_LOCAL_STORAGE} PARENT_SCOPE)
endfunction(thread_pool_has_thread_local_storage)
