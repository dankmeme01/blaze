#include <modules/allocator/AllocatorModule.hpp>
#include <mimalloc.h>

using namespace geode::prelude;

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

static void* dtr_calloc(size_t count, size_t size) {
    return mi_calloc(count, size);
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

static void dtr_free(void* p) {
    if (!mi_cfree(p)) {
        HeapFree(crtHeap(), 0, p);
    }
}

static size_t dtr_msize(const void* p) {
    return mi_is_in_heap_region(p) ? mi_usable_size(p) : HeapSize(crtHeap(), 0, p);
}

static void* dtr_recalloc(void* p, size_t count, size_t size) {
    if (!p) return mi_calloc(count, size);

    if (mi_is_in_heap_region(p)) return mi_recalloc(p, count, size);

    auto oldSize = HeapSize(crtHeap(), 0, p);
    auto q = HeapReAlloc(crtHeap(), 0, p, count * size);
    std::memset((char*)q + oldSize, 0, count * size - oldSize);

    return q;
}

static void* dtr_expand(void* p, size_t newsize) {
    if (!p) return mi_malloc(newsize);

    if (mi_is_in_heap_region(p)) return mi_expand(p, newsize);

    return HeapReAlloc(crtHeap(), HEAP_REALLOC_IN_PLACE_ONLY, p, newsize);
}

static void doHook(MallocHook& hook) {
    auto dll = GetModuleHandleW(hook.dll);
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

    auto hooks = std::to_array<MallocHook>({
        {L"ucrtbase.dll", "_malloc_base", dtr_malloc},
        {L"ucrtbase.dll", "_free_base", dtr_free},
        {L"ucrtbase.dll", "_realloc_base", dtr_realloc},
        {L"ucrtbase.dll", "_calloc_base", dtr_calloc},
        {L"ucrtbase.dll", "_recalloc_base", dtr_recalloc},
        {L"ucrtbase.dll", "_expand_base", dtr_expand},
        {L"ucrtbase.dll", "_msize_base", dtr_msize},
        {L"ucrtbase.dll", "malloc", dtr_malloc},
        {L"ucrtbase.dll", "free", dtr_free},
        {L"ucrtbase.dll", "realloc", dtr_realloc},
        {L"ucrtbase.dll", "calloc", dtr_calloc},
        {L"ucrtbase.dll", "_recalloc", dtr_recalloc},
        {L"ucrtbase.dll", "_expand", dtr_expand},
        {L"ucrtbase.dll", "_msize", dtr_msize},
    });

    for (auto& h : hooks) {
        doHook(h);
    }

    mi_register_output([](const char* msg, void*) {
        log::info("(mimalloc): {}", msg);
    }, nullptr);

    mi_register_error([](int err, void*) {
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

    bool ok =
        mi_is_in_heap_region(p.get())
        && p2 && mi_is_in_heap_region(p2)
        && p3 && mi_is_in_heap_region(p3)
        && p4.size() == 38 && mi_is_in_heap_region(p4.data());

    p2 = realloc(p2, 32);
    p3 = _recalloc(p3, 32, 1);
    ok = ok
        && p2 && mi_is_in_heap_region(p2)
        && p3 && mi_is_in_heap_region(p3);

    free(p2);
    free(p3);

    if (!ok) {
        log::error("Allocator hooks smoke test failed!");
    }
}
