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

    void Initialize(const std::wstring& currentPath, HWND hwnd = nullptr) {
        // Stop existing watcher before mutating state
        StopDirectoryWatcher();
        if (hwnd) m_hwnd = hwnd;

        namespace fs = std::filesystem;
        
        std::wstring archivePart;
        size_t vfsIndex = (size_t)-1;
        bool isVfsInput = ParseVirtualPath(currentPath, archivePart, vfsIndex);
        
        fs::path p = isVfsInput ? fs::path(archivePart) : fs::path(currentPath);
        if (!fs::exists(p)) return;

        // VFS State Teardown
        m_archive.reset();
        m_archivePath.clear();

        m_files.clear();
        m_currentIndex = -1;

        const bool isDirectory = fs::is_directory(p);

        // If a directory is passed in, scan it directly. Otherwise scan the parent directory.
        fs::path dir = isDirectory ? p : p.parent_path();
        if (dir.empty()) return;

        // Supported extensions (comprehensive list including RAW formats)
        // using QuickView::SUPPORTED_EXTENSIONS from SupportedExtensions.h

        std::error_code ec;
        if (fs::exists(p, ec) && fs::is_directory(p, ec)) {
            // Already handled isDirectory
        }

        m_sizes.clear();

        // VFS Support for Archives
        std::wstring pExt = p.extension().wstring();
        std::transform(pExt.begin(), pExt.end(), pExt.begin(), [](wchar_t c){ return std::towlower(c); });

        if (pExt == L".cbz" || pExt == L".zip" || pExt == L".cbr" || pExt == L".rar") {
            // Load from Archive VFS
            m_archivePath = p.wstring();
            if (pExt == L".cbr" || pExt == L".rar") {
                m_archive = std::make_unique<QuickView::RarArchive>(m_archivePath);
            } else {
                m_archive = std::make_unique<QuickView::ZipArchive>(m_archivePath);
            }

            if (m_archive && m_archive->IsValid()) {
                size_t numEntries = m_archive->GetEntryCount();
                for (size_t i = 0; i < numEntries; ++i) {
                    const QuickView::ArchiveEntry& entry = m_archive->GetEntry(i);
                    // Zero-allocation extension check using string_view
                    std::string_view nameView = m_archive->GetEntryNameView(i);
                    
                    size_t lastDot = nameView.find_last_of('.');
                    if (lastDot != std::string_view::npos) {
                        std::string_view extUtf8 = nameView.substr(lastDot);
                        
                        // Fast ASCII to wide lower-case conversion for extension
                        std::wstring ext;
                        ext.reserve(extUtf8.length());
                        for (char c : extUtf8) {
                            ext.push_back(std::towlower((wchar_t)(uint8_t)c));
                        }

                        for (const auto& supp : QuickView::SUPPORTED_EXTENSIONS) {
                            if (ext == supp) {
                                // Full conversion only for supported image types
                                std::wstring name = m_archive->GetEntryName(i);
                                std::wstring virtualPath = m_archivePath + L"|" + std::to_wstring(i) + L"|" + name;
                                m_files.push_back(virtualPath);
                                m_sizes.push_back(entry.uncompSize);
                                break;
                            }
                        }
                    }
                }
            }
        } else {
            for (const auto& entry : fs::directory_iterator(dir, ec)) {
                if (entry.is_regular_file(ec)) {
                    std::wstring ext = entry.path().extension().wstring();
                    std::transform(ext.begin(), ext.end(), ext.begin(), [](wchar_t c){ return std::towlower(c); });

                    for (const auto& supp : QuickView::SUPPORTED_EXTENSIONS) {
                        if (ext == supp) {
                            m_files.push_back(entry.path().wstring());
                            // Cache file size for Scout Lane decision
                            m_sizes.push_back(entry.file_size(ec));
                            break;
                        }
                    }
                }
            }
        }

        
        std::vector<SortEntry> entries;
        entries.reserve(m_files.size());
        namespace fs2 = std::filesystem;
        for(size_t i=0; i<m_files.size(); ++i) {
            SortEntry e;
            e.p = m_files[i];
            e.s = m_sizes[i];
            std::error_code ec;
            
            // For virtual paths, use the archive file's timestamp
            if (IsVirtualPath(e.p)) {
                e.m = fs2::last_write_time(p, ec);
            } else {
                e.m = fs2::last_write_time(e.p, ec);
            }

            e.t = fs2::path(e.p).extension().wstring();
            std::transform(e.t.begin(), e.t.end(), e.t.begin(), [](wchar_t c){ return std::towlower(c); });

            // Only parse EXIF date if specifically requested and it's a real file
            if (g_runtime.SortOrder == 3 && !IsVirtualPath(e.p)) {
                 FILE *fp = nullptr;
                 _wfopen_s(&fp, e.p.c_str(), L"rb");
                 if (fp) {
                     unsigned char buf[65536];
                     size_t bytes = fread(buf, 1, sizeof(buf), fp);
                     fclose(fp);
                     if (bytes > 0) {
                         easyexif::EXIFInfo info;
                         if (info.parseFrom(buf, (unsigned)bytes) == PARSE_EXIF_SUCCESS) {
                             e.exifDate = info.DateTimeOriginal;
                         }
                     }
                 }
            }

            entries.push_back(e);
        }
        
        int sortOrder = g_runtime.SortOrder;
        bool sortDesc = g_runtime.SortDescending;

        SortEntries(entries, sortOrder, sortDesc);
        
        // Write back
        m_files.clear();
        m_sizes.clear();
        for(const auto& e : entries) {
            m_files.push_back(e.p);
            m_sizes.push_back(e.s);
        }
        
        // [ImageID] Compute stable hash IDs for all files
        m_ids.clear();
        m_ids.reserve(m_files.size());
        for (const auto& f : m_files) {
            m_ids.push_back(ComputePathHash(f));
        }


        // Find current index
        if (!isDirectory) {
            std::wstring currentFull = isVfsInput ? currentPath : p.wstring();

            // Fix initial page load for Archive VFS
            if (m_archive && m_archive->IsValid() && (isVfsInput || currentFull == m_archivePath)) {
                if (!m_files.empty()) {
                    m_currentIndex = 0; // Default to first page in archive
                }
            } else {
                for (size_t i = 0; i < m_files.size(); ++i) {
                    if (m_files[i] == currentFull) {
                        m_currentIndex = (int)i;
                        break;
                    }
                }
            }
        }

        // [Directory Watcher] Start monitoring for non-VFS real directories
        if (!m_archive && m_hwnd) {
            std::wstring watchDir = dir.wstring();
            if (!watchDir.empty()) {
                StartDirectoryWatcher(watchDir);
            }
        }
    }

    // Legacy support: We ignore `loop` and strictly use NavLoopMode instead
    std::wstring Next(bool /*unused*/ = true) {
        if (m_files.empty()) return L"";

        if (m_currentIndex >= (int)m_files.size() - 1) {
            if (g_runtime.NavTraverse) {
                // Through folders
                std::wstring nextFolderImg = FindAdjacentFolderImage(true);
                if (!nextFolderImg.empty()) {
                    m_crossFolderMessage = L">>> Entering [" + std::filesystem::path(nextFolderImg).parent_path().filename().wstring() + L"] >>>";
                    return nextFolderImg;
                }
            }

            if (g_runtime.NavLoop) {
                // Loop
                m_hitEnd = true; // Signal OSD
                m_currentIndex = 0;
                return m_files[m_currentIndex];
            } else {
                // Stop at end
                m_hitEnd = true;
                return L"";
            }
        }

        m_hitEnd = false;
        m_currentIndex++;
        return m_files[m_currentIndex];
    }

    std::wstring Previous(bool /*unused*/ = true) {
        if (m_files.empty()) return L"";

        if (m_currentIndex <= 0) {
            if (g_runtime.NavTraverse) {
                // Through folders
                std::wstring prevFolderImg = FindAdjacentFolderImage(false);
                if (!prevFolderImg.empty()) {
                    m_crossFolderMessage = L"<<< Entering [" + std::filesystem::path(prevFolderImg).parent_path().filename().wstring() + L"] <<<";
                    return prevFolderImg;
                }
            }

            if (g_runtime.NavLoop) {
                // Loop
                m_hitEnd = true; // Signal OSD
                m_currentIndex = (int)m_files.size() - 1;
                return m_files[m_currentIndex];
            } else {
                // Stop at start
                m_hitEnd = true;
                return L"";
            }
        }

        m_hitEnd = false;
        m_currentIndex--;
        return m_files[m_currentIndex];
    }

    std::wstring First() {
        if (m_files.empty()) return L"";
        m_hitEnd = false;
        m_currentIndex = 0;
        return m_files[m_currentIndex];
    }

    std::wstring Last() {
        if (m_files.empty()) return L"";
        m_hitEnd = false;
        m_currentIndex = (int)m_files.size() - 1;
        return m_files[m_currentIndex];
    }
    
    bool HitEnd() const { return m_hitEnd; }

    std::wstring GetCrossFolderMessage() {
        std::wstring msg = m_crossFolderMessage;
        m_crossFolderMessage.clear(); // Consume
        return msg;
    }

    std::wstring PeekNext() const {
        if (m_files.empty()) return L"";
        size_t nextIdx = (m_currentIndex + 1) % m_files.size();
        return m_files[nextIdx];
    }

    std::wstring PeekPrevious() const {
        if (m_files.empty()) return L"";
        size_t prevIdx = (m_currentIndex - 1 + m_files.size()) % m_files.size();
        return m_files[prevIdx];
    }
    
    // [Fix] Refresh metadata for current file (e.g. after Save)
    void Refresh() {
        if (m_currentIndex >= 0 && m_currentIndex < (int)m_files.size()) {
            std::error_code ec;
            m_sizes[m_currentIndex] = std::filesystem::file_size(m_files[m_currentIndex], ec);
        }
    }
    
    // Status info
    // Status info
    size_t Count() const { return m_files.size(); }
    int Index() const { return m_currentIndex; }
    void SetIndex(int index) {
        if (index >= 0 && index < (int)m_files.size()) {
            m_currentIndex = index;
            m_hitEnd = false;
        }
    }

    // Random Access (For Gallery Virtualization)
    const std::wstring& GetFile(int index) const {
        static std::wstring empty;
        if (index < 0 || index >= (int)m_files.size()) return empty;
        return m_files[index];
    }

    std::wstring GetResolvedPath(const std::wstring& requestedPath) const {
        if (m_archive && m_archive->IsValid() && requestedPath == m_archivePath) {
            if (!m_files.empty() && m_currentIndex >= 0 && m_currentIndex < (int)m_files.size()) {
                return m_files[m_currentIndex];
            }
        }
        return requestedPath;
    }

    int FindIndex(const std::wstring& path) const {
        // Handle virtual path matching where we might be passed just the archive path from OS
        if (m_archive && m_archive->IsValid()) {
            if (path == m_archivePath) {
                return 0; // Return first entry if they try to open the archive itself
            }
        }

        auto it = std::find(m_files.begin(), m_files.end(), path);
        if (it != m_files.end()) return (int)std::distance(m_files.begin(), it);
        return -1;
    }

    // Virtual file system accessors
    bool IsVirtualPath(const std::wstring& path) const {
        return path.find(L"|") != std::wstring::npos;
    }

    // Parse virtual path avoiding exceptions
    static bool ParseVirtualPath(const std::wstring& path, std::wstring& outArchivePath, size_t& outIndex) {
        size_t firstPipe = path.find(L"|");
        if (firstPipe == std::wstring::npos) return false;

        size_t secondPipe = path.find(L"|", firstPipe + 1);
        if (secondPipe == std::wstring::npos) return false;

        outArchivePath = path.substr(0, firstPipe);
        size_t index = 0;
        size_t len = secondPipe - firstPipe - 1;
        if (len == 0) return false;
        
        for (size_t i = firstPipe + 1; i < secondPipe; ++i) {
            wchar_t c = path[i];
            if (c >= L'0' && c <= L'9') {
                index = index * 10 + (c - L'0');
            } else {
                return false; // Invalid character
            }
        }
        outIndex = index;
        return true;
    }

    QuickView::IArchive* GetArchive() const { return m_archive.get(); }

    const std::vector<std::wstring>& GetAllFiles() const { return m_files; }

    uintmax_t GetFileSize(int index) const {
        if (index < 0 || index >= (int)m_sizes.size()) return 0;
        return m_sizes[index];
    }
    
    // [ImageID] Get stable hash ID for image at index
    ImageID GetImageID(int index) const {
        if (index < 0 || index >= (int)m_ids.size()) return 0;
        return m_ids[index];
    }
    
    // [ImageID] Get hash ID for current image
    ImageID GetCurrentImageID() const {
        return GetImageID(m_currentIndex);
    }
    
    // [ImageID] Compute hash from path (for external use)
    static ImageID PathToImageID(const std::wstring& path) {
        return ComputePathHash(path);
    }

    // [Directory Watcher] Apply pending scan result from background thread (main thread only)
    void ApplyPendingScanResult() {
        DirectoryScanResult result;
        {
            std::lock_guard<std::mutex> lock(m_scanResultMutex);
            if (!m_pendingScanResult) return;
            result = std::move(*m_pendingScanResult);
            m_pendingScanResult.reset();
        }

        // Cache current path BEFORE swap for index reconciliation
        std::wstring currentPath;
        if (m_currentIndex >= 0 && m_currentIndex < (int)m_files.size()) {
            currentPath = m_files[m_currentIndex];
        }

        // O(1) swap
        m_files = std::move(result.files);
        m_sizes = std::move(result.sizes);
        m_ids = std::move(result.ids);

        // Relocate current index in new list
        if (!currentPath.empty()) {
            auto it = std::find(m_files.begin(), m_files.end(), currentPath);
            if (it != m_files.end()) {
                m_currentIndex = (int)std::distance(m_files.begin(), it);
            } else {
                // Fallback: file was deleted externally — clamp to nearest valid index
                if (m_currentIndex >= (int)m_files.size()) {
                    m_currentIndex = (int)m_files.size() - 1;
                }
                if (m_files.empty()) m_currentIndex = -1;
            }
        }
    }

private:
    // Shared sort comparator for Entry vectors (used by Initialize and PerformDirectoryScan)
    struct SortEntry {
        std::wstring p;
        uintmax_t s;
        std::filesystem::file_time_type m;
        std::wstring t; // type (extension)
        std::string exifDate; // EXIF DateTaken
    };

    static void SortEntries(std::vector<SortEntry>& entries, int sortOrder, bool sortDesc) {
        std::sort(entries.begin(), entries.end(), [sortOrder, sortDesc](const SortEntry& a, const SortEntry& b){
            int cmp = 0;
            switch (sortOrder) {
                case 1: // Name
                case 0: // Auto (Use Name Natural Sort)
                    cmp = StrCmpLogicalW(a.p.c_str(), b.p.c_str());
                    break;
                case 2: // Modified
                    if (a.m < b.m) cmp = -1;
                    else if (a.m > b.m) cmp = 1;
                    else cmp = StrCmpLogicalW(a.p.c_str(), b.p.c_str()); // Fallback
                    break;
                case 3: // Date Taken
                    if (a.exifDate.empty() && !b.exifDate.empty()) cmp = 1; // Empty goes last
                    else if (!a.exifDate.empty() && b.exifDate.empty()) cmp = -1;
                    else {
                        cmp = a.exifDate.compare(b.exifDate);
                        if (cmp == 0) cmp = StrCmpLogicalW(a.p.c_str(), b.p.c_str());
                    }
                    break;
                case 4: // Size
                    if (a.s < b.s) cmp = -1;
                    else if (a.s > b.s) cmp = 1;
                    else cmp = StrCmpLogicalW(a.p.c_str(), b.p.c_str());
                    break;
                case 5: // Type
                    cmp = StrCmpLogicalW(a.t.c_str(), b.t.c_str());
                    if (cmp == 0) cmp = StrCmpLogicalW(a.p.c_str(), b.p.c_str());
                    break;
            }

            if (sortDesc) return cmp > 0;
            return cmp < 0;
        });
    }

    // Helper to get physical host path from a potentially virtual path, zero allocations
    static std::wstring_view GetPhysicalHostPath(std::wstring_view vfsPath) {
        auto pos = vfsPath.find(L'|');
        if (pos != std::wstring_view::npos) {
            return vfsPath.substr(0, pos);
        }
        return vfsPath;
    }

    std::wstring FindAdjacentFolderImage(bool next) {
        if (m_files.empty()) return L"";

        namespace fs = std::filesystem;

        // Determine logical "current container" (directory or archive)
        fs::path currentContainer;
        if (m_archive && m_archive->IsValid()) {
            currentContainer = m_archivePath;
        } else {
            currentContainer = fs::path(m_files[0]).parent_path();
        }

        // 1. Through Subfolders: Only if current is a real directory
        if (next && fs::is_directory(currentContainer)) {
            std::error_code ec;
            std::vector<std::wstring> subfolders;
            for (const auto& entry : fs::directory_iterator(currentContainer, ec)) {
                if (entry.is_directory(ec)) {
                    subfolders.push_back(entry.path().wstring());
                } else if (entry.is_regular_file(ec)) {
                    std::wstring ext = entry.path().extension().wstring();
                    std::transform(ext.begin(), ext.end(), ext.begin(), [](wchar_t c){ return std::towlower(c); });
                    if (ext == L".cbz" || ext == L".zip" || ext == L".cbr" || ext == L".rar") {
                        subfolders.push_back(entry.path().wstring());
                    }
                }
            }
            if (!subfolders.empty()) {
                std::sort(subfolders.begin(), subfolders.end(), [](const std::wstring& a, const std::wstring& b) {
                    return StrCmpLogicalW(a.c_str(), b.c_str()) < 0;
                });
                for (const auto& sub : subfolders) {
                    FileNavigator tempNav;
                    tempNav.Initialize(sub);
                    if (tempNav.Count() > 0) return tempNav.First();
                }
            }
        }

        // 2. Siblings navigation: Find next folder/archive in the parent directory
        fs::path parentDir = currentContainer.parent_path();
        if (parentDir.empty() || parentDir == currentContainer) return L"";

        std::vector<std::wstring> siblings;
        std::error_code ec_fold;
        for (const auto& entry : fs::directory_iterator(parentDir, ec_fold)) {
            if (entry.is_directory(ec_fold)) {
                siblings.push_back(entry.path().wstring());
            } else if (entry.is_regular_file(ec_fold)) {
                std::wstring ext = entry.path().extension().wstring();
                std::transform(ext.begin(), ext.end(), ext.begin(), [](wchar_t c){ return std::towlower(c); });
                if (ext == L".cbz" || ext == L".zip" || ext == L".cbr" || ext == L".rar") {
                    siblings.push_back(entry.path().wstring());
                }
            }
        }

        if (siblings.empty()) return L"";

        std::sort(siblings.begin(), siblings.end(), [](const std::wstring& a, const std::wstring& b){
             return StrCmpLogicalW(a.c_str(), b.c_str()) < 0;
        });

        std::wstring currentStr = currentContainer.wstring();
        auto it = std::find(siblings.begin(), siblings.end(), currentStr);
        int idx = (it == siblings.end()) ? -1 : (int)std::distance(siblings.begin(), it);

        int startIdx = idx;
        while (true) {
            if (next) idx++; else idx--;

            // Boundary logic
            if (idx < 0 || idx >= (int)siblings.size()) {
                if (g_runtime.NavLoop) {
                    idx = (idx < 0) ? (int)siblings.size() - 1 : 0;
                } else {
                    return L""; 
                }
            }
            
            if (idx == startIdx) break; 

            FileNavigator tempNav;
            tempNav.Initialize(siblings[idx]);
            if (tempNav.Count() > 0) {
                 return next ? tempNav.First() : tempNav.Last();
            }
        }

        return L"";
    }

    // =========================================================================
    // [Directory Watcher] Background directory monitoring
    // =========================================================================

    // Perform a full directory scan on the watcher thread (no VFS, no shared state mutation)
    DirectoryScanResult PerformDirectoryScan() {
        DirectoryScanResult result;
        namespace fs = std::filesystem;
        std::error_code ec;
        
        for (const auto& entry : fs::directory_iterator(m_watchedDir, ec)) {
            if (entry.is_regular_file(ec)) {
                std::wstring ext = entry.path().extension().wstring();
                std::transform(ext.begin(), ext.end(), ext.begin(), [](wchar_t c){ return std::towlower(c); });

                for (const auto& supp : QuickView::SUPPORTED_EXTENSIONS) {
                    if (ext == supp) {
                        result.files.push_back(entry.path().wstring());
                        result.sizes.push_back(entry.file_size(ec));
                        break;
                    }
                }
            }
        }

        // Sort (same logic as Initialize, but without VFS virtual path handling)
        std::vector<SortEntry> entries;
        entries.reserve(result.files.size());
        for (size_t i = 0; i < result.files.size(); ++i) {
            SortEntry e;
            e.p = result.files[i];
            e.s = result.sizes[i];
            std::error_code ec2;
            e.m = fs::last_write_time(e.p, ec2);
            e.t = fs::path(e.p).extension().wstring();
            std::transform(e.t.begin(), e.t.end(), e.t.begin(), [](wchar_t c){ return std::towlower(c); });

            int sortOrder = g_runtime.SortOrder;
            if (sortOrder == 3) {
                FILE* fp = nullptr;
                _wfopen_s(&fp, e.p.c_str(), L"rb");
                if (fp) {
                    unsigned char buf[65536];
                    size_t bytes = fread(buf, 1, sizeof(buf), fp);
                    fclose(fp);
                    if (bytes > 0) {
                        easyexif::EXIFInfo info;
                        if (info.parseFrom(buf, (unsigned)bytes) == PARSE_EXIF_SUCCESS) {
                            e.exifDate = info.DateTimeOriginal;
                        }
                    }
                }
            }

            entries.push_back(e);
        }

        int sortOrder = g_runtime.SortOrder;
        bool sortDesc = g_runtime.SortDescending;
        SortEntries(entries, sortOrder, sortDesc);

        result.files.clear();
        result.sizes.clear();
        for (const auto& e : entries) {
            result.files.push_back(e.p);
            result.sizes.push_back(e.s);
        }

        result.ids.reserve(result.files.size());
        for (const auto& f : result.files) {
            result.ids.push_back(ComputePathHash(f));
        }

        return result;
    }

    // Background thread entry point: monitors directory via FindFirstChangeNotificationW
    void WatcherThreadProc() {
        HANDLE hNotify = FindFirstChangeNotificationW(
            m_watchedDir.c_str(),
            FALSE,                          // Non-recursive (current directory only)
            FILE_NOTIFY_CHANGE_FILE_NAME    // File create, delete, rename only
        );
        if (hNotify == INVALID_HANDLE_VALUE) return;

        HANDLE handles[2] = { hNotify, m_hCancelEvent };

        while (true) {
            DWORD wait = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
            if (wait == WAIT_OBJECT_0 + 1) break;  // Cancel event signaled
            if (wait != WAIT_OBJECT_0) break;       // Error or abandoned

            // === Coalescing / Debounce Loop (300ms) ===
            // Drain all rapid-fire events until 300ms of silence
            bool cancelled = false;
            while (true) {
                if (!FindNextChangeNotification(hNotify)) {
                    cancelled = true; // Directory removed or device ejected
                    break;
                }
                DWORD r = WaitForMultipleObjects(2, handles, FALSE, 300);
                if (r == WAIT_OBJECT_0 + 1) { cancelled = true; break; } // Cancel
                if (r == WAIT_TIMEOUT) break; // 300ms silence — proceed to scan
                if (r != WAIT_OBJECT_0) { cancelled = true; break; } // Error
                // r == WAIT_OBJECT_0: more changes arrived, loop again (reset timer)
            }
            if (cancelled) break;

            // === Background Scan (on this thread, zero UI impact) ===
            auto scanResult = PerformDirectoryScan();

            {
                std::lock_guard<std::mutex> lock(m_scanResultMutex);
                m_pendingScanResult = std::move(scanResult);
            }
            PostMessageW(m_hwnd, WM_NAVIGATOR_DIR_CHANGED, 0, 0);

            // Re-arm for next batch of changes
            if (!FindNextChangeNotification(hNotify)) break; // Directory gone
        }

        FindCloseChangeNotification(hNotify);
    }

    void StartDirectoryWatcher(const std::wstring& dirPath) {
        m_watchedDir = dirPath;

        // Create manual-reset event (initially non-signaled) for graceful shutdown
        m_hCancelEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!m_hCancelEvent) return;

        m_watcherThread = std::thread(&FileNavigator::WatcherThreadProc, this);
    }

    void StopDirectoryWatcher() {
        if (m_hCancelEvent) {
            SetEvent(m_hCancelEvent); // Signal cancellation
        }
        if (m_watcherThread.joinable()) {
            m_watcherThread.join();   // Wait for clean exit
        }
        if (m_hCancelEvent) {
            CloseHandle(m_hCancelEvent);
            m_hCancelEvent = nullptr;
        }
        // Discard any unprocessed result
        std::lock_guard<std::mutex> lock(m_scanResultMutex);
        m_pendingScanResult.reset();
    }

    // =========================================================================
    // Data Members
    // =========================================================================

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
