# Benchmarks

This file contains some benchmarks that were ran to compare performance of vanilla game without Blaze hooks, and with Blaze hooks applied. Some of the things here have their source code in [`src/benchmarks/](./src/benchmarks/).

Unless specified otherwise, each benchmark is performed on the following machine:
* OS: Linux x64 + Wine v11.19
* CPU: Intel Core i5-13600K 6+8c/20t 3.5GHz
* RAM: 32 GB DDR5 (5600 MT/s)

## `fullPathForFilename`

Source code: [fpff.cpp](./src/benchmarks/fpff.cpp)

Three kind of benchmarks are done, all have `ignoreSuffix = false`:
* Calling with a bogus path repeatedly: `icons/fake/ass/png`
* Calling with a real game sheet that already was loaded, repeatedly
* Calling with every icon, most of which have not been loaded yet (Globed disabled)

There are also three path resolution methods benchmarked:
1. Stock GD/Cocos behavior: calling `CCFileUtils::fullPathForFilename` as-is, without any modifications.
2. AsyncLoad fullPathForFilename rewrite, with added thread safety and increased performance (used when directory snapshots are off in Blaze)
3. AsyncLoad fullPathForFilename rewrite combined with Blaze directory snapshots setting to replace file existance checks

| Path kind  | Stock GD  |  AsyncLoad FPFF  | AL+Blaze snapshots | Speedup in default Blaze config |
|------------|-----------|------------------|--------------------|---------------------------------|
| Bogus path | 5.96 ms   | 5.91 µs          | **2.43 µs**        | **~2400x** |
| Real sheet | 477 ns    | 172 ns           | **100 ns**         | **~5x** |
| Game icon  | 220.30 µs | 72.18 µs         | **2.24 µs**        | **~100x** |

The numbers speak for themselves: without directory snapshotting, Blaze utilizes AsyncLoad's FPFF rewrite to achieve ~1000x improvement for loading non-existing files and ~3x improvement in other cases.

With directory snapshotting (which is currently the default configuration), Blaze wins the competition with no contest, due to not needing to call filesystem operations on every call.

Notes:
1. Directory snapshotting is currently available only on Windows. Other platforms use the plain AsyncLoad FPFF method, which is still significantly faster than regular GD.
2. The figures are purely informational and do not represent every real workload. The numbers may be worse on devices with worse drives or may also be affected by an antivirus. That said, the performance should *always* be *strictly better* than vanilla GD, otherwise it is a bug.
3. While AsyncLoad's `fullPathForFilename` aims to preserve 1:1 compatibility, behavior is different when directory snapshots are enabled. This doesn't matter for virtually all usages, but if a mod tries to use `CCFileUtils` for reading a file that was written very recently (during game's runtime), the file may not be found until textures are reloaded or cache is cleared.