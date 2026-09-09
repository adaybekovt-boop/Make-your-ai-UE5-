#pragma once
#include <functional>
#include <string>
namespace mai {
// Validation is supplied by the owner of the save format (JSON/checksum/domain).
// The filesystem layer never treats a readable but corrupt file as a good backup.
using SaveValidator = std::function<bool(const std::string&, std::string&)>;
bool WriteDurableFile(const std::string& path, const std::string& bytes,
                      std::string& error, const SaveValidator& validate = {});
bool ReadBoundedFile(const std::string& path, std::string& bytes, std::string& error);
// Read-only recovery. Both candidates pass the same validation, primary first.
// Missing or corrupt candidates are not deleted, renamed or repaired implicitly.
bool ReadValidatedSave(const std::string& path, const SaveValidator& validate,
                       std::string& bytes, bool& usedBackup, std::string& error);
}
