#include <modules/allocator/AllocatorModule.hpp>
#include <mimalloc.h>

using namespace geode::prelude;

static std::atomic<size_t> g_errors = 0;

struct MallocHook {
    const wchar_t* dll;
    const char* name;
    void* detour;

    template <typename F> requires (std::is_pointer_v<F> && std::is_function_v<std::remove_pointer_t<F>>)
    MallocHook(const wchar_t* dll, const char* name, F detour)
        : dll(dll), name(name), detour((void*)detour) {}
};

static inline auto crtHeap() {
    static auto h = _get_heap_handle();
    return (HANDLE)h;
}

static void* dtr_malloc(size_t size) {
    return mi_malloc(size);
}

static void* dtr_malloc_dbg(size_t size, int, const char*, int) {
    return dtr_malloc(size);
}

static void* dtr_calloc(size_t count, size_t size) {
    return mi_calloc(count, size);
}

static void* dtr_calloc_dbg(size_t count, size_t size, int, const char*, int) {
    return dtr_calloc(count, size);
}

static void dtr_free(void* p) {
    if (!mi_cfree(p)) {
        HeapFree(crtHeap(), 0, p);
    }
}

static void dtr_free_dbg(void* p, int) {
    dtr_free(p);
}

static size_t dtr_msize(const void* p) {
    return mi_is_in_heap_region(p) ? mi_usable_size(p) : HeapSize(crtHeap(), 0, p);
}

static size_t dtr_msize_dbg(const void* p, int) {
    return dtr_msize(p);
}

static void* dtr_realloc(void* p, size_t newsize) {
    if (!p) return mi_malloc(newsize);
    if (mi_is_in_heap_region(p)) return mi_realloc(p, newsize);

    if (newsize == 0) {
        HeapFree(crtHeap(), 0, p);
        return nullptr;
    }

    return HeapReAlloc(crtHeap(), 0, p, newsize);
}

static void* dtr_realloc_dbg(void* p, size_t newsize, int, const char*, int) {
    return dtr_realloc(p, newsize);
}

static void* dtr_recalloc(void* p, size_t count, size_t size) {
    if (!p) return mi_calloc(count, size);
    if (mi_is_in_heap_region(p)) return mi_recalloc(p, count, size);

    size_t newSize = count * size;
    size_t oldSize = HeapSize(crtHeap(), 0, p);
    auto q = HeapReAlloc(crtHeap(), 0, p, newSize);

    if (q && newSize > oldSize) {
        std::memset((char*)q + oldSize, 0, newSize - oldSize);
    }

    return q;
}

static void* dtr_recalloc_dbg(void* p, size_t count, size_t size, int, const char*, int) {
    return dtr_recalloc(p, count, size);
}

static void* dtr_expand(void* p, size_t newsize) {
    if (!p) return mi_malloc(newsize);
    if (mi_is_in_heap_region(p)) return mi_expand(p, newsize);

    return HeapReAlloc(crtHeap(), HEAP_REALLOC_IN_PLACE_ONLY, p, newsize);
}


static void* dtr_expand_dbg(void* p, size_t newsize, int, const char*, int) {
    return dtr_expand(p, newsize);
}


/// -- aligned allocations -- ///


static void* foreignAlignedBase(const void* p) {
    auto slot = (uintptr_t*)((reinterpret_cast<uintptr_t>(p) & ~(sizeof(void*) - 1)) - sizeof(void*));
    return (void*)(*slot);
}

static size_t foreignAlignedUsableSize(const void* p) {
    auto base = foreignAlignedBase(p);
    return HeapSize(crtHeap(), 0, base) - ((uintptr_t)p - (uintptr_t)base);
}


static void* dtr_aligned_malloc(size_t size, size_t alignment) {
    return mi_malloc_aligned(size, alignment);
}

static void* dtr_aligned_malloc_dbg(size_t size, size_t alignment, int, const char*, int) {
    return dtr_aligned_malloc(size, alignment);
}

static void dtr_aligned_free(void* p) {
    if (!mi_cfree(p)) {
        HeapFree(crtHeap(), 0, foreignAlignedBase(p));
    }
}

static void dtr_aligned_free_dbg(void* p, int) {
    dtr_aligned_free(p);
}

static size_t dtr_aligned_msize(const void* p, size_t alignment) {
    return mi_is_in_heap_region(p) ? mi_usable_size(p) : foreignAlignedUsableSize(p);
}

static size_t dtr_aligned_msize_dbg(const void* p, size_t alignment, int) {
    return dtr_aligned_msize(p, alignment);
}

static void* foreignAlignedMigrate(void* p, size_t newSize, size_t alignment, size_t offset, bool zero) {
    auto base = foreignAlignedBase(p);
    auto oldSize = foreignAlignedUsableSize(p);

    if (newSize == 0) {
        HeapFree(crtHeap(), 0, base);
        return nullptr;
    }

    auto q = zero
        ? mi_zalloc_aligned_at(newSize, alignment, offset)
        : mi_malloc_aligned_at(newSize, alignment, offset);
    if (!q) return nullptr;

    std::memcpy(q, p, std::min(oldSize, newSize));
    HeapFree(crtHeap(), 0, base);

    return q;
}

