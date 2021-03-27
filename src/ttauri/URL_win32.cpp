// Copyright Take Vos 2019-2020.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at https://www.boost.org/LICENSE_1_0.txt)

#include "URL.hpp"
#include "strings.hpp"
#include "required.hpp"
#include "url_parser.hpp"
#include "ttauri/metadata.hpp"
#include <regex>

#include <Windows.h>
#include <ShlObj_core.h>

namespace tt {

/*! Convenience function for SHGetKnownFolderPath().
 *  Retrieves a full path of a known folder identified by the folder's KNOWNFOLDERID.
 *  See https://docs.microsoft.com/en-us/windows/win32/shell/knownfolderid#constants
 *
 * \param KNOWNFOLDERID folder_id.
 * \return The URL of the folder.
 */
static URL get_folder_by_id(const KNOWNFOLDERID &folder_id) noexcept
{
    PWSTR path = nullptr;
    if (SHGetKnownFolderPath(folder_id, 0, nullptr, &path) != S_OK) {
        // This should really never happen.
        tt_no_default();
    }
    URL folder = URL::urlFromWPath(path);
    CoTaskMemFree(path);
    return folder;
}

URL URL::urlFromCurrentWorkingDirectory() noexcept
{
    DWORD required_buffer_size = GetCurrentDirectoryW(0, nullptr);
    if (!required_buffer_size){
        tt_no_default();
    }
    std::unique_ptr<wchar_t[]> currentDirectory(new wchar_t[required_buffer_size]);
    GetCurrentDirectoryW(required_buffer_size, currentDirectory.get());
    return URL::urlFromWPath(currentDirectory.get());
}

URL URL::urlFromExecutableFile() noexcept
{
    std::wstring modulePath;
    auto bufferSize = MAX_PATH; // initial default value = 256
    // iterative buffer resizing to max value of 32768 (256*2^7)
    for (size_t i = 0; i < 7; ++i) {
        modulePath.resize(bufferSize);
        auto chars = GetModuleFileNameW(nullptr, &modulePath[0], bufferSize);
        if (chars < modulePath.length()) {
            modulePath.resize(chars);
            return URL::urlFromWPath(modulePath);
        } else {
            bufferSize *= 2;
        }
    }
    // throw io_error("The executable path exceeds 32768 chars length.");
    return URL();
}

URL URL::urlFromResourceDirectory() noexcept
{
    // Resource path, is the same directory as where the executable lives.
    static auto r = urlFromExecutableDirectory() / "resources";
    return r;
}

URL URL::urlFromApplicationDataDirectory() noexcept
{
    // FOLDERID_LocalAppData has the default path: %LOCALAPPDATA% (%USERPROFILE%\AppData\Local)
    return get_folder_by_id(FOLDERID_LocalAppData) / application_metadata().vendor / application_metadata().display_name;
}

URL URL::urlFromSystemfontDirectory() noexcept
{
    // FOLDERID_Fonts has the default path: %windir%\Fonts
    return get_folder_by_id(FOLDERID_Fonts);
}

std::vector<std::string> URL::filenamesByScanningDirectory(std::string_view path) noexcept
{
    auto searchPath = static_cast<std::string>(path);
    searchPath += '/';
    searchPath += '*';

    std::vector<std::string> filenames;
    WIN32_FIND_DATAW fileData;

    ttlet findHandle = FindFirstFileW(URL::nativeWPathFromPath(searchPath).data(), &fileData);
    if (findHandle == INVALID_HANDLE_VALUE) {
        return filenames;
    }

    do {
        auto filename = to_string(std::wstring(fileData.cFileName));

        if (filename == "." || filename == "..") {
            continue;
        } else if ((fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) > 0) {
            filename += '/';
        } else if ((fileData.dwFileAttributes & FILE_ATTRIBUTE_DEVICE) > 0) {
            continue;
        }

        filenames.push_back(filename);

    } while (FindNextFileW(findHandle, &fileData));

    FindClose(findHandle);
    return filenames;
}

}
