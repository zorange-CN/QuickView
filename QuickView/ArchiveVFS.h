#pragma once
#include "MappedFile.h"
#include <vector>
#include <string>
#include <string_view>
#include <cstdint>
#include <zlib.h>


namespace QuickView {

    // DOD structure for archive entries
    // Pack to 1 to ensure zero-padding wasted space.
    #pragma pack(push, 1)
    struct ArchiveEntry {
        uint32_t headerOffset;
        uint32_t nameOffset; // Absolute offset in mapped file to UTF-8 name
        uint32_t compSize;
        uint32_t uncompSize;
        uint16_t nameLen;    // Length of the UTF-8 name
        uint16_t method;     // Compression method
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

        // Lazy Evaluation: Decodes the UTF-8 name from the memory-mapped file on demand
        std::wstring GetEntryName(size_t index) const;

        // Zero-allocation UTF-8 string view directly into the mapped file
        std::string_view GetEntryNameView(size_t index) const;

        // Extract a specific entry directly into the provided external buffer.
        // Requires externalBuffer to be pre-allocated with a size >= entry.uncompSize
        bool ExtractEntry(size_t index, uint8_t* externalBuffer, size_t bufferSize) const;

    private:
        bool ParseCentralDirectory();

        MappedFile m_mappedFile;
        bool m_valid = false;

        std::vector<ArchiveEntry> m_entries;
    };

}
