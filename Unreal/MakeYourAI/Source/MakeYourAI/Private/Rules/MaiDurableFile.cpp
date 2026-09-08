#include "Rules/MaiDurableFile.h"
#include <atomic>
#include <cerrno>
#include <filesystem>
#include <fstream>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif
namespace mai {
namespace {
constexpr std::size_t MaxBytes = 8 * 1024 * 1024;
std::atomic<unsigned> Sequence{0};
std::string Utf8(const std::filesystem::path& path) {
    const auto text = path.u8string();
    return std::string(reinterpret_cast<const char*>(text.data()), text.size());
}
struct StagingFiles {
    std::filesystem::path data, backup;
    ~StagingFiles() {
        std::error_code ec;
        if (!data.empty()) std::filesystem::remove(data, ec);
        if (!backup.empty()) std::filesystem::remove(backup, ec);
    }
};
}
bool ReadBoundedFile(const std::string& path, std::string& bytes, std::string& error) {
    bytes.clear(); error.clear();
    std::error_code ec;
    const auto p = std::filesystem::u8path(path);
    const auto size = std::filesystem::file_size(p, ec);
    if (ec || size > MaxBytes) { error = "Save is unavailable or exceeds 8 MiB"; return false; }
    std::ifstream file(p, std::ios::binary);
    if (!file) { error = "Save could not be opened"; return false; }
    bytes.resize(static_cast<std::size_t>(size));
    file.read(bytes.data(), static_cast<std::streamsize>(size));
    if (file.gcount() != static_cast<std::streamsize>(size) ||
        file.peek() != std::char_traits<char>::eof() || file.bad()) {
        bytes.clear(); error = "Incomplete save read"; return false;
    }
    return true;
}
bool ReadValidatedSave(const std::string& path, const SaveValidator& validate,
                       std::string& bytes, bool& usedBackup, std::string& error) {
    bytes.clear(); error.clear(); usedBackup = false;
    if (!validate) { error = "Save validation is required"; return false; }
    std::string candidate, primaryError, backupError;
    if (ReadBoundedFile(path, candidate, primaryError) && validate(candidate, primaryError)) {
        bytes = std::move(candidate); return true;
    }
    if (ReadBoundedFile(path + ".bak", candidate, backupError) && validate(candidate, backupError)) {
        bytes = std::move(candidate); usedBackup = true; return true;
    }
    error = "No valid save. Primary: " + primaryError + "; backup: " + backupError;
    return false;
}
bool WriteDurableFile(const std::string& path, const std::string& bytes,
                      std::string& error, const SaveValidator& validate) {
    error.clear();
    if (bytes.empty() || bytes.size() > MaxBytes) { error = "Invalid save size"; return false; }
    if (validate && !validate(bytes, error)) return false;
    const auto target = std::filesystem::u8path(path);
    const auto directoryPath = target.parent_path().empty() ? std::filesystem::path(".") : target.parent_path();
    std::error_code ec;
    std::filesystem::create_directories(directoryPath, ec);
    if (ec) { error = "Cannot create save directory"; return false; }
    const bool exists = std::filesystem::exists(target, ec);
    if (ec || (exists && !std::filesystem::is_regular_file(target, ec)) || ec) {
        error = "Save target is not an accessible regular file"; return false;
    }
    // Do not rotate a damaged primary over the last valid backup. Validation
    // happens before replacement; startup recovery itself never writes a file.
    bool retainPrimary = exists;
    if (exists && validate) {
        std::string oldBytes, ignored;
        retainPrimary = ReadBoundedFile(path, oldBytes, ignored) && validate(oldBytes, ignored);
    }
#if defined(_WIN32)
    const auto pid = GetCurrentProcessId();
#else
    const auto pid = getpid();
#endif
    auto stage = target;
    stage += ".pending-" + std::to_string(pid) + "-" + std::to_string(++Sequence);
    auto backup = target; backup += ".bak";
    auto temporaryBackup = stage; temporaryBackup += ".backup";
    StagingFiles cleanup{stage, temporaryBackup};
#if defined(_WIN32)
    HANDLE file = CreateFileW(stage.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                             FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, nullptr);
    if (file == INVALID_HANDLE_VALUE) { error = "Cannot create exclusive save staging file"; return false; }
    DWORD written = 0;
    const bool okay = WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) &&
                      written == bytes.size() && FlushFileBuffers(file);
    CloseHandle(file);
#else
    const int file = open(stage.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (file < 0) { error = "Cannot create exclusive save staging file"; return false; }
    std::size_t done = 0;
    while (done < bytes.size()) {
        const auto n = write(file, bytes.data() + done, bytes.size() - done);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        done += static_cast<std::size_t>(n);
    }
    const bool okay = done == bytes.size() && fsync(file) == 0;
    close(file);
#endif
    if (!okay) { error = "Failed writing/flushing save; previous slot retained"; return false; }
    std::string verify;
    if (!ReadBoundedFile(Utf8(stage), verify, error) || verify != bytes) {
        error = "Staging readback differs; previous slot retained"; return false;
    }
    if (validate && !validate(verify, error)) return false;
#if defined(_WIN32)
    const bool replaced = exists
        ? ReplaceFileW(target.c_str(), stage.c_str(), retainPrimary ? backup.c_str() : nullptr,
                       REPLACEFILE_IGNORE_MERGE_ERRORS, nullptr, nullptr) != 0
        : MoveFileExW(stage.c_str(), target.c_str(), MOVEFILE_WRITE_THROUGH) != 0;
    if (!replaced) { error = "Atomic save replacement failed; previous slot retained"; return false; }
#else
    if (retainPrimary) {
        std::filesystem::copy_file(target, temporaryBackup, std::filesystem::copy_options::none, ec);
        if (ec) { error = "Cannot retain previous save"; return false; }
        const int oldBackup = open(temporaryBackup.c_str(), O_RDONLY);
        if (oldBackup < 0) { error = "Cannot open save backup for durability check"; return false; }
        const int flushed = fsync(oldBackup); close(oldBackup);
        if (flushed != 0) { error = "Cannot flush save backup"; return false; }
        std::filesystem::rename(temporaryBackup, backup, ec);
        if (ec) { error = "Cannot commit save backup"; return false; }
    }
    std::filesystem::rename(stage, target, ec);
    if (ec) { error = "Atomic save replacement failed"; return false; }
    const int directory = open(directoryPath.c_str(), O_RDONLY | O_DIRECTORY);
    if (directory < 0) { error = "Save replaced but directory durability could not be confirmed"; return false; }
    const int synced = fsync(directory); close(directory);
    if (synced != 0) { error = "Save replaced but directory durability could not be confirmed"; return false; }
#endif
    return true;
}
}
