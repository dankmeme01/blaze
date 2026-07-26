# blaze

TODO:

* Use mimalloc, some slop for reference:

use mi_any_heap_contains in free/realloc hooks,
intercept: malloc, calloc, realloc, free, _msize, _aligned_malloc, _aligned_free, _aligned_realloc
option for hardening, zeroing

* `DS_Dictionary::loadRootSubDictFromCompressedFile` is called early in the game two times (6aac0 <- 17b4f0 <- 825d0 <- ccapplication), calls slow `getFileData`, `decompressString2` and pugixml routines
* `DS_Dictionary::getDictForKey` potentially slow?