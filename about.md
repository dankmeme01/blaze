# Blaze

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
