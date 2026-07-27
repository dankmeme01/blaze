# blaze

TODO:

* Use mimalloc, some slop for reference:

use mi_any_heap_contains in free/realloc hooks,
intercept: malloc, calloc, realloc, free, _msize, _aligned_malloc, _aligned_free, _aligned_realloc
option for hardening, zeroing

* `DS_Dictionary::loadRootSubDictFromCompressedFile` is called early in the game two times (6aac0 <- 17b4f0 <- 825d0 <- ccapplication), calls slow `getFileData`, `decompressString2` and pugixml routines
* `DS_Dictionary::getDictForKey` potentially slow?

Occasional
13:13:06.805 DEBUG [Main] [Async Load API]: Task 6374240985019527482 finished after 7.066ms, state: Failed
13:13:06.805 ERROR [Main] [Blaze]: Failed to load spritesheet GJ_GameSheetEditor: Failed to load spritesheet: Texture load failed: failed to decode image: initWithImageData failed
13:13:06.805 DEBUG [Main] [Async Load API]: Task 16349595361424751938 finished after 7.124ms, state: Failed
13:13:06.805 ERROR [Main] [Blaze]: Failed to load spritesheet GJ_GameSheet02: Failed to load spritesheet: Texture load failed: failed to decode image: initWithImageData failed
