#include <Geode/Geode.hpp>

#include <modules/algorithm/Compressor.hpp>
#include <modules/algorithm/Decompressor.hpp>

using namespace geode::prelude;

// $execute {
//     auto testFile = utils::file::readBinary("Z:/home/dankpc/libcocos2dcpp.so.cpp").unwrap();

//     auto t1 = asp::Instant::now();
//     blaze::Compressor c{1};
//     auto compressed = c.compress(testFile.data(), testFile.size());
//     auto t2 = asp::Instant::now();

//     blaze::Decompressor d;
//     auto decoded = d.decompress(compressed.data.get(), compressed.size).unwrap();
//     auto t3 = asp::Instant::now();

//     bool matches = asp::iter::from(decoded.span()).zip(asp::iter::from(testFile)).all([](auto& pair) {
//         return pair.first == pair.second;
//     });

//     log::debug("Compress: {}, decompress: {}, match: {}, compressed size: {}", t2.durationSince(t1), t3.durationSince(t2), matches, compressed.size);
// }

// Compress: 111.959ms, decompress: 42.992ms, match: true, compressed size: 9847970
