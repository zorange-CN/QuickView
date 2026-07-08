#include <gtest/gtest.h>
#include "SupportedExtensions.h"

#include <set>
#include <string>

using namespace QuickView;

// The compile-time behavior is already pinned by the static_asserts in
// SupportedExtensions.h; these tests cover list integrity and runtime edges.

TEST(SupportedExtensionsTest, NoDuplicateExtensionsAcrossSegments) {
    // Every extension must appear exactly once across SUPPORTED + EXTENDED_RAW
    // (single source of truth: segments must not overlap).
    std::set<std::wstring> seen;
    for (const auto& e : SUPPORTED_EXTENSIONS) {
        EXPECT_TRUE(seen.insert(std::wstring(e)).second)
            << "duplicate in SUPPORTED_EXTENSIONS: " << std::string(e.begin(), e.end());
    }
    for (const auto& e : detail::EXTENDED_RAW_EXTENSIONS) {
        EXPECT_TRUE(seen.insert(std::wstring(e)).second)
            << "extended RAW ext duplicates a supported ext: " << std::string(e.begin(), e.end());
    }
}

TEST(SupportedExtensionsTest, AllListedExtensionsAreLowercaseWithDot) {
    auto check = [](std::wstring_view e) {
        ASSERT_GE(e.size(), 2u);
        EXPECT_EQ(e[0], L'.');
        for (wchar_t c : e.substr(1)) {
            EXPECT_TRUE((c >= L'a' && c <= L'z') || (c >= L'0' && c <= L'9'));
        }
    };
    for (const auto& e : SUPPORTED_EXTENSIONS) check(e);
    for (const auto& e : RAW_EXTENSIONS) check(e);
}

TEST(SupportedExtensionsTest, EveryBrowsableRawIsClassifiedAsRaw) {
    // Regression guard for the .crw class of bug: anything QuickView browses
    // as camera RAW must also classify as RAW.
    for (const auto& e : detail::CAMERA_RAW_EXTENSIONS) {
        EXPECT_TRUE(IsRawExtension(e)) << std::string(e.begin(), e.end());
    }
}

TEST(SupportedExtensionsTest, IsRawExtensionCases) {
    EXPECT_TRUE(IsRawExtension(L".crw"));  // was missing from the old IsRawFile
    EXPECT_TRUE(IsRawExtension(L".cr3"));
    EXPECT_TRUE(IsRawExtension(L".NEF"));  // case-insensitive
    EXPECT_TRUE(IsRawExtension(L".braw")); // extended (non-browsable) RAW
    EXPECT_FALSE(IsRawExtension(L".jpg"));
    EXPECT_FALSE(IsRawExtension(L".tif")); // TIFF is deliberately not RAW
    EXPECT_FALSE(IsRawExtension(L""));
    EXPECT_FALSE(IsRawExtension(L"arw")); // missing dot is not an extension
}

TEST(SupportedExtensionsTest, ExtensionOfEdgeCases) {
    EXPECT_EQ(ExtensionOf(L"C:\\photos\\IMG_0001.NEF"), L".NEF");
    EXPECT_EQ(ExtensionOf(L"IMG_0001.jpg"), L".jpg");
    EXPECT_EQ(ExtensionOf(L"noextension"), L"");
    EXPECT_EQ(ExtensionOf(L""), L"");
    EXPECT_EQ(ExtensionOf(L"C:\\dir.raw\\readme"), L"");          // dot only in directory
    EXPECT_EQ(ExtensionOf(L"C:\\a\\manga.cbz|12|page01.png"), L".png"); // VFS virtual path
    EXPECT_EQ(ExtensionOf(L".arw"), L".arw");                     // bare dotfile
    EXPECT_EQ(ExtensionOf(L"photo.bak.jpg"), L".jpg");            // last dot wins
}

TEST(SupportedExtensionsTest, IsRawPathCases) {
    EXPECT_TRUE(IsRawPath(L"C:\\Photos\\IMG_0001.NEF"));
    EXPECT_TRUE(IsRawPath(L"C:\\Photos\\img_0001.cr3"));
    EXPECT_FALSE(IsRawPath(L"C:\\Photos\\IMG_0001.JPG"));
    EXPECT_FALSE(IsRawPath(L"C:\\dir.nef\\file"));
}

TEST(SupportedExtensionsTest, HeifAndArchiveHelpers) {
    EXPECT_TRUE(IsHeifExtension(L".heic"));
    EXPECT_TRUE(IsHeifExtension(L".HIF"));
    EXPECT_FALSE(IsHeifExtension(L".jpg"));
    EXPECT_TRUE(IsHeifPath(L"C:\\x\\IMG_1234.HEIC"));

    EXPECT_TRUE(IsArchiveExtension(L".cbz"));
    EXPECT_TRUE(IsArchivePath(L"C:\\comics\\manga.CBZ"));
    EXPECT_FALSE(IsArchivePath(L"C:\\comics\\page.png"));
}

TEST(SupportedExtensionsTest, FilterStringIsWellFormed) {
    const std::wstring filter = GetSupportedExtensionsFilter();
    EXPECT_NE(filter.find(L"*.jpg"), std::wstring::npos);
    EXPECT_NE(filter.find(L"*.crw"), std::wstring::npos);
    EXPECT_NE(filter.find(L"*.rar"), std::wstring::npos);
    // Extended (non-browsable) RAW must NOT be offered by the folder filter
    EXPECT_EQ(filter.find(L"*.braw"), std::wstring::npos);
}
