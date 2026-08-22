#include <Geode/Geode.hpp>
#include <Geode/modify/DS_Dictionary.hpp>
#include <AsyncLoad/FileUtils.hpp>
#include <util/macros.hpp>
#include "LoadModule.hpp"

using namespace geode::prelude;

namespace blaze {

struct HookedDSDict : Modify<HookedDSDict, DS_Dictionary> {
    static void onModify(auto& self) {
        // LoadModule::get().addHooks(
        //     self,
        //     "DS_Dictionary::loadRootSubDictFromCompressedFile"
        // );
    }

    // TODO: this hook removed because no pugi bindings ...

    // $override
    // bool loadRootSubDictFromCompressedFile(const char* fileName) {
    //     dictTree.clear();
    //     dictTree.push_back(pugi::xml_node());

    //     // TODO: verify how true is this, there's also FileOperation::getFilePath
    //     std::string path { CCFileUtils::get()->getWritablePath2() };
    //     path += fileName;

    //     auto result = AsyncLoad::getFileDataOwned(path.c_str());
    //     if (!result) {
    //         log::warn("DS_Dictionary load from '{}' failed: {}", path, result.unwrapErr());
    //         return false;
    //     }

    //     // decompress
    //     auto buf = std::move(result).unwrap();
    //     auto data = ZipUtils::decompressString2(buf.data.get(), true, buf.size, 11);

    //     // parse xml
    //     auto xmlResult = doc.load_buffer(data.data(), data.size());
    //     if (!xmlResult) return false;

    //     // set tree
    //     dictTree.back() = doc.child("plist").child("dict");

    //     // inlined checkCompatibility()
    //     compatible = doc.child("plist").attribute("gjver").as_int(0) < 2;

    //     return true;
    // }
};

}