static void* dtr_aligned_realloc(void* p, size_t newsize, size_t alignment) {
    if (!p) return mi_malloc_aligned(newsize, alignment);
    if (mi_is_in_heap_region(p)) return mi_realloc_aligned(p, newsize, alignment);
    return foreignAlignedMigrate(p, newsize, alignment, 0, false);
}

static void* dtr_aligned_realloc_dbg(void* p, size_t newsize, size_t alignment, int, const char*, int) {
    return dtr_aligned_realloc(p, newsize, alignment);
}

static void* dtr_aligned_recalloc(void* p, size_t newcount, size_t size, size_t alignment) {
    if (!p) return mi_calloc_aligned(newcount, size, alignment);
    if (mi_is_in_heap_region(p)) return mi_aligned_recalloc(p, newcount, size, alignment);
    return foreignAlignedMigrate(p, newcount * size, alignment, 0, true);
}

static void* dtr_aligned_recalloc_dbg(void* p, size_t newcount, size_t size, size_t alignment, int, const char*, int) {
    return dtr_aligned_recalloc(p, newcount, size, alignment);
}

static void* dtr_aligned_offset_malloc(size_t size, size_t alignment, size_t offset) {
    return mi_malloc_aligned_at(size, alignment, offset);
}

static void* dtr_aligned_offset_malloc_dbg(size_t size, size_t alignment, size_t offset, int, const char*, int) {
    return dtr_aligned_offset_malloc(size, alignment, offset);
}

static void* dtr_aligned_offset_realloc(void* p, size_t newsize, size_t alignment, size_t offset) {
    if (!p) return mi_malloc_aligned_at(newsize, alignment, offset);
    if (mi_is_in_heap_region(p)) return mi_realloc_aligned_at(p, newsize, alignment, offset);
    return foreignAlignedMigrate(p, newsize, alignment, offset, false);
}

static void* dtr_aligned_offset_realloc_dbg(void* p, size_t newsize, size_t alignment, size_t offset, int, const char*, int) {
    return dtr_aligned_offset_realloc(p, newsize, alignment, offset);
}

static void* dtr_aligned_offset_recalloc(void* p, size_t newcount, size_t size, size_t alignment, size_t offset) {
    if (!p) return mi_calloc_aligned_at(newcount, size, alignment, offset);
    if (mi_is_in_heap_region(p)) return mi_aligned_offset_recalloc(p, newcount, size, alignment, offset);
    return foreignAlignedMigrate(p, newcount * size, alignment, offset, true);
}

static void* dtr_aligned_offset_recalloc_dbg(void* p, size_t newcount, size_t size, size_t alignment, size_t offset, int, const char*, int) {
    return dtr_aligned_offset_recalloc(p, newcount, size, alignment, offset);
}

/// -- hooks -- ///


static void doHook(MallocHook& hook) {
    // cache the dll so consecutive tries don't invoke gmh again
    static std::pair<HMODULE, std::wstring_view> lastModule;

    HMODULE dll;
    if (lastModule.second == hook.dll) {
        dll = lastModule.first;
    } else {
        dll = GetModuleHandleW(hook.dll);
        lastModule = {dll, hook.dll};
    }

    if (!dll) return;

    auto addr = GetProcAddress(dll, hook.name);
    if (!addr) return;

    std::array<uint8_t, 12> patch;
    patch[0] = 0x48; // mov rax, imm64
    patch[1] = 0xb8;
    std::memcpy(&patch[2], &hook.detour, sizeof(void*));
    patch[10] = 0xff; // jmp rax
    patch[11] = 0xe0;

    DWORD oldProt;
    if (!VirtualProtect((void*)addr, patch.size(), PAGE_EXECUTE_READWRITE, &oldProt)) {
        log::error("Failed to hook {}, protection failed: {}", hook.name, GetLastError());
        return;
    }

    std::memcpy((void*)addr, patch.data(), patch.size());
    FlushInstructionCache(GetCurrentProcess(), (void*)addr, patch.size());

    VirtualProtect((void*)addr, patch.size(), oldProt, &oldProt);
}

static void smokeTest();

