#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#define VEK_VERSION_MAJOR 1
#define VEK_VERSION_MINOR 1
#define VEK_VERSION_PATCH 0
#define VEK_VERSION_STRING "1.1.0"

class VekValue {
public:
    using Storage = std::variant<std::monostate, double, bool, std::string>;

    VekValue() = default;
    VekValue(double v) : value(v) {}
    VekValue(float v) : value(static_cast<double>(v)) {}
    VekValue(int v) : value(static_cast<double>(v)) {}
    VekValue(bool v) : value(v) {}
    VekValue(const char* v) : value(std::string(v)) {}
    VekValue(std::string v) : value(std::move(v)) {}

    bool IsNil() const;
    bool IsNumber() const;
    bool IsBool() const;
    bool IsString() const;

    double AsNumber(double fallback = 0.0) const;
    bool AsBool(bool fallback = false) const;
    std::string AsString() const;
    bool Truthy() const;

private:
    Storage value;
};

struct VekSecurityPolicy {
    std::size_t maxSourceBytes = 256u * 1024u;
    std::size_t maxTokens = 65536;
    std::size_t maxFunctions = 512;
    std::size_t maxParametersPerFunction = 32;
    std::size_t maxStringBytes = 16u * 1024u;
    std::size_t maxArgumentsPerCall = 32;
    std::size_t maxCallDepth = 64;
    std::size_t maxInstructionsPerCall = 500000;
    std::size_t maxNativeCallsPerCall = 8192;
    std::size_t maxLoopIterationsPerCall = 250000;
};

class VekScriptEngine {
public:
    using NativeFunction = std::function<VekValue(const std::vector<VekValue>&)>;

    VekScriptEngine();
    ~VekScriptEngine();
    VekScriptEngine(VekScriptEngine&&) noexcept;
    VekScriptEngine& operator=(VekScriptEngine&&) noexcept;
    VekScriptEngine(const VekScriptEngine&) = delete;
    VekScriptEngine& operator=(const VekScriptEngine&) = delete;

    bool LoadFile(const std::string& path);
    bool LoadSource(const std::string& source, const std::string& sourceName = "<memory>");
    void Clear();

    void SetSecurityPolicy(const VekSecurityPolicy& policy);
    const VekSecurityPolicy& GetSecurityPolicy() const;

    bool RegisterNative(const std::string& name, NativeFunction function);
    void SealNativeRegistry();
    bool NativeRegistrySealed() const;

    bool HasFunction(const std::string& name) const;
    VekValue Call(const std::string& name, const std::vector<VekValue>& args = {});

    bool IsLoaded() const;
    const std::string& LastError() const;
    const std::string& SourceName() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

// Safe, opt-in standard library for standalone VEK programs. Embedded games can
// register only the capabilities they want instead.
void VekRegisterStandardLibrary(VekScriptEngine& engine);
