#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class VekScriptEngine;

namespace vek {

struct GravitySettings {
    float acceleration = 15.5f;
    float terminalFallSpeed = 32.0f;
    float groundedVelocity = 0.0f;
};

struct GravityState {
    float verticalVelocity = 0.0f;
    float airborneTime = 0.0f;
    float lastLandingSpeed = 0.0f;
    bool grounded = true;
};

class GravitySystem {
public:
    static bool BeginJump(GravityState& state, float jumpSpeed);
    static bool Step(GravityState& state, const GravitySettings& settings,
                     float deltaTime, float groundY, float& worldY);
};

struct HealthSettings {
    float maxHealth = 100.0f;
    float hurtThreshold = 0.35f;
};

struct HealthState {
    float health = 100.0f;
    bool alive = true;
};

class HealthSystem {
public:
    static void Reset(HealthState& state, const HealthSettings& settings);
    static float ApplyDamage(HealthState& state, const HealthSettings& settings, float amount);
    static float Heal(HealthState& state, const HealthSettings& settings, float amount);
    static void SetHealth(HealthState& state, const HealthSettings& settings, float value);
    static float Normalized(const HealthState& state, const HealthSettings& settings);
    static bool IsHurt(const HealthState& state, const HealthSettings& settings);
};

enum class RagdollPhase {
    Animated,
    Ragdoll,
    Recovering
};

struct RagdollSettings {
    float minimumImpactSpeed = 13.0f;
    float fatalImpactSpeed = 25.0f;
    float minimumDuration = 0.8f;
    float maximumDuration = 3.0f;
    float linearDamping = 3.2f;
    float angularDamping = 4.4f;
    float recoveryDuration = 0.65f;
};

struct RagdollState {
    RagdollPhase phase = RagdollPhase::Animated;
    float time = 0.0f;
    float targetDuration = 0.0f;
    float blend = 0.0f;
    float pitch = 0.0f;
    float roll = 0.0f;
    float yawOffset = 0.0f;
    float pitchVelocity = 0.0f;
    float rollVelocity = 0.0f;
    float yawVelocity = 0.0f;
    float bodyDrop = 0.0f;
    float armSpread = 0.0f;
    float legSpread = 0.0f;
};

class RagdollSystem {
public:
    static bool ShouldTrigger(float health, float impactSpeed, float damage,
                              const RagdollSettings& settings);
    static void Trigger(RagdollState& state, float impactSpeed, float directionSign,
                        const RagdollSettings& settings);
    static void Update(RagdollState& state, float deltaTime, const RagdollSettings& settings);
    static void BeginRecovery(RagdollState& state, const RagdollSettings& settings);
    static bool Active(const RagdollState& state);
};

// Renderer-agnostic GUI command system. VEK can author sophisticated interfaces,
// while each host (raylib, SDL, web, editor, etc.) decides how to render them.
// The vehicle game intentionally does NOT consume this API yet.
enum class GuiCommandType {
    BeginWindow,
    EndWindow,
    BeginPanel,
    EndPanel,
    Label,
    Button,
    Checkbox,
    Slider,
    ProgressBar,
    TextInput,
    Separator,
    Spacer
};

struct GuiRect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

struct GuiCommand {
    GuiCommandType type = GuiCommandType::Label;
    std::string id;
    std::string text;
    GuiRect rect;
    float value = 0.0f;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    bool checked = false;
    bool enabled = true;
};

struct GuiStyle {
    float scale = 1.0f;
    float opacity = 1.0f;
    float cornerRadius = 8.0f;
    float padding = 10.0f;
};

class GuiSystem {
public:
    void BeginFrame();
    void EndFrame();
    void SetStyle(const GuiStyle& style);
    const GuiStyle& GetStyle() const;

    void BeginWindow(const std::string& id, const std::string& title, GuiRect rect);
    void EndWindow();
    void BeginPanel(const std::string& id, GuiRect rect);
    void EndPanel();
    void Label(const std::string& text, GuiRect rect = {});
    bool Button(const std::string& id, const std::string& text, GuiRect rect = {});
    bool Checkbox(const std::string& id, const std::string& text, bool value, GuiRect rect = {});
    float Slider(const std::string& id, const std::string& text, float value,
                 float minValue, float maxValue, GuiRect rect = {});
    void ProgressBar(const std::string& id, float value, float minValue,
                     float maxValue, GuiRect rect = {});
    std::string TextInput(const std::string& id, const std::string& value, GuiRect rect = {});
    void Separator();
    void Spacer(float amount);

    void SetPressed(const std::string& id, bool pressed);
    void SetValue(const std::string& id, float value);
    void SetText(const std::string& id, std::string value);

    const std::vector<GuiCommand>& Commands() const;
    void RegisterNatives(VekScriptEngine& engine);

private:
    GuiStyle style;
    std::vector<GuiCommand> commands;
    std::unordered_map<std::string, bool> pressed;
    std::unordered_map<std::string, float> values;
    std::unordered_map<std::string, std::string> texts;
};

// Safe gameplay-oriented helpers exposed to VEK scripts. They are pure helpers
// and do not grant filesystem, process, network, or raw-memory access.
void VekRegisterGameplayLibrary(VekScriptEngine& engine);

} // namespace vek
