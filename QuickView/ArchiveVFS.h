#pragma once
#include "MappedFile.h"
#include <vector>
#include <string>
#include <memory>
#include <cstdint>
#include <zlib.h>

namespace QuickView {

    // DOD structure for archive entries
    #pragma pack(push, 1)
    struct ArchiveEntry {
        uint32_t headerOffset;
        uint32_t compSize;
        uint32_t uncompSize;
        uint16_t nameHash; // Hash of the filename for quick DOD lookups
        uint16_t method;   // Compression method
    };
    #pragma pack(pop)

    class ZipArchive {
    public:
        ZipArchive(const std::wstring& path);
        ~ZipArchive() = default;

        bool IsValid() const { return m_mappedFile.IsValid() && m_valid; }

        // Returns the number of entries
        size_t GetEntryCount() const { return m_entries.size(); }

        // Gets a specific entry
        const ArchiveEntry& GetEntry(size_t index) const { return m_entries[index]; }

        // Gets original filename (if we decide to keep strings, else we construct virtual paths with index only)
        // For Zip VFS, we actually need to retain the original names *somewhere* to populate m_files with display names.
        // Let's store names separately to keep ArchiveEntry POD and small.
        const std::wstring& GetEntryName(size_t index) const { return m_names[index]; }

        // Decompress a specific entry into a raw memory buffer
        // The caller takes ownership of the returned buffer and must free it via 'delete[]'
        // Returns true if successful, with outData pointing to the allocated buffer and outSize containing the size
        bool ExtractEntry(size_t index, uint8_t** outData, size_t* outSize) const;

    private:
        bool ParseCentralDirectory();

        MappedFile m_mappedFile;
        bool m_valid = false;

        std::vector<ArchiveEntry> m_entries;
        std::vector<std::wstring> m_names;
    };

}
