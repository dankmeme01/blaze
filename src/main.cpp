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

// #include <Geode/modify/AppDelegate.hpp>
// #include <Geode/modify/CCEGLView.hpp>
// class $modify(AppDelegate) {
//     void setupGLView() {
//         log::info("Pre setup");
//         AppDelegate::setupGLView();
//         log::info("Post setup");
//         std::abort();
//     }
// };

// class $modify(CCEGLView) {
//     static cocos2d::CCEGLView* createWithRect(std::string const& p1, cocos2d::CCRect p2, float p3) {
//         log::info("Pre createWithRect");
//         auto ret = CCEGLView::createWithRect(p1, p2, p3);
//         log::info("Post createWithRect");
//         return ret;
//     }
// };
