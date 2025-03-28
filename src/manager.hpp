#pragma once

#include <asp/thread/Thread.hpp>
#include <asp/sync/Channel.hpp>
#include <util.hpp>
#include <util/memory_chunk.hpp>

#include <filesystem>

class LoadManager : public SingletonBase<LoadManager> {
    friend class SingletonBase;
    LoadManager();

public:
    bool reallocFromCache(uint32_t checksum, uint8_t*& outbuf, size_t& outsize);
    void queueForCache(const std::filesystem::path& path, std::vector<uint8_t>&& data);
    std::filesystem::path getCacheDir();
    std::filesystem::path cacheFileForChecksum(uint32_t crc);
    std::unique_ptr<uint8_t[]> readFile(const char* path, size_t& outSize, bool absolutePath = false);
    blaze::OwnedMemoryChunk readFileToChunk(const char* path);

private:
    using ConverterTask = std::pair<std::filesystem::path, std::vector<uint8_t>>;

    asp::Thread<LoadManager*> converterThread;
    asp::Channel<ConverterTask> converterQueue;

    void threadFunc(decltype(converterThread)::StopToken&);
};
