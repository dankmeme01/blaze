# 1.2.0

- Parallelize `FMODAudioEngine` setup, making game loading quite a bit faster.
- Reorder some code for slight performance improvements

# 1.1.0

- Add custom implementations for saving & loading savefiles
- Speed up loading times by doing more multithreading

# 1.0.0

- Initial version of the mod
- Makes the game use a faster PNG load library
- Cache resources (compressed in a faster format) in a separate directory, so subsequent launches will be faster
- Load game resources in parallel from multiple threads, significantly speeding up load times
