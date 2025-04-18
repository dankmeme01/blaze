# Blaze

Geometry Dash mod aimed at improving performance. Currently it mostly touches game load and save parts, and doesn't help in-game performance, but that is also planned in the future.

Feats:

* Up to 500-700% faster savefile saving - no more hangs when closing the game
* 30-40% faster savefile loading
* Multithreaded game resource loading, up to 200-400% faster
* 5-10% faster CCString creation
* Faster loading of textures from any mod (faster image decoding library & optional image caching)
* No custom formats - mod can be safely uninstalled and your savefile will still load
* Parallelized audio engine loading (experimental, disabled by default, unavailable on Android)
* Parallelized GLFW setup (experimental, disabled by default)

Overall, game launches can be 2-3x faster, on my machine on average the speed goes up from ~2.9s to ~1.4s (38mb save, i5-13600k, no megahack, without globed asset preloading)

Note: all those numbers are on Windows. On other operating systems, the performance increase might not be that big.

For developers:

* Hooks `CCFileUtils::getFileData` to make it thread-safe (without Blaze, using this function from multiple threads is UB on Android!)
* Hooks most image APIs (`CCImage` create and init functions) as well, to use a faster PNG decoder
* Hooks `base64Decode` function, making it faster and patching the UB

## Credit

* [matcool](https://github.com/matcool) for [Fast Format](https://github.com/matcool/geode-mods/blob/main/fast-format/main.cpp) (i took the entire mod)
* [cgytrus](https://github.com/cgytrus) for [Algebra Dash](https://github.com/cgytrus/AlgebraDash) (Tracy integration taken from it)
* [prevter](https://github.com/Prevter) for helping with GLFW symbol sigscanning

## Used libraries

Thanks to the following libraries for making this mod ever so slightly faster :)

* [asp](https://github.com/dankmeme01/asp2)
* [fast_float](https://github.com/fastfloat/fast_float)
* [fast_vector](https://github.com/sigerror/fast-vector)
* [fpng](https://github.com/richgel999/fpng)
* [libdeflate](https://github.com/ebiggers/libdeflate)
* [miniz](https://github.com/richgel999/miniz)
* [pugixml](https://github.com/zeux/pugixml)
* [simdutf](https://github.com/simdutf/simdutf)
* [sinaps](https://github.com/Prevter/sinaps/)
* [spng](https://github.com/randy408/libspng)
