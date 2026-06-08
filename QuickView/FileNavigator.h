#pragma once
#include "pch.h"
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <cwctype>
#include <functional>
#include <Shlwapi.h>   // for StrCmpLogicalW
#include "EditState.h" // for g_runtime
#include "exif.h"      // for easyexif
#include "SupportedExtensions.h" // Unified supported extensions
#include "ArchiveVFS.h"

#pragma comment(lib, "Shlwapi.lib")

// [Directory Watcher] Custom window message posted when background scan completes
constexpr UINT WM_NAVIGATOR_DIR_CHANGED = WM_APP + 50;

// [ImageID Architecture] Stable content-based unique identifier
using ImageID = size_t;  // 64-bit path hash

// Helper: Compute normalized path hash (case-insensitive for Windows)
inline ImageID ComputePathHash(const std::wstring& path) {
    std::wstring normalized = path;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::towlower);
    return std::hash<std::wstring>{}(normalized);
}

class FileNavigator {
public:
    // Background scan result (produced by watcher thread, consumed by main thread)
    struct DirectoryScanResult {
        std::vector<std::wstring> files;
        std::vector<uintmax_t> sizes;
        std::vector<ImageID> ids;
    };

    FileNavigator() = default;
    ~FileNavigator() { StopDirectoryWatcher(); }
    FileNavigator(const FileNavigator&) = delete;
    FileNavigator& operator=(const FileNavigator&) = delete;

    void Initialize(const std::wstring& currentPath, HWND hwnd = nullptr);

    std::wstring Next(bool loop = true);
    std::wstring Previous(bool loop = true);
    std::wstring First();
    std::wstring Last();
    
    inline bool HitEnd() const { return m_hitEnd; }

    std::wstring GetCrossFolderMessage();
    std::wstring PeekNext() const;
    std::wstring PeekPrevious() const;
    void Refresh();
    
    // Status info
    inline size_t Count() const { return m_files.size(); }
    inline int Index() const { return m_currentIndex; }
    void SetIndex(int index);

    // Random Access (For Gallery Virtualization)
    const std::wstring& GetFile(int index) const;
    std::wstring GetResolvedPath(const std::wstring& requestedPath) const;
    int FindIndex(const std::wstring& path) const;

    // Virtual file system accessors
    inline bool IsVirtualPath(const std::wstring& path) const {
        return path.find(L"|") != std::wstring::npos;
    }

    // Parse virtual path avoiding exceptions
    static bool ParseVirtualPath(const std::wstring& path, std::wstring& outArchivePath, size_t& outIndex);

    inline QuickView::IArchive* GetArchive() const { return m_archive.get(); }
    inline const std::vector<std::wstring>& GetAllFiles() const { return m_files; }

    inline uintmax_t GetFileSize(int index) const {
        if (index < 0 || index >= (int)m_sizes.size()) return 0;
        return m_sizes[index];
    }
    
    // [ImageID] Get stable hash ID for image at index
    inline ImageID GetImageID(int index) const {
        if (index < 0 || index >= (int)m_ids.size()) return 0;
        return m_ids[index];
    }
    
    // [ImageID] Get hash ID for current image
    inline ImageID GetCurrentImageID() const {
        return GetImageID(m_currentIndex);
    }
    
    // [ImageID] Compute hash from path (for external use)
    static ImageID PathToImageID(const std::wstring& path) {
        return ComputePathHash(path);
    }

    // [Directory Watcher] Apply pending scan result from background thread (main thread only)
    void ApplyPendingScanResult();

    // Shared sort comparator for Entry vectors (used by Initialize and PerformDirectoryScan)
    struct SortEntry {
        std::wstring p;
        uintmax_t s;
        std::filesystem::file_time_type m;
        std::wstring t; // type (extension)
        std::string exifDate; // EXIF DateTaken
    };

    static void SortEntries(std::vector<SortEntry>& entries, int sortOrder, bool sortDesc);

private:
    static std::wstring_view GetPhysicalHostPath(std::wstring_view vfsPath);
    static std::vector<std::wstring> GetSortedSiblings(const std::filesystem::path& parentDir);
    std::wstring FindAdjacentFolderImage(bool next);

    // [Directory Watcher] Background directory monitoring
    DirectoryScanResult PerformDirectoryScan();
    void WatcherThreadProc();
    void StartDirectoryWatcher(const std::wstring& dirPath);
    void StopDirectoryWatcher();

    // Data Members
    std::vector<std::wstring> m_files;
    std::vector<uintmax_t> m_sizes;
    std::vector<ImageID> m_ids;  // [ImageID] Precomputed path hashes
    int m_currentIndex = -1;
    bool m_hitEnd = false;
    std::wstring m_crossFolderMessage;

    // VFS Support
    std::unique_ptr<QuickView::IArchive> m_archive;

    // [Directory Watcher] Background monitoring state
    HWND m_hwnd = nullptr;
    std::wstring m_watchedDir;
    HANDLE m_hCancelEvent = nullptr;
    std::thread m_watcherThread;
    std::mutex m_scanResultMutex;
    std::optional<DirectoryScanResult> m_pendingScanResult;

public:
    std::wstring m_archivePath;
};
