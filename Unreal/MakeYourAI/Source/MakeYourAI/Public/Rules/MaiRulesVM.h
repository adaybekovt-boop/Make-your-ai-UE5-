#pragma once
#include <cstdint>
#include <memory>
#include <string>
namespace mai {
// Embedded rules only. No JS libc, file access, networking, DOM, module loader or browser.
class RulesVM final {
public:
    RulesVM();
    ~RulesVM();
    RulesVM(const RulesVM&) = delete;
    RulesVM& operator=(const RulesVM&) = delete;
    bool Open(const std::string& trustedBundle, std::uint32_t seed, std::string& error);
    bool Call(const std::string& jsonRequest, std::string& jsonResponse, std::string& error);
    void Close();
    bool IsReady() const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
