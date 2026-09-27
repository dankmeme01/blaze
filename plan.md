# blaze

TODO:

* Use mimalloc, some slop for reference:

use mi_any_heap_contains in free/realloc hooks,
intercept: malloc, calloc, realloc, free, _msize, _aligned_malloc, _aligned_free, _aligned_realloc
option for hardening, zeroing

* `DS_Dictionary::getDictForKey` potentially slow?
* texture packs are a concern: idk if we should hook addSpriteFramesWithFile, and then there is  https://github.com/geode-sdk/textureldr/pull/76/changes

/home/dankpc/gd/instances/2.2081/geode/logs/Geode 2026-08-14 11.13.28.log
Occasional
13:13:06.805 DEBUG [Main] [Async Load API]: Task 6374240985019527482 finished after 7.066ms, state: Failed
13:13:06.805 ERROR [Main] [Blaze]: Failed to load spritesheet GJ_GameSheetEditor: Failed to load spritesheet: Texture load failed: failed to decode image: initWithImageData failed
13:13:06.805 DEBUG [Main] [Async Load API]: Task 16349595361424751938 finished after 7.124ms, state: Failed
13:13:06.805 ERROR [Main] [Blaze]: Failed to load spritesheet GJ_GameSheet02: Failed to load spritesheet: Texture load failed: failed to decode image: initWithImageData failed

* optimize orb effects: https://discord.com/channels/911701438269386882/911702535373475870/1548047013432659989