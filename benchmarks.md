# Benchmarks

This file contains some benchmarks that were ran to compare performance of vanilla game without Blaze hooks, and with Blaze hooks applied. Some of the things here have their source code in [`src/benchmarks/](./src/benchmarks/).

## `fullPathForFilename`

Source code: [fpff.cpp](./src/benchmarks/fpff.cpp)

Three kind of benchmarks are done, all have `ignoreSuffix = false`:
* Calling with a bogus path repeatedly: `icons/fake/ass/png`
* Calling with a real game sheet that already was loaded, repeatedly
* Calling with every icon, most of which have not been loaded yet (Globed disabled)

There are also three path resolution methods benchmarked:
1. Stock GD/Cocos behavior: calling `CCFileUtils::fullPathForFilename` as-is, without any modifications.
2. AsyncLoad fullPathForFilename rewrite, with added thread safety and increased performance (used by default in Blaze)
3. AsyncLoad fullPathForFilename rewrite combined with Blaze directory snapshots setting to replace file existance checks

| Path kind  | Stock GD  |  AsyncLoad FPFF  | AL+Blaze snapshots |
|------------|-----------|------------------|--------------------|
| Bogus path | 5.96 ms   | 5.91 µs          | **2.43 µs**  |
| Real sheet | 477 ns    | 172 ns           | **100 ns**   |
| Game icon  | 220.30 µs | 72.18 µs         | **2.24 µs**  |

The numbers speak for themselves: without directory snapshotting, Blaze utilizes AsyncLoad's FPFF rewrite to achieve ~1000x improvement for loading non-existing files and ~3x improvement in other cases.

With directory snapshotting, Blaze wins the competition with no contest, due to not needing to call filesystem operations on every call.