#include "ArchiveVFS.h"
#include <algorithm>
#include <cstring>
#include <cwchar>
#include <cwctype>

namespace QuickView {

    // Simple hash function for wstring
    static uint16_t HashWString(const std::wstring& str) {
        uint32_t hash = 5381;
        for (wchar_t c : str) {
            hash = ((hash << 5) + hash) + std::towlower(c); // hash * 33 + c
        }
        return (uint16_t)(hash & 0xFFFF);
    }

    // Helper to convert UTF-8 to UTF-16
    static std::wstring Utf8ToUtf16(const char* utf8, size_t len) {
        if (len == 0) return L"";
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8, (int)len, NULL, 0);
        std::wstring wstrTo(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, utf8, (int)len, &wstrTo[0], size_needed);
        return wstrTo;
    }

    ZipArchive::ZipArchive(const std::wstring& path) : m_mappedFile(path) {
        if (m_mappedFile.IsValid()) {
            m_valid = ParseCentralDirectory();
        }
    }

    bool ZipArchive::ParseCentralDirectory() {
        const uint8_t* data = m_mappedFile.data();
        size_t size = m_mappedFile.size();
        if (size < 22) return false; // Minimum size for EOCD

        // 1. Find EOCD (End of Central Directory) signature 0x06054b50
        // Search backwards from the end
        size_t eocdOffset = 0;
        bool foundEocd = false;

        // EOCD is at least 22 bytes, max comment length is 65535, so search max 65535+22 bytes
        size_t searchLimit = (size > 65557) ? 65557 : size;
        for (size_t i = 0; i < searchLimit - 21; ++i) {
            size_t p = size - 22 - i;
            if (data[p] == 0x50 && data[p+1] == 0x4b && data[p+2] == 0x05 && data[p+3] == 0x06) {
                eocdOffset = p;
                foundEocd = true;
                break;
            }
        }

        if (!foundEocd) return false;

        // 2. Parse EOCD to get Central Directory offset and size
        // EOCD offset 16 is Central Directory offset
        uint32_t cdOffset = *(uint32_t*)(data + eocdOffset + 16);
        uint16_t numEntries = *(uint16_t*)(data + eocdOffset + 10);

        if (cdOffset >= size) return false;

        // 3. Parse Central Directory records
        m_entries.reserve(numEntries);
        m_names.reserve(numEntries);

        size_t currentOffset = cdOffset;
        for (uint16_t i = 0; i < numEntries; ++i) {
            if (currentOffset + 46 > size) break; // CD header is at least 46 bytes

            // Check CD signature 0x02014b50
            if (data[currentOffset] != 0x50 || data[currentOffset+1] != 0x4b ||
                data[currentOffset+2] != 0x01 || data[currentOffset+3] != 0x02) {
                break;
            }

            uint16_t method = *(uint16_t*)(data + currentOffset + 10);
            uint32_t compSize = *(uint32_t*)(data + currentOffset + 20);
            uint32_t uncompSize = *(uint32_t*)(data + currentOffset + 24);
            uint16_t nameLen = *(uint16_t*)(data + currentOffset + 28);
            uint16_t extraLen = *(uint16_t*)(data + currentOffset + 30);
            uint16_t commentLen = *(uint16_t*)(data + currentOffset + 32);
            uint32_t headerOffset = *(uint32_t*)(data + currentOffset + 42);

            // Extract Name
            std::wstring wname = L"";
            if (nameLen > 0 && currentOffset + 46 + nameLen <= size) {
                const char* namePtr = (const char*)(data + currentOffset + 46);
                wname = Utf8ToUtf16(namePtr, nameLen);
            }

            // Only add if it's a file (not directory)
            if (uncompSize > 0 && !wname.empty() && wname.back() != L'/') {
                ArchiveEntry entry;
                entry.headerOffset = headerOffset;
                entry.compSize = compSize;
                entry.uncompSize = uncompSize;
                entry.nameHash = HashWString(wname);
                entry.method = method;

                m_entries.push_back(entry);
                m_names.push_back(wname);
            }

            // Move to next CD record
            currentOffset += 46 + nameLen + extraLen + commentLen;
        }

        return !m_entries.empty();
    }

    bool ZipArchive::ExtractEntry(size_t index, uint8_t** outData, size_t* outSize) const {
        if (index >= m_entries.size() || !m_mappedFile.IsValid()) return false;

        const ArchiveEntry& entry = m_entries[index];
        const uint8_t* data = m_mappedFile.data();
        size_t size = m_mappedFile.size();

        if (entry.headerOffset + 30 > size) return false;

        // Verify Local File Header signature 0x04034b50
        size_t lfhOffset = entry.headerOffset;
        if (data[lfhOffset] != 0x50 || data[lfhOffset+1] != 0x4b ||
            data[lfhOffset+2] != 0x03 || data[lfhOffset+3] != 0x04) {
            return false;
        }

        uint16_t nameLen = *(uint16_t*)(data + lfhOffset + 26);
        uint16_t extraLen = *(uint16_t*)(data + lfhOffset + 28);

        size_t payloadOffset = lfhOffset + 30 + nameLen + extraLen;
        if (payloadOffset > size || entry.compSize > size - payloadOffset) return false;

        // Allocate output buffer
        uint8_t* outBuffer = new uint8_t[entry.uncompSize];
        if (!outBuffer) return false;

        if (entry.method == 0) {
            // Store (No compression)
            if (entry.compSize != entry.uncompSize) {
                delete[] outBuffer;
                return false;
            }
            std::memcpy(outBuffer, data + payloadOffset, entry.uncompSize);
        } else if (entry.method == 8) {
            // Deflate
            z_stream strm = {};
            strm.next_in = (Bytef*)(data + payloadOffset);
            strm.avail_in = entry.compSize;
            strm.next_out = outBuffer;
            strm.avail_out = entry.uncompSize;

            // -MAX_WBITS for raw deflate stream (no zlib header inside zip)
            if (inflateInit2(&strm, -MAX_WBITS) != Z_OK) {
                delete[] outBuffer;
                return false;
            }

            int ret = inflate(&strm, Z_FINISH);
            inflateEnd(&strm);

            if (ret != Z_STREAM_END && ret != Z_OK) {
                delete[] outBuffer;
                return false;
            }
        } else {
            // Unsupported compression method
            delete[] outBuffer;
            return false;
        }

        *outData = outBuffer;
        *outSize = entry.uncompSize;
        return true;
    }

}
