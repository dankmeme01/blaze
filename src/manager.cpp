#include "manager.hpp"

#include <crc32.hpp>
#include <formats.hpp>

using namespace geode::prelude;

$execute {
    auto dir = Mod::get()->getSaveDir() / "cached-images";
    (void) file::createDirectoryAll(dir);
}

LoadManager::LoadManager() {
    converterThread.setStartFunction([] {
        utils::thread::setName("PNG Converter");
    });

    converterThread.setExceptionFunction([](const auto& exc) {
        log::error("converter thread failed: {}", exc.what());

        Loader::get()->queueInMainThread([msg = std::string(exc.what())] {
            FLAlertLayer::create("FastPNG error", msg, "Ok")->show();
        });
    });

    converterThread.setLoopFunction(&LoadManager::threadFunc);
    converterThread.start(this);
}

std::filesystem::path LoadManager::getCacheDir() {
    return Mod::get()->getSaveDir() / "cached-images";
}

std::filesystem::path LoadManager::cacheFileForChecksum(uint32_t checksum) {
    return this->getCacheDir() / std::to_string(checksum);
}

bool LoadManager::reallocFromCache(uint32_t checksum, uint8_t*& outbuf, size_t& outsize) {
    auto filep = this->cacheFileForChecksum(checksum);

    std::ifstream file(filep, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    std::error_code ec;
    auto fsize = std::filesystem::file_size(filep, ec);

    if (ec != std::error_code{}) {
        auto fsize = file.tellg();
        file.seekg(0, std::ios::end);
        fsize = file.tellg() - fsize;
        file.seekg(0, std::ios::beg);
    }

    outsize = fsize;

    outbuf = new uint8_t[fsize];

    file.read(reinterpret_cast<char*>(outbuf), fsize);
    file.close();

    return true;
}

void LoadManager::queueForCache(const std::filesystem::path& path, std::vector<uint8_t>&& data) {
    converterQueue.push(std::make_pair(path, std::move(data)));
}

std::unique_ptr<uint8_t[]> LoadManager::readFile(const char* path, size_t& outSize) {
    unsigned long s;

    GEODE_ANDROID(auto lock = zipFileMutex.lock());

    auto buf = CCFileUtils::get()->getFileData(path, "rb", &s);
    outSize = s;

    return std::unique_ptr<uint8_t[]>(buf);
}

void LoadManager::threadFunc(decltype(converterThread)::StopToken& st) {
    auto tasko = converterQueue.popTimeout(std::chrono::seconds(1));

    if (!tasko) return;

    auto& task = tasko.value();
    auto& path = task.first;

    auto data = std::move(task.second);

    if (data.empty()) {
        log::warn("Failed to convert image (couldn't open file): {}", path);
        return;
    }

    // compute the checksum for saving the file
    auto checksum = blaze::crc32(data.data(), data.size());

    // decode with spng, re-encode with fpng/raw
    auto res = blaze::decodeSPNG(data.data(), data.size());
    if (!res) {
        log::warn("Failed to convert image (decode error: {}): {}", res.unwrapErr(), path);
        return;
    }

    auto result = std::move(res.unwrap());

    auto encodedres = blaze::encodeFPNG(result.rawData.get(), result.rawSize, result.width, result.height);

    if (!encodedres) {
        log::warn("Failed to convert image (encode error: {}): {}", encodedres.unwrapErr(), path);
        return;
    }

    auto encoded = std::move(encodedres.unwrap());
    auto outPath = this->getCacheDir() / std::to_string(checksum);

    std::ofstream file(outPath, std::ios::out | std::ios::binary);

    file.write(reinterpret_cast<const char*>(encoded.data()), encoded.size());
    file.close();

    // log::info("Converted and saved {} as cached image {}", path, checksum);
}


