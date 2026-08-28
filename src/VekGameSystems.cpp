#include <vek/VekGameSystems.h>
#include <vek/VekScriptEngine.h>

#include <algorithm>
#include <cmath>

namespace vek {

namespace {
float ClampDt(float dt) { return std::clamp(dt, 0.0f, 0.1f); }
float Approach(float current, float target, float speed, float dt) {
    if (speed <= 0.0f) return target;
    float t = std::clamp(speed * ClampDt(dt), 0.0f, 1.0f);
    return current + (target - current) * t;
}
}

bool GravitySystem::BeginJump(GravityState& state, float jumpSpeed) {
    if (!state.grounded || jumpSpeed <= 0.0f) return false;
    state.grounded = false;
    state.airborneTime = 0.0f;
    state.lastLandingSpeed = 0.0f;
    state.verticalVelocity = jumpSpeed;
    return true;
}

bool GravitySystem::Step(GravityState& state, const GravitySettings& settings,
                         float deltaTime, float groundY, float& worldY) {
    const float dt = ClampDt(deltaTime);
    if (state.grounded) {
        worldY = groundY;
        state.verticalVelocity = settings.groundedVelocity;
        return false;
    }

    state.airborneTime += dt;
    state.verticalVelocity -= std::max(0.0f, settings.acceleration) * dt;
    state.verticalVelocity = std::max(state.verticalVelocity, -std::fabs(settings.terminalFallSpeed));
    worldY += state.verticalVelocity * dt;

    if (worldY <= groundY) {
        state.lastLandingSpeed = std::fabs(state.verticalVelocity);
        worldY = groundY;
        state.verticalVelocity = settings.groundedVelocity;
        state.grounded = true;
        state.airborneTime = 0.0f;
        return true;
    }
    return false;
}

void HealthSystem::Reset(HealthState& state, const HealthSettings& settings) {
    state.health = std::max(0.0f, settings.maxHealth);
    state.alive = state.health > 0.0f;
}

float HealthSystem::ApplyDamage(HealthState& state, const HealthSettings& settings, float amount) {
    if (amount <= 0.0f || !state.alive) return state.health;
    state.health = std::clamp(state.health - amount, 0.0f, std::max(0.0f, settings.maxHealth));
    state.alive = state.health > 0.0f;
    return state.health;
}

float HealthSystem::Heal(HealthState& state, const HealthSettings& settings, float amount) {
    if (amount <= 0.0f) return state.health;
    state.health = std::clamp(state.health + amount, 0.0f, std::max(0.0f, settings.maxHealth));
    state.alive = state.health > 0.0f;
    return state.health;
}

void HealthSystem::SetHealth(HealthState& state, const HealthSettings& settings, float value) {
    state.health = std::clamp(value, 0.0f, std::max(0.0f, settings.maxHealth));
    state.alive = state.health > 0.0f;
}

float HealthSystem::Normalized(const HealthState& state, const HealthSettings& settings) {
    return settings.maxHealth > 0.0f ? std::clamp(state.health / settings.maxHealth, 0.0f, 1.0f) : 0.0f;
}

bool HealthSystem::IsHurt(const HealthState& state, const HealthSettings& settings) {
    return state.alive && Normalized(state, settings) <= settings.hurtThreshold;
}

bool RagdollSystem::ShouldTrigger(float health, float impactSpeed, float damage,
                                  const RagdollSettings& settings) {
    if (health <= 0.0f) return true;
    if (impactSpeed >= settings.fatalImpactSpeed) return true;
    return damage > 0.0f && impactSpeed >= settings.minimumImpactSpeed;
}

void RagdollSystem::Trigger(RagdollState& state, float impactSpeed, float directionSign,
                            const RagdollSettings& settings) {
    const float sign = directionSign < 0.0f ? -1.0f : 1.0f;
    state.phase = RagdollPhase::Ragdoll;
    state.time = 0.0f;
    float normalized = std::clamp((impactSpeed - settings.minimumImpactSpeed) /
                                  std::max(1.0f, settings.fatalImpactSpeed - settings.minimumImpactSpeed), 0.0f, 1.0f);
    state.targetDuration = settings.minimumDuration + (settings.maximumDuration - settings.minimumDuration) * normalized;
    state.blend = 0.0f;
    state.pitchVelocity = (110.0f + impactSpeed * 5.0f) * sign;
    state.rollVelocity = (55.0f + impactSpeed * 2.4f) * -sign;
    state.yawVelocity = (35.0f + impactSpeed * 1.8f) * sign;
    state.bodyDrop = 0.0f;
    state.armSpread = 0.0f;
    state.legSpread = 0.0f;
}

void RagdollSystem::Update(RagdollState& state, float deltaTime, const RagdollSettings& settings) {
    const float dt = ClampDt(deltaTime);
    if (state.phase == RagdollPhase::Animated) return;
    state.time += dt;

    if (state.phase == RagdollPhase::Ragdoll) {
        state.blend = Approach(state.blend, 1.0f, 8.0f, dt);
        state.pitch += state.pitchVelocity * dt;
        state.roll += state.rollVelocity * dt;
        state.yawOffset += state.yawVelocity * dt;
        state.pitchVelocity = Approach(state.pitchVelocity, 0.0f, settings.angularDamping, dt);
        state.rollVelocity = Approach(state.rollVelocity, 0.0f, settings.angularDamping, dt);
        state.yawVelocity = Approach(state.yawVelocity, 0.0f, settings.angularDamping, dt);
        state.bodyDrop = Approach(state.bodyDrop, 0.72f, settings.linearDamping, dt);
        state.armSpread = Approach(state.armSpread, 1.0f, 6.5f, dt);
        state.legSpread = Approach(state.legSpread, 1.0f, 5.0f, dt);
        if (state.time >= state.targetDuration) BeginRecovery(state, settings);
    } else if (state.phase == RagdollPhase::Recovering) {
        state.blend = Approach(state.blend, 0.0f, 1.0f / std::max(0.05f, settings.recoveryDuration), dt);
        state.pitch = Approach(state.pitch, 0.0f, 7.0f, dt);
        state.roll = Approach(state.roll, 0.0f, 7.0f, dt);
        state.yawOffset = Approach(state.yawOffset, 0.0f, 7.0f, dt);
        state.bodyDrop = Approach(state.bodyDrop, 0.0f, 6.0f, dt);
        state.armSpread = Approach(state.armSpread, 0.0f, 7.0f, dt);
        state.legSpread = Approach(state.legSpread, 0.0f, 7.0f, dt);
        if (state.blend <= 0.01f && std::fabs(state.pitch) < 0.5f && std::fabs(state.roll) < 0.5f) {
            state = RagdollState{};
        }
    }
}

void RagdollSystem::BeginRecovery(RagdollState& state, const RagdollSettings&) {
    state.phase = RagdollPhase::Recovering;
    state.time = 0.0f;
}

bool RagdollSystem::Active(const RagdollState& state) {
    return state.phase != RagdollPhase::Animated;
}

void GuiSystem::BeginFrame() { commands.clear(); }
void GuiSystem::EndFrame() {}
void GuiSystem::SetStyle(const GuiStyle& s) { style = s; }
const GuiStyle& GuiSystem::GetStyle() const { return style; }

void GuiSystem::BeginWindow(const std::string& id, const std::string& title, GuiRect rect) {
    commands.push_back({GuiCommandType::BeginWindow,id,title,rect});
}
void GuiSystem::EndWindow() { commands.push_back({GuiCommandType::EndWindow}); }
void GuiSystem::BeginPanel(const std::string& id, GuiRect rect) { commands.push_back({GuiCommandType::BeginPanel,id,"",rect}); }
void GuiSystem::EndPanel() { commands.push_back({GuiCommandType::EndPanel}); }
void GuiSystem::Label(const std::string& text, GuiRect rect) { commands.push_back({GuiCommandType::Label,"",text,rect}); }

bool GuiSystem::Button(const std::string& id, const std::string& text, GuiRect rect) {
    commands.push_back({GuiCommandType::Button,id,text,rect});
    auto it=pressed.find(id); bool hit=it!=pressed.end() && it->second; if(hit) it->second=false; return hit;
}

bool GuiSystem::Checkbox(const std::string& id, const std::string& text, bool value, GuiRect rect) {
    GuiCommand c; c.type=GuiCommandType::Checkbox; c.id=id; c.text=text; c.rect=rect; c.checked=value; commands.push_back(c);
    auto it=pressed.find(id); if(it!=pressed.end() && it->second){it->second=false; return !value;} return value;
}

float GuiSystem::Slider(const std::string& id, const std::string& text, float value,
                        float minValue, float maxValue, GuiRect rect) {
    auto it=values.find(id); if(it!=values.end()) value=it->second;
    value=std::clamp(value,minValue,maxValue);
    GuiCommand c; c.type=GuiCommandType::Slider;c.id=id;c.text=text;c.rect=rect;c.value=value;c.minValue=minValue;c.maxValue=maxValue;commands.push_back(c);
    return value;
}

void GuiSystem::ProgressBar(const std::string& id, float value, float minValue, float maxValue, GuiRect rect) {
    GuiCommand c; c.type=GuiCommandType::ProgressBar;c.id=id;c.rect=rect;c.value=std::clamp(value,minValue,maxValue);c.minValue=minValue;c.maxValue=maxValue;commands.push_back(c);
}

std::string GuiSystem::TextInput(const std::string& id, const std::string& value, GuiRect rect) {
    std::string current=value; auto it=texts.find(id); if(it!=texts.end()) current=it->second;
    GuiCommand c; c.type=GuiCommandType::TextInput;c.id=id;c.text=current;c.rect=rect;commands.push_back(c); return current;
}
void GuiSystem::Separator(){commands.push_back({GuiCommandType::Separator});}
void GuiSystem::Spacer(float amount){GuiCommand c;c.type=GuiCommandType::Spacer;c.value=amount;commands.push_back(c);}
void GuiSystem::SetPressed(const std::string& id,bool v){pressed[id]=v;}
void GuiSystem::SetValue(const std::string& id,float v){values[id]=v;}
void GuiSystem::SetText(const std::string& id,std::string v){texts[id]=std::move(v);}
const std::vector<GuiCommand>& GuiSystem::Commands() const{return commands;}

void GuiSystem::RegisterNatives(VekScriptEngine& e) {
    e.RegisterNative("gui_begin_window",[this](const std::vector<VekValue>& a){if(a.size()>=6)BeginWindow(a[0].AsString(),a[1].AsString(),{(float)a[2].AsNumber(),(float)a[3].AsNumber(),(float)a[4].AsNumber(),(float)a[5].AsNumber()});return VekValue();});
    e.RegisterNative("gui_end_window",[this](const std::vector<VekValue>&){EndWindow();return VekValue();});
    e.RegisterNative("gui_label",[this](const std::vector<VekValue>& a){if(!a.empty())Label(a[0].AsString());return VekValue();});
    e.RegisterNative("gui_button",[this](const std::vector<VekValue>& a){if(a.size()<2)return VekValue(false);return VekValue(Button(a[0].AsString(),a[1].AsString()));});
    e.RegisterNative("gui_checkbox",[this](const std::vector<VekValue>& a){if(a.size()<3)return VekValue(false);return VekValue(Checkbox(a[0].AsString(),a[1].AsString(),a[2].AsBool()));});
    e.RegisterNative("gui_slider",[this](const std::vector<VekValue>& a){if(a.size()<5)return VekValue(0.0);return VekValue(Slider(a[0].AsString(),a[1].AsString(),(float)a[2].AsNumber(),(float)a[3].AsNumber(),(float)a[4].AsNumber()));});
    e.RegisterNative("gui_progress",[this](const std::vector<VekValue>& a){if(a.size()>=4)ProgressBar(a[0].AsString(),(float)a[1].AsNumber(),(float)a[2].AsNumber(),(float)a[3].AsNumber());return VekValue();});
    e.RegisterNative("gui_text_input",[this](const std::vector<VekValue>& a){if(a.size()<2)return VekValue("");return VekValue(TextInput(a[0].AsString(),a[1].AsString()));});
    e.RegisterNative("gui_separator",[this](const std::vector<VekValue>&){Separator();return VekValue();});
    e.RegisterNative("gui_spacer",[this](const std::vector<VekValue>& a){Spacer(a.empty()?0.0f:(float)a[0].AsNumber());return VekValue();});
}

void VekRegisterGameplayLibrary(VekScriptEngine& e) {
    // Small safe math set commonly needed by gameplay rule scripts.
    e.RegisterNative("abs",[](const std::vector<VekValue>& a){return VekValue(a.empty()?0.0:std::fabs(a[0].AsNumber()));});
    e.RegisterNative("min",[](const std::vector<VekValue>& a){if(a.size()<2)return VekValue(0.0);return VekValue(std::min(a[0].AsNumber(),a[1].AsNumber()));});
    e.RegisterNative("max",[](const std::vector<VekValue>& a){if(a.size()<2)return VekValue(0.0);return VekValue(std::max(a[0].AsNumber(),a[1].AsNumber()));});
    e.RegisterNative("clamp",[](const std::vector<VekValue>& a){if(a.size()<3)return VekValue(0.0);return VekValue(std::clamp(a[0].AsNumber(),a[1].AsNumber(),a[2].AsNumber()));});
    e.RegisterNative("gravity_step",[](const std::vector<VekValue>& a){
        if(a.size()<5)return VekValue(0.0); float v=(float)a[0].AsNumber(); float gravity=std::max(0.0f,(float)a[1].AsNumber()); float terminal=std::fabs((float)a[2].AsNumber()); float dt=ClampDt((float)a[3].AsNumber()); bool grounded=a[4].AsBool(); if(grounded)return VekValue(0.0); return VekValue(std::max(v-gravity*dt,-terminal));
    });
    e.RegisterNative("gravity_jump",[](const std::vector<VekValue>& a){if(a.size()<2)return VekValue(0.0);return VekValue(a[0].AsBool()?std::max(0.0,a[1].AsNumber()):0.0);});
    e.RegisterNative("health_damage",[](const std::vector<VekValue>& a){if(a.size()<3)return VekValue(0.0);return VekValue(std::clamp(a[0].AsNumber()-std::max(0.0,a[2].AsNumber()),0.0,std::max(0.0,a[1].AsNumber())));});
    e.RegisterNative("health_heal",[](const std::vector<VekValue>& a){if(a.size()<3)return VekValue(0.0);return VekValue(std::clamp(a[0].AsNumber()+std::max(0.0,a[2].AsNumber()),0.0,std::max(0.0,a[1].AsNumber())));});
    e.RegisterNative("health_ratio",[](const std::vector<VekValue>& a){if(a.size()<2||a[1].AsNumber()<=0.0)return VekValue(0.0);return VekValue(std::clamp(a[0].AsNumber()/a[1].AsNumber(),0.0,1.0));});
    e.RegisterNative("health_alive",[](const std::vector<VekValue>& a){return VekValue(!a.empty()&&a[0].AsNumber()>0.0);});
    e.RegisterNative("ragdoll_should_trigger",[](const std::vector<VekValue>& a){if(a.size()<5)return VekValue(false);double health=a[0].AsNumber(),impact=a[1].AsNumber(),damage=a[2].AsNumber(),minImpact=a[3].AsNumber(),fatal=a[4].AsNumber();return VekValue(health<=0.0||impact>=fatal||(damage>0.0&&impact>=minImpact));});
    e.RegisterNative("ragdoll_duration",[](const std::vector<VekValue>& a){if(a.size()<5)return VekValue(1.0);double impact=a[0].AsNumber(),minImpact=a[1].AsNumber(),fatal=a[2].AsNumber(),minD=a[3].AsNumber(),maxD=a[4].AsNumber();double n=std::clamp((impact-minImpact)/std::max(1.0,fatal-minImpact),0.0,1.0);return VekValue(minD+(maxD-minD)*n);});
    e.RegisterNative("damp",[](const std::vector<VekValue>& a){if(a.size()<4)return VekValue(0.0);double current=a[0].AsNumber(),target=a[1].AsNumber(),speed=std::max(0.0,a[2].AsNumber()),dt=std::clamp(a[3].AsNumber(),0.0,0.1);double t=std::clamp(speed*dt,0.0,1.0);return VekValue(current+(target-current)*t);});
}

} // namespace vek