$execute {
    // TODO: only run this when module is enabled
    log::info("Enabling allocator hooks (using mimalloc {})", mi_version());

    // pre-load ucrtbased.dll so hooks succeed
    // this is required because blaze may load before any mods that are built in debug, and dll won't exist at that time.
    // there is little harm to loading the DLL even if there are no debug mods
    LoadLibraryW(L"ucrtbased.dll");

    auto hooks = std::to_array<MallocHook>({
        // Release - regular allocation
        {L"ucrtbase.dll", "_free_base", dtr_free},
        {L"ucrtbase.dll", "free", dtr_free},
        {L"ucrtbase.dll", "_realloc_base", dtr_realloc},
        {L"ucrtbase.dll", "realloc", dtr_realloc},
        {L"ucrtbase.dll", "_recalloc_base", dtr_recalloc},
        {L"ucrtbase.dll", "_recalloc", dtr_recalloc},
        {L"ucrtbase.dll", "_expand_base", dtr_expand},
        {L"ucrtbase.dll", "_expand", dtr_expand},
        {L"ucrtbase.dll", "_msize_base", dtr_msize},
        {L"ucrtbase.dll", "_msize", dtr_msize},
        {L"ucrtbase.dll", "_malloc_base", dtr_malloc},
        {L"ucrtbase.dll", "malloc", dtr_malloc},
        {L"ucrtbase.dll", "_calloc_base", dtr_calloc},
        {L"ucrtbase.dll", "calloc", dtr_calloc},

        // Release - aligned allocation
        {L"ucrtbase.dll", "_aligned_free", dtr_aligned_free},
        {L"ucrtbase.dll", "_aligned_realloc", dtr_aligned_realloc},
        {L"ucrtbase.dll", "_aligned_offset_realloc", dtr_aligned_offset_realloc},
        {L"ucrtbase.dll", "_aligned_recalloc", dtr_aligned_recalloc},
        {L"ucrtbase.dll", "_aligned_offset_recalloc", dtr_aligned_offset_recalloc},
        {L"ucrtbase.dll", "_aligned_msize", dtr_aligned_msize},
        {L"ucrtbase.dll", "_aligned_malloc", dtr_aligned_malloc},
        {L"ucrtbase.dll", "_aligned_offset_malloc", dtr_aligned_offset_malloc},

        // Debug - regular allocation
        {L"ucrtbased.dll", "_free_dbg", dtr_free_dbg},
        {L"ucrtbased.dll", "_realloc_dbg", dtr_realloc_dbg},
        {L"ucrtbased.dll", "_recalloc_dbg", dtr_recalloc_dbg},
        {L"ucrtbased.dll", "_expand_dbg", dtr_expand_dbg},
        {L"ucrtbased.dll", "_msize_dbg", dtr_msize_dbg},
        {L"ucrtbased.dll", "_calloc_dbg", dtr_calloc_dbg},
        {L"ucrtbased.dll", "_malloc_dbg", dtr_malloc_dbg},

        // Debug - aligned allocation
        {L"ucrtbased.dll", "_aligned_free_dbg", dtr_aligned_free_dbg},
        {L"ucrtbased.dll", "_aligned_realloc_dbg", dtr_aligned_realloc_dbg},
        {L"ucrtbased.dll", "_aligned_offset_realloc_dbg", dtr_aligned_offset_realloc_dbg},
        {L"ucrtbased.dll", "_aligned_recalloc_dbg", dtr_aligned_recalloc_dbg},
        {L"ucrtbased.dll", "_aligned_offset_recalloc_dbg", dtr_aligned_offset_recalloc_dbg},
        {L"ucrtbased.dll", "_aligned_msize_dbg", dtr_aligned_msize_dbg},
        {L"ucrtbased.dll", "_aligned_malloc_dbg", dtr_aligned_malloc_dbg},
        {L"ucrtbased.dll", "_aligned_offset_malloc_dbg", dtr_aligned_offset_malloc_dbg},
    });

    for (auto& h : hooks) {
        doHook(h);
    }

    mi_register_output([](const char* msg, void*) {
        log::info("(mimalloc): {}", msg);
    }, nullptr);

    mi_register_error([](int err, void*) {
        g_errors.fetch_add(1, std::memory_order::relaxed);
        log::error("(mimalloc) error: {}", err);
    }, nullptr);

#ifdef BLAZE_DEBUG
    smokeTest();
#endif
}

void smokeTest() {
    // run some tests to ensure hooks worked
    auto p = std::make_unique<uint8_t[]>(16);
    auto p2 = malloc(16);
    auto p3 = calloc(16, 1);
    std::string p4 = "hello world this is long to bypass sso";
    auto p5 = _aligned_malloc(64, 64);

    bool ok =
        mi_is_in_heap_region(p.get())
        && p2 && mi_is_in_heap_region(p2)
        && p3 && mi_is_in_heap_region(p3)
        && p4.size() == 38 && mi_is_in_heap_region(p4.data())
        && p5 && mi_is_in_heap_region(p5) && ((uintptr_t)p5 % 64 == 0) && _aligned_msize(p5, 64, 0) == 64;

    p2 = realloc(p2, 32);
    p3 = _recalloc(p3, 32, 1);
    p5 = _aligned_realloc(p5, 128, 64);
    ok = ok
        && p2 && mi_is_in_heap_region(p2)
        && p3 && mi_is_in_heap_region(p3)
        && p5 && mi_is_in_heap_region(p5);

    free(p2);
    free(p3);
    _aligned_free(p5);

    auto errors = g_errors.load(std::memory_order::relaxed);
    if (!ok || errors > 0) {
        log::error("Allocator hooks smoke test failed! Error count: {}", errors);
    }
}
