#pragma once
#include "pch.h"
#include <atomic>
#include <mutex>
#include <cstdint>
#include <memory>

#include <cstdlib>
#include <stdexcept>
#include <windows.h>

// ============================================================================
// QuantumArena - 量子流架构核心内存池
// ============================================================================
// 设计目标:
//   1. 0ns 级 Reset() - 通过指针重置而非释放
//   2. 64 字节对齐 - SIMD/AVX 友好 (Cache Line Alignment)
//   3. 双缓冲支持 - Active/Back 交换
//   4. 无锁设计 - 单线程独占使用 (HeavyLane 专用)
//   5. 溢出保护 - 超大图自动溢出到系统堆
//   6. 动态按需提交 - 预留海量地址空间，用到多少提交多少，闲置时释放物理内存
// ============================================================================

class QuantumArena;


class QuantumArena {
public:
    // 默认 512MB 预分配 (足够 8K x 8K RGBA)
    static constexpr size_t DEFAULT_SIZE = 512 * 1024 * 1024;
    static constexpr size_t ALIGNMENT = 64; // Cache Line

    QuantumArena(size_t capacity = DEFAULT_SIZE) 
        : m_capacity(capacity) 
    {
        // 延迟初始化 - 构造函数不分配内存
    }

    ~QuantumArena() {
        FreeOverflows();
        if (m_buffer) {
            VirtualFree(m_buffer, 0, MEM_RELEASE);
            m_buffer = nullptr;
        }
    }

    // 禁止拷贝
    QuantumArena(const QuantumArena&) = delete;
    QuantumArena& operator=(const QuantumArena&) = delete;

    // 允许移动
    QuantumArena(QuantumArena&& other) noexcept 
        : m_buffer(other.m_buffer)
        , m_capacity(other.m_capacity)
        , m_committed(other.m_committed.load())
        , m_offset(other.m_offset.load())
        , m_peakUsage(other.m_peakUsage.load())
        , m_overflowHead(other.m_overflowHead)
    {
        other.m_buffer = nullptr;
        other.m_capacity = 0;
        other.m_committed = 0;
        other.m_offset = 0;
        other.m_overflowHead = nullptr;
    }

    // ========== 核心操作 ==========


    // Declaration for global strategy check (implemented in main.cpp or ImageEngine.cpp)
    static bool ShouldShrinkMemory() noexcept;

    /// <summary>
    /// 极速重置 - 0ns 级操作
    /// 不释放内存，仅重置分配指针。并动态释放物理内存归还系统
    /// 警告: 调用后，之前分配的所有内存变为无效！
    /// </summary>
    void Reset() noexcept {
        FreeOverflows();
        m_offset = 0;
        if (ShouldShrinkMemory()) {
            Shrink(); // 归还空闲物理内存
        }
    }

    /// <summary>
    /// 收缩内存 - 将未使用的预留内存去提交 (Decommit)，归还给 OS
    /// </summary>
    void Shrink() noexcept {
        if (!m_buffer) return;
        size_t used = m_offset.load(std::memory_order_relaxed);
        // 按 4KB 页对齐
        size_t keepSize = (used + 4095) & ~4095;
        if (keepSize < m_committed) {
            std::lock_guard<std::mutex> lock(m_commitMutex);
            if (keepSize < m_committed) {
                size_t decommitSize = m_committed - keepSize;
                VirtualFree(m_buffer + keepSize, decommitSize, MEM_DECOMMIT);
                m_committed = keepSize;
            }
        }
    }

    /// <summary>
    /// 线性分配 (Arena 语义)
    /// 返回对齐的内存块，失败返回 nullptr
    /// </summary>
    void* Allocate(size_t size, size_t alignment = ALIGNMENT) noexcept {
        EnsureInitialized();
        if (!m_buffer) return AllocateOverflow(size, alignment);

        // 计算对齐后的偏移
        size_t current = m_offset.load(std::memory_order_relaxed);
        size_t aligned = (current + alignment - 1) & ~(alignment - 1);
        size_t newOffset = aligned + size;

        // 检查是否溢出虚拟容量
        if (newOffset > m_capacity) {
            // 溢出到系统堆 (防爆仓)
            return AllocateOverflow(size, alignment);
        }

        // 按需动态提交物理内存 (MEM_COMMIT)
        if (newOffset > m_committed) {
            std::lock_guard<std::mutex> lock(m_commitMutex);
            if (newOffset > m_committed) {
                size_t chunk = 1024 * 1024; // 1MB 粒度提交，兼顾性能和内存占用
                size_t targetCommit = (newOffset + chunk - 1) & ~(chunk - 1);
                if (targetCommit > m_capacity) targetCommit = m_capacity;

                size_t commitSize = targetCommit - m_committed;
                void* res = VirtualAlloc(m_buffer + m_committed, commitSize, MEM_COMMIT, PAGE_READWRITE);
                if (!res) {
                    return AllocateOverflow(size, alignment);
                }
                m_committed = targetCommit;
            }
        }

        // CAS 更新 (虽然设计为单线程，但保持原子性以防万一)
        while (!m_offset.compare_exchange_weak(current, newOffset,
            std::memory_order_release, std::memory_order_relaxed)) 
        {
            aligned = (current + alignment - 1) & ~(alignment - 1);
            newOffset = aligned + size;
            
            if (newOffset > m_capacity) {
                return AllocateOverflow(size, alignment);
            }
            
            if (newOffset > m_committed) {
                std::lock_guard<std::mutex> lock(m_commitMutex);
                if (newOffset > m_committed) {
                    size_t chunk = 1024 * 1024;
                    size_t targetCommit = (newOffset + chunk - 1) & ~(chunk - 1);
                    if (targetCommit > m_capacity) targetCommit = m_capacity;
                    size_t commitSize = targetCommit - m_committed;
                    void* res = VirtualAlloc(m_buffer + m_committed, commitSize, MEM_COMMIT, PAGE_READWRITE);
                    if (!res) {
                        return AllocateOverflow(size, alignment);
                    }
                    m_committed = targetCommit;
                }
            }
        }

        // 更新峰值统计
        size_t peak = m_peakUsage.load(std::memory_order_relaxed);
        while (newOffset > peak && !m_peakUsage.compare_exchange_weak(peak, newOffset));

        return m_buffer + aligned;
    }

    /// <summary>
    /// 检查指针是否属于此 Arena (用于判断是否需要 free)
    /// </summary>
    bool Owns(void* ptr) const noexcept {
        if (!ptr) return false;
        if (m_buffer) {
            char* p = static_cast<char*>(ptr);
            if (p >= m_buffer && p < m_buffer + m_capacity) return true;
        }
        // 检查溢出链表 (降级大图分配通常只有个位数个节点，遍历开销可忽略)
        void* curr = m_overflowHead;
        while (curr) {
            void* node_ptr = static_cast<char*>(curr) + ALIGNMENT;
            if (node_ptr == ptr) return true;
            curr = *static_cast<void**>(curr);
        }
        return false;
    }

    // ========== 统计信息 ==========

    size_t GetCapacity() const noexcept { return m_capacity; }
    size_t GetUsedBytes() const noexcept { return m_offset.load(std::memory_order_relaxed); }
    size_t GetPeakUsage() const noexcept { return m_peakUsage.load(std::memory_order_relaxed); }
    size_t GetFreeBytes() const noexcept { 
        size_t used = m_offset.load(std::memory_order_relaxed);
        return used < m_capacity ? m_capacity - used : 0; 
    }
    bool IsInitialized() const noexcept { return m_buffer != nullptr; }

private:
    void* AllocateOverflow(size_t size, size_t alignment) noexcept {
        // 分配 size + alignment，将链表节点放在前面，以保证返回的业务指针依然满足 alignment
        void* raw_ptr = _aligned_malloc(size + alignment, alignment);
        if (raw_ptr) {
            std::lock_guard<std::mutex> lock(m_overflowMutex);
            void** header = static_cast<void**>(raw_ptr);
            *header = m_overflowHead;
            m_overflowHead = raw_ptr;
            return static_cast<char*>(raw_ptr) + alignment;
        }
        return nullptr;
    }

    void FreeOverflows() noexcept {
        void* curr = nullptr;
        {
            std::lock_guard<std::mutex> lock(m_overflowMutex);
            curr = m_overflowHead;
            m_overflowHead = nullptr;
        }
        while (curr) {
            void* next = *static_cast<void**>(curr);
            _aligned_free(curr);
            curr = next;
        }
    }

    void EnsureInitialized() {
        if (m_buffer) return;

        // 仅预留虚拟地址空间 (MEM_RESERVE)，不消耗任何物理内存！
        m_buffer = static_cast<char*>(VirtualAlloc(nullptr, m_capacity, MEM_RESERVE, PAGE_READWRITE));
        if (!m_buffer) {
            return;
        }

        m_committed = 0;
    }

    char* m_buffer = nullptr;
    size_t m_capacity;
    std::atomic<size_t> m_committed{0};
    std::mutex m_commitMutex;
    std::atomic<size_t> m_offset{0};
    std::atomic<size_t> m_peakUsage{0};

    std::mutex m_overflowMutex;
    void* m_overflowHead = nullptr;
    
public:
    std::atomic<int> m_activeJobs{0};
};



// ============================================================================
// QuantumArenaPool - 双缓冲 Arena 管理器
// ============================================================================
// 用于 Ping-Pong 模式：一个 Arena 供当前显示，另一个供后台解码
// ============================================================================

class QuantumArenaPool {
public:
    QuantumArenaPool(size_t arenaSize = QuantumArena::DEFAULT_SIZE)
        : m_arenas{QuantumArena(arenaSize), QuantumArena(arenaSize)}
        , m_activeIndex(0)
    {}

    /// <summary>
    /// 获取当前活跃的 Arena (屏幕显示中)
    /// </summary>
    QuantumArena& GetActive() noexcept {
        return m_arenas[m_activeIndex.load(std::memory_order_acquire)];
    }

    /// <summary>
    /// 获取后台 Arena (解码用)
    /// </summary>
    QuantumArena& GetBack() noexcept {
        return m_arenas[1 - m_activeIndex.load(std::memory_order_acquire)];
    }

    /// <summary>
    /// 交换 Active 和 Back Arena (解码完成后调用)
    /// </summary>
    void Swap() noexcept {
        m_activeIndex.fetch_xor(1, std::memory_order_acq_rel);
    }

    /// <summary>
    /// 重置后台 Arena (新任务开始前调用)
    /// </summary>
    void ResetBack() noexcept {
        GetBack().Reset();
    }

    // Const Accessors for Debug Stats
    const QuantumArena& GetActive() const noexcept {
        return m_arenas[m_activeIndex.load(std::memory_order_acquire)];
    }

    const QuantumArena& GetBack() const noexcept {
        return m_arenas[1 - m_activeIndex.load(std::memory_order_acquire)];
    }

    size_t GetUsedMemory() const noexcept { return GetActive().GetUsedBytes() + GetBack().GetUsedBytes(); }
    size_t GetTotalMemory() const noexcept { return GetActive().GetCapacity() + GetBack().GetCapacity(); }

private:
    QuantumArena m_arenas[2];
    std::atomic<int> m_activeIndex;
};

// ============================================================================
// ArenaConfig - 动态内存配额 (根据物理内存自动配置)
// ============================================================================

struct ArenaConfig {
    size_t scoutArenaSize;    // Scout Arena 大小
    size_t heavyArenaSize;    // Heavy Arena 大小 (每个)
    
    /// <summary>
    /// 根据系统物理内存自动计算最佳配置
    /// </summary>
    static ArenaConfig Detect() {
        MEMORYSTATUSEX statex;
        statex.dwLength = sizeof(statex);
        GlobalMemoryStatusEx(&statex);
        
        uint64_t totalPhys = statex.ullTotalPhys;
        ArenaConfig config;
        
        if (totalPhys <= 4ULL * 1024 * 1024 * 1024) {
            // <= 4GB: Lite 模式
            config.scoutArenaSize = 64 * 1024 * 1024;   // 64MB
            config.heavyArenaSize = 256 * 1024 * 1024;  // 256MB × 2
        } else if (totalPhys <= 8ULL * 1024 * 1024 * 1024) {
            // <= 8GB: 标准模式
            config.scoutArenaSize = 128 * 1024 * 1024;  // 128MB
            config.heavyArenaSize = 512 * 1024 * 1024;  // 512MB × 2
        } else {
            // > 8GB: 极限模式 (仅预留地址空间，按需提交)
            config.scoutArenaSize = 256 * 1024 * 1024;  // 256MB
            config.heavyArenaSize = 1024 * 1024 * 1024; // 1GB × 2
        }
        
        return config;
    }
    
    /// <summary>
    /// 获取配置模式名称 (用于调试)
    /// </summary>
    const wchar_t* GetModeName() const {
        if (heavyArenaSize <= 256 * 1024 * 1024) return L"Lite (4GB)";
        if (heavyArenaSize <= 512 * 1024 * 1024) return L"Standard (8GB)";
        return L"Pro (>8GB)";
    }
};

// ============================================================================
// TripleArenaPool - 三 Arena 管理器
// ============================================================================
// Scout 专用 1 个小型 Arena + Heavy 专用 2 个大型 Arena (Ping-Pong)
// ============================================================================

class TripleArenaPool {
public:
    TripleArenaPool() 
        : m_heavyIndex(0)
    {
        // 延迟初始化 - 构造函数不分配内存
    }
    
    /// <summary>
    /// 使用探测到的配置初始化
    /// </summary>
    void Initialize() {
        Initialize(ArenaConfig::Detect());
    }
    
    /// <summary>
    /// 使用指定配置初始化
    /// </summary>
    void Initialize(const ArenaConfig& config) {
        m_config = config;
        m_scoutArena = std::make_unique<QuantumArena>(config.scoutArenaSize);
        m_heavyArenas[0] = std::make_unique<QuantumArena>(config.heavyArenaSize);
        m_heavyArenas[1] = std::make_unique<QuantumArena>(config.heavyArenaSize);
        m_initialized = true;
    }
    
    // ============== Scout Arena ==============
    
    QuantumArena& GetScoutArena() {
        EnsureInitialized();
        return *m_scoutArena;
    }
    
    /// <summary>
    /// 重置 Scout Arena (每次任务开始前调用)
    /// </summary>
    void ResetScout() {
        if (m_scoutArena) m_scoutArena->Reset();
    }
    
    // ============== Heavy Arena (Ping-Pong) ==============
    
    QuantumArena& GetActiveHeavyArena() {
        EnsureInitialized();
        return *m_heavyArenas[m_heavyIndex.load(std::memory_order_acquire)];
    }
    
    QuantumArena& GetBackHeavyArena() {
        EnsureInitialized();
        return *m_heavyArenas[1 - m_heavyIndex.load(std::memory_order_acquire)];
    }
    
    /// <summary>
    /// 交换 Heavy Arena (解码完成后调用)
    /// </summary>
    void SwapHeavy() {
        m_heavyIndex.fetch_xor(1, std::memory_order_acq_rel);
    }
    
    /// <summary>
    /// 重置后台 Heavy Arena (新任务开始前调用)
    /// </summary>
    void ResetBackHeavy() {
        GetBackHeavyArena().Reset();
    }
    
    // ============== 调试统计 ==============
    
    const ArenaConfig& GetConfig() const { return m_config; }
    int GetHeavyIndex() const { return m_heavyIndex.load(std::memory_order_acquire); }
    
    size_t GetTotalCapacity() const {
        if (!m_initialized) return 0;
        return m_scoutArena->GetCapacity() + 
               m_heavyArenas[0]->GetCapacity() + 
               m_heavyArenas[1]->GetCapacity();
    }
    
    size_t GetTotalUsed() const {
        if (!m_initialized) return 0;
        return m_scoutArena->GetUsedBytes() + 
               m_heavyArenas[0]->GetUsedBytes() + 
               m_heavyArenas[1]->GetUsedBytes();
    }
    
    // Const accessors for HUD
    const QuantumArena* GetScoutArenaPtr() const { return m_scoutArena.get(); }
    const QuantumArena* GetHeavyArena0Ptr() const { return m_heavyArenas[0].get(); }
    // Aliases for Engine compatibility
    size_t GetUsedMemory() const { return GetTotalUsed(); }
    
    size_t GetTotalMemory() const {
        if (!m_initialized) return 0;
        return m_scoutArena->GetCapacity() + 
               m_heavyArenas[0]->GetCapacity() + 
               m_heavyArenas[1]->GetCapacity();
    }

private:
    void EnsureInitialized() {
        if (!m_initialized) Initialize();
    }
    
    ArenaConfig m_config;
    std::unique_ptr<QuantumArena> m_scoutArena;
    std::unique_ptr<QuantumArena> m_heavyArenas[2];
    std::atomic<int> m_heavyIndex;
    bool m_initialized = false;
};

// ============================================================================
// 别名 - 保持向后兼容
// ============================================================================
using MemoryArena = QuantumArena;

template <typename T>
using ArenaAllocator = std::pmr::polymorphic_allocator<T>;
