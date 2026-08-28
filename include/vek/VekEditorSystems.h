#pragma once

#include <cstddef>
#include <string>
#include <vector>

class VekScriptEngine;

namespace vek {

enum class GameMode {
    Survival = 0,
    Sandbox = 1
};

struct GameModePolicy {
    GameMode mode = GameMode::Survival;
    bool unlimitedMoney = false;
    bool unlimitedParts = false;
    bool allPartsUnlocked = false;
    bool infiniteFuel = false;
    bool freeRepairs = false;
    bool progressionEnabled = true;
    bool jobsEnabled = true;
};

class GameModeSystem {
public:
    static GameModePolicy Policy(GameMode mode);
    static const char* Name(GameMode mode);
};

enum class BuildPartCategory {
    Structural = 0,
    Movement,
    Mechanical,
    Functional,
    Experimental
};

struct BuildPartDefinition {
    int id = 0;
    std::string key;
    std::string displayName;
    BuildPartCategory category = BuildPartCategory::Structural;
    float mass = 0.0f;
    float cost = 0.0f;
    float power = 0.0f;
    int unlockLevel = 1;
    bool sandboxOnly = false;
    bool isWheel = false;
    bool isEngine = false;
    bool isSeat = false;
    bool isStructural = false;
};

class BuildPartCatalog {
public:
    static const std::vector<BuildPartDefinition>& All();
    static const BuildPartDefinition* Find(int id);
    static const BuildPartDefinition* Find(const std::string& key);
    static std::vector<const BuildPartDefinition*> Category(BuildPartCategory category);
};

struct VehicleBuildCounts {
    int totalParts = 0;
    int structuralParts = 0;
    int wheels = 0;
    int engines = 0;
    int seats = 0;
    int movementParts = 0;
    float mass = 0.0f;
    float power = 0.0f;
    float estimatedCost = 0.0f;
};

struct BuildValidationResult {
    bool valid = false;
    std::string primaryMessage;
    std::vector<std::string> warnings;
};

struct VehicleEditorSettings {
    float gridSize = 0.5f;
    float fineGridSize = 0.25f;
    float rotationStepDegrees = 15.0f;
    int survivalMaxParts = 96;
    int sandboxMaxParts = 512;
    float buildRadius = 12.0f;
    bool allowFreePlacement = true;
    bool allowMirror = true;
    bool allowUndoRedo = true;
};

class VehicleEditorSystem {
public:
    explicit VehicleEditorSystem(VehicleEditorSettings settings = {});

    const VehicleEditorSettings& Settings() const;
    void SetSettings(const VehicleEditorSettings& settings);

    float Snap(float value, bool fineGrid = false) const;
    bool PositionInsideBuildArea(float x, float z) const;
    int MaxParts(GameMode mode) const;
    bool PartUnlocked(const BuildPartDefinition& part, GameMode mode, int progressionLevel) const;
    float EffectivePartCost(const BuildPartDefinition& part, GameMode mode) const;
    BuildValidationResult Validate(const VehicleBuildCounts& counts, GameMode mode) const;

private:
    VehicleEditorSettings settings;
};

// Safe helpers for scripts. These expose only deterministic editor/mode math;
// they do not expose filesystem/process/network/raw-memory access.
void VekRegisterVehicleEditorLibrary(VekScriptEngine& engine);

} // namespace vek
