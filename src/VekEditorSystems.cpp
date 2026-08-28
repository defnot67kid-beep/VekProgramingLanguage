#include <vek/VekEditorSystems.h>
#include <vek/VekScriptEngine.h>

#include <algorithm>
#include <cmath>

namespace vek {

GameModePolicy GameModeSystem::Policy(GameMode mode) {
    GameModePolicy p;
    p.mode = mode;
    if (mode == GameMode::Sandbox) {
        p.unlimitedMoney = true;
        p.unlimitedParts = true;
        p.allPartsUnlocked = true;
        p.infiniteFuel = true;
        p.freeRepairs = true;
        p.progressionEnabled = false;
        p.jobsEnabled = true;
    }
    return p;
}

const char* GameModeSystem::Name(GameMode mode) {
    return mode == GameMode::Sandbox ? "Sandbox" : "Survival";
}

const std::vector<BuildPartDefinition>& BuildPartCatalog::All() {
    static const std::vector<BuildPartDefinition> parts = {
        {0,"chassis","Basic Chassis",BuildPartCategory::Structural,300,250,0,1,false,false,false,false,true},
        {1,"wheel","Road Wheel",BuildPartCategory::Movement,25,80,0,1,false,true,false,false,false},
        {2,"engine_small","Small Engine",BuildPartCategory::Mechanical,120,400,22000,1,false,false,true,false,false},
        {3,"seat","Driver Seat",BuildPartCategory::Functional,20,100,0,1,false,false,false,true,false},
        {4,"frame","Frame Block",BuildPartCategory::Structural,55,60,0,1,false,false,false,false,true},
        {5,"beam","Steel Beam",BuildPartCategory::Structural,35,35,0,1,false,false,false,false,true},
        {6,"plate","Body Plate",BuildPartCategory::Structural,18,20,0,1,false,false,false,false,true},
        {7,"roll_cage","Roll Cage",BuildPartCategory::Structural,42,85,0,4,false,false,false,false,true},
        {8,"wing","Wing Section",BuildPartCategory::Structural,65,220,0,12,false,false,false,false,true},
        {9,"offroad_wheel","Off-road Wheel",BuildPartCategory::Movement,38,140,0,5,false,true,false,false,false},
        {10,"track","Track Module",BuildPartCategory::Movement,95,340,0,8,false,true,false,false,false},
        {11,"propeller","Propeller",BuildPartCategory::Movement,45,320,0,14,false,false,false,false,false},
        {12,"rotor","Rotor",BuildPartCategory::Movement,80,650,0,18,false,false,false,false,false},
        {13,"thruster","Thruster",BuildPartCategory::Experimental,55,900,18000,30,false,false,true,false,false},
        {14,"engine_diesel","Diesel Engine",BuildPartCategory::Mechanical,210,850,39000,6,false,false,true,false,false},
        {15,"electric_motor","Electric Motor",BuildPartCategory::Mechanical,95,700,30000,8,false,false,true,false,false},
        {16,"fuel_tank","Fuel Tank",BuildPartCategory::Mechanical,70,180,0,2,false,false,false,false,false},
        {17,"battery","Battery Pack",BuildPartCategory::Mechanical,85,260,0,8,false,false,false,false,false},
        {18,"suspension","Suspension Unit",BuildPartCategory::Mechanical,32,120,0,4,false,false,false,false,false},
        {19,"steering","Steering Unit",BuildPartCategory::Mechanical,20,95,0,2,false,false,false,false,false},
        {20,"light","Work Light",BuildPartCategory::Functional,5,30,0,1,false,false,false,false,false},
        {21,"cargo_box","Cargo Box",BuildPartCategory::Functional,55,140,0,3,false,false,false,false,false},
        {22,"tow_hook","Tow Hook",BuildPartCategory::Functional,18,80,0,4,false,false,false,false,false},
        {23,"winch","Winch",BuildPartCategory::Functional,48,230,0,7,false,false,false,false,false},
        {24,"tool_box","Tool Box",BuildPartCategory::Functional,24,75,0,2,false,false,false,false,false},
        {25,"jet_engine","Jet Engine",BuildPartCategory::Experimental,240,3500,90000,30,false,false,true,false,false},
        {26,"hover_pad","Hover Pad",BuildPartCategory::Experimental,60,1200,12000,35,false,false,true,false,false},
        {27,"sandbox_reactor","Prototype Reactor",BuildPartCategory::Experimental,120,0,150000,1,true,false,true,false,false}
    };
    return parts;
}

const BuildPartDefinition* BuildPartCatalog::Find(int id) {
    for (const auto& p : All()) if (p.id == id) return &p;
    return nullptr;
}
const BuildPartDefinition* BuildPartCatalog::Find(const std::string& key) {
    for (const auto& p : All()) if (p.key == key) return &p;
    return nullptr;
}
std::vector<const BuildPartDefinition*> BuildPartCatalog::Category(BuildPartCategory category) {
    std::vector<const BuildPartDefinition*> out;
    for (const auto& p : All()) if (p.category == category) out.push_back(&p);
    return out;
}

VehicleEditorSystem::VehicleEditorSystem(VehicleEditorSettings s) : settings(s) {}
const VehicleEditorSettings& VehicleEditorSystem::Settings() const { return settings; }
void VehicleEditorSystem::SetSettings(const VehicleEditorSettings& s) { settings = s; }
float VehicleEditorSystem::Snap(float value, bool fineGrid) const {
    float g = fineGrid ? settings.fineGridSize : settings.gridSize;
    if (g <= 0.0001f) return value;
    return std::round(value / g) * g;
}
bool VehicleEditorSystem::PositionInsideBuildArea(float x, float z) const {
    return std::fabs(x) <= settings.buildRadius && std::fabs(z) <= settings.buildRadius;
}
int VehicleEditorSystem::MaxParts(GameMode mode) const {
    return mode == GameMode::Sandbox ? settings.sandboxMaxParts : settings.survivalMaxParts;
}
bool VehicleEditorSystem::PartUnlocked(const BuildPartDefinition& part, GameMode mode, int progressionLevel) const {
    if (mode == GameMode::Sandbox) return true;
    if (part.sandboxOnly) return false;
    return progressionLevel >= part.unlockLevel;
}
float VehicleEditorSystem::EffectivePartCost(const BuildPartDefinition& part, GameMode mode) const {
    return mode == GameMode::Sandbox ? 0.0f : std::max(0.0f, part.cost);
}
BuildValidationResult VehicleEditorSystem::Validate(const VehicleBuildCounts& c, GameMode) const {
    BuildValidationResult r;
    if (c.structuralParts < 1) { r.primaryMessage = "Add at least one structural/chassis part."; return r; }
    if (c.seats < 1) { r.primaryMessage = "Add a driver seat."; return r; }
    if (c.engines < 1) { r.primaryMessage = "Add an engine or powered propulsion part."; return r; }
    if (c.wheels < 3 && c.movementParts < 1) { r.primaryMessage = "Add wheels or another movement system."; return r; }
    if (c.mass > 0.0f && c.power / c.mass < 20.0f) r.warnings.push_back("Low power-to-weight ratio.");
    if (c.wheels == 3) r.warnings.push_back("Three-wheel builds may be less stable.");
    r.valid = true;
    r.primaryMessage = "Build ready.";
    return r;
}

void VekRegisterVehicleEditorLibrary(VekScriptEngine& e) {
    e.RegisterNative("mode_is_sandbox",[](const std::vector<VekValue>& a){return VekValue(!a.empty() && a[0].AsString()=="sandbox");});
    e.RegisterNative("mode_part_cost",[](const std::vector<VekValue>& a){if(a.size()<2)return VekValue(0.0);return VekValue(a[0].AsString()=="sandbox"?0.0:std::max(0.0,a[1].AsNumber()));});
    e.RegisterNative("editor_snap",[](const std::vector<VekValue>& a){if(a.size()<2)return VekValue(0.0);double g=std::fabs(a[1].AsNumber());if(g<0.0001)return VekValue(a[0].AsNumber());return VekValue(std::round(a[0].AsNumber()/g)*g);});
    e.RegisterNative("editor_inside_radius",[](const std::vector<VekValue>& a){if(a.size()<3)return VekValue(false);double r=std::fabs(a[2].AsNumber());return VekValue(std::fabs(a[0].AsNumber())<=r && std::fabs(a[1].AsNumber())<=r);});
    e.RegisterNative("editor_power_to_weight",[](const std::vector<VekValue>& a){if(a.size()<2||a[0].AsNumber()<=0.0)return VekValue(0.0);return VekValue(a[1].AsNumber()/a[0].AsNumber());});
    e.RegisterNative("editor_basic_valid",[](const std::vector<VekValue>& a){if(a.size()<4)return VekValue(false);return VekValue(a[0].AsNumber()>=1&&a[1].AsNumber()>=1&&a[2].AsNumber()>=1&&(a[3].AsNumber()>=3||a.size()>4&&a[4].AsNumber()>=1));});
    e.RegisterNative("editor_part_unlocked",[](const std::vector<VekValue>& a){if(a.size()<4)return VekValue(false);bool sandbox=a[0].AsString()=="sandbox";int level=(int)a[1].AsNumber();int required=(int)a[2].AsNumber();bool sandboxOnly=a[3].AsBool();return VekValue(sandbox || (!sandboxOnly && level>=required));});
    e.RegisterNative("editor_max_parts",[](const std::vector<VekValue>& a){if(a.size()<3)return VekValue(96.0);return VekValue(a[0].AsString()=="sandbox"?a[2].AsNumber():a[1].AsNumber());});
}

} // namespace vek
