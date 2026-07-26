# blaze

TODO:

* Use mimalloc, some slop for reference:

use mi_any_heap_contains in free/realloc hooks,
intercept: malloc, calloc, realloc, free, _msize, _aligned_malloc, _aligned_free, _aligned_realloc
option for hardening, zeroing