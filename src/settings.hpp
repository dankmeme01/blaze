#pragma once

namespace blaze {
    struct _settings {
        bool _init = false;
        bool fastDecompression = false;
        bool imageCache = false;
        bool imageCacheSmall = false;
        bool asyncGlfw = false;
        bool asyncFmod = false;
        bool fastSaving = false;
        bool uncompressedSaves = false;
        bool lowMemory = false;
        bool loadMore = false;
    };

    _settings& settings();
}