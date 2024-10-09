# Blaze

Mod for Geometry Dash that significantly speeds up loading times.

todo readme

feats:

* Up to 500-700% faster savefile saving (at the cost of a few extra kilobytes)
* 30-40% faster savefile loading
* ~200-400% faster game resource loading
* 5-10% faster CCString creation
* Faster loading of textures from any mod (faster image decoding library & optional image caching)
* No custom formats - mod can be safely uninstalled and your savefile will still load
* Parallelized audio engine loading (experimental, disabled by default, unavailable on Android)
* Parallelized GLFW setup (experimental, disabled by default)

Overall, game launches can be 2-3x faster, on my machine on average the speed goes up from ~2.9s to ~1.4s (38mb save, i5-13600k, no megahack, without globed asset preloading)

Note: all those numbers are on Windows. On other operating systems, the performance increase might not be that big.


## Credit

* [matcool](https://github.com/matcool) for [Fast Format](https://github.com/matcool/geode-mods/blob/main/fast-format/main.cpp) (i took the entire mod)
* [cgytrus](https://github.com/cgytrus) for [Algebra Dash](https://github.com/cgytrus/AlgebraDash) (Tracy integration taken from it, the rest of the optimizations were implemented by myself, even if the ideas are similar to ones in Config's mod)
