#include <vek/VekGameSystems.h>
#include <vek/VekScriptEngine.h>

#include <algorithm>
#include <cctype>
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



bool AnimationLibrary::RegisterAnimation(const AnimationDefinition& d) {
    if (d.id.empty() || !std::isfinite(d.duration) || d.duration <= 0.0f || !std::isfinite(d.speed) || d.speed <= 0.0f)
        return false;
    AnimationDefinition clean=d;
    clean.duration=std::clamp(clean.duration,0.01f,3600.0f);
    clean.speed=std::clamp(clean.speed,0.01f,100.0f);
    clean.blendIn=std::clamp(clean.blendIn,0.0f,10.0f);
    clean.blendOut=std::clamp(clean.blendOut,0.0f,10.0f);
    for(auto& marker:clean.markers) marker.time=std::clamp(marker.time,0.0f,clean.duration);
    clips[clean.id]=std::move(clean);
    return true;
}
bool AnimationLibrary::RegisterAnimationValue(const VekValue& v,std::string* error) {
    if(!v.IsMap()){if(error)*error="animation_register expects a map";return false;}
    AnimationDefinition d;
    d.id=v.Get("id").AsString();
    d.duration=(float)v.Get("duration").AsNumber(1.0);
    d.speed=(float)v.Get("speed").AsNumber(1.0);
    d.blendIn=(float)v.Get("blend_in").AsNumber(0.12);
    d.blendOut=(float)v.Get("blend_out").AsNumber(0.12);
    d.loop=v.Get("loop").AsBool(false);
    if(auto tags=v.Get("tags").AsArray()) for(const auto& x:*tags) if(x.IsString()) d.tags.push_back(x.AsString());
    if(auto markers=v.Get("markers").AsArray()) for(const auto& x:*markers) if(x.IsMap()){
        AnimationMarker m; m.name=x.Get("name").AsString(); m.time=(float)x.Get("time").AsNumber(); m.data=x.Get("data");
        if(!m.name.empty()) d.markers.push_back(std::move(m));
    }
    if(!RegisterAnimation(d)){if(error)*error="invalid animation definition";return false;}
    if(error)error->clear();return true;
}
const AnimationDefinition* AnimationLibrary::Find(const std::string& id) const {auto it=clips.find(id);return it==clips.end()?nullptr:&it->second;}
void AnimationLibrary::Clear(){clips.clear();}
std::size_t AnimationLibrary::Size() const{return clips.size();}
void AnimationLibrary::RegisterNatives(VekScriptEngine& e){
    e.RegisterNative("animation_register",[this](const std::vector<VekValue>& a){std::string er;return VekValue(!a.empty()&&RegisterAnimationValue(a[0],&er));});
    e.RegisterNative("animation_exists",[this](const std::vector<VekValue>& a){return VekValue(!a.empty()&&Find(a[0].AsString())!=nullptr);});
    e.RegisterNative("animation_duration",[this](const std::vector<VekValue>& a){auto*d=a.empty()?nullptr:Find(a[0].AsString());return VekValue(d?d->duration:0.0f);});
}
bool AnimationSystem::Play(AnimationPlaybackState& s,const AnimationDefinition& clip,bool restart){
    if(!restart&&s.playing&&s.clipId==clip.id)return false;
    s.clipId=clip.id;s.time=0;s.normalizedTime=0;s.loopCount=0;s.playing=true;s.finished=false;return true;
}
void AnimationSystem::Stop(AnimationPlaybackState& s){s.playing=false;s.finished=true;}
void AnimationSystem::Update(AnimationPlaybackState& s,const AnimationDefinition& clip,float dt){
    if(!s.playing||s.clipId!=clip.id)return;
    float duration=std::max(0.01f,clip.duration),step=std::clamp(dt,0.0f,0.1f)*std::max(0.01f,clip.speed);
    s.time+=step;
    if(clip.loop){
        while(s.time>=duration){s.time-=duration;++s.loopCount;}
    }else if(s.time>=duration){s.time=duration;s.playing=false;s.finished=true;}
    s.normalizedTime=std::clamp(s.time/duration,0.0f,1.0f);
}
bool AnimationSystem::PassedMarker(const AnimationPlaybackState& before,const AnimationPlaybackState& after,const AnimationDefinition& clip,const std::string& marker){
    for(const auto&m:clip.markers)if(m.name==marker){
        if(after.loopCount>before.loopCount)return before.time<=m.time||after.time>=m.time;
        return before.time<m.time&&after.time>=m.time;
    }
    return false;
}

bool ProximityPromptRegistry::RegisterPrompt(const ProximityPromptDefinition& d){
    if(d.id.empty()||d.actionText.empty()||!std::isfinite(d.maxDistance)||d.maxDistance<=0.0f)return false;
    ProximityPromptDefinition clean=d;clean.maxDistance=std::clamp(clean.maxDistance,0.1f,100.0f);clean.holdDuration=std::clamp(clean.holdDuration,0.0f,30.0f);prompts[clean.id]=std::move(clean);return true;
}
bool ProximityPromptRegistry::RegisterPromptValue(const VekValue&v,std::string*error){
    if(!v.IsMap()){if(error)*error="prompt_register expects a map";return false;}
    ProximityPromptDefinition d;d.id=v.Get("id").AsString();d.actionText=v.Get("action_text").AsString();d.objectText=v.Get("object_text").AsString();d.inputKey=v.Get("input_key").AsString();if(d.inputKey.empty())d.inputKey="E";d.maxDistance=(float)v.Get("max_distance").AsNumber(3.5);d.holdDuration=(float)v.Get("hold_duration").AsNumber(0.0);auto enabledValue=v.Get("enabled");d.enabled=enabledValue.IsNil()?true:enabledValue.AsBool();auto losValue=v.Get("requires_line_of_sight");d.requiresLineOfSight=losValue.IsNil()?false:losValue.AsBool();d.priority=(int)v.Get("priority").AsNumber(0);
    if(!RegisterPrompt(d)){if(error)*error="invalid proximity prompt definition";return false;}if(error)error->clear();return true;
}
const ProximityPromptDefinition* ProximityPromptRegistry::Find(const std::string&id)const{auto it=prompts.find(id);return it==prompts.end()?nullptr:&it->second;}
void ProximityPromptRegistry::Clear(){prompts.clear();}
std::size_t ProximityPromptRegistry::Size()const{return prompts.size();}
void ProximityPromptRegistry::RegisterNatives(VekScriptEngine&e){
    e.RegisterNative("prompt_register",[this](const std::vector<VekValue>&a){std::string er;return VekValue(!a.empty()&&RegisterPromptValue(a[0],&er));});
    e.RegisterNative("prompt_exists",[this](const std::vector<VekValue>&a){return VekValue(!a.empty()&&Find(a[0].AsString())!=nullptr);});
    e.RegisterNative("prompt_max_distance",[this](const std::vector<VekValue>&a){auto*d=a.empty()?nullptr:Find(a[0].AsString());return VekValue(d?d->maxDistance:0.0f);});
}
bool ProximityPromptSystem::Update(ProximityPromptState&s,const ProximityPromptDefinition&d,float distance,bool inputDown,bool inputPressed,float dt){
    s.distance=std::max(0.0f,distance);s.activated=false;s.visible=d.enabled&&s.distance<=d.maxDistance;
    if(!s.visible){s.holdProgress=0;return false;}
    if(d.holdDuration<=0.0f){if(inputPressed){s.activated=true;return true;}return false;}
    if(inputDown)s.holdProgress=std::min(d.holdDuration,s.holdProgress+std::clamp(dt,0.0f,0.1f));else s.holdProgress=0.0f;
    if(s.holdProgress>=d.holdDuration){s.activated=true;s.holdProgress=0;return true;}return false;
}

bool GarageDoorRegistry::RegisterGarage(const GarageDoorDefinition&d){
    if(d.id.empty()||!std::isfinite(d.width)||!std::isfinite(d.height)||d.width<1.0f||d.height<1.0f)return false;
    GarageDoorDefinition c=d;c.width=std::clamp(c.width,1.0f,80.0f);c.height=std::clamp(c.height,1.0f,30.0f);c.panelCount=std::clamp(c.panelCount,2,32);c.openDuration=std::clamp(c.openDuration,0.15f,30.0f);c.closeDuration=std::clamp(c.closeDuration,0.15f,30.0f);c.autoCloseDelay=std::clamp(c.autoCloseDelay,0.0f,300.0f);garages[c.id]=std::move(c);return true;
}
bool GarageDoorRegistry::RegisterGarageValue(const VekValue&v,std::string*error){
    if(!v.IsMap()){if(error)*error="garage_register expects a map";return false;}
    GarageDoorDefinition d;d.id=v.Get("id").AsString();d.displayName=v.Get("display_name").AsString();if(d.displayName.empty())d.displayName="Garage Door";d.width=(float)v.Get("width").AsNumber(24.0);d.height=(float)v.Get("height").AsNumber(8.0);d.panelCount=(int)v.Get("panel_count").AsNumber(8);d.openDuration=(float)v.Get("open_duration").AsNumber(2.6);d.closeDuration=(float)v.Get("close_duration").AsNumber(2.3);d.autoCloseDelay=(float)v.Get("auto_close_delay").AsNumber(8.0);auto sl=v.Get("starts_locked");d.startsLocked=sl.IsNil()?true:sl.AsBool();auto ac=v.Get("auto_close");d.autoClose=ac.IsNil()?true:ac.AsBool();d.openAnimation=v.Get("open_animation").AsString();d.closeAnimation=v.Get("close_animation").AsString();d.accessId=v.Get("access_id").AsString();if(!RegisterGarage(d)){if(error)*error="invalid garage door definition";return false;}if(error)error->clear();return true;
}
const GarageDoorDefinition*GarageDoorRegistry::Find(const std::string&id)const{auto it=garages.find(id);return it==garages.end()?nullptr:&it->second;}
void GarageDoorRegistry::Clear(){garages.clear();}std::size_t GarageDoorRegistry::Size()const{return garages.size();}
void GarageDoorRegistry::RegisterNatives(VekScriptEngine&e){e.RegisterNative("garage_register",[this](const std::vector<VekValue>&a){std::string er;return VekValue(!a.empty()&&RegisterGarageValue(a[0],&er));});e.RegisterNative("garage_exists",[this](const std::vector<VekValue>&a){return VekValue(!a.empty()&&Find(a[0].AsString())!=nullptr);});e.RegisterNative("garage_open_duration",[this](const std::vector<VekValue>&a){auto*d=a.empty()?nullptr:Find(a[0].AsString());return VekValue(d?d->openDuration:0.0f);});}
void GarageDoorSystem::Reset(GarageDoorState&s,const GarageDoorDefinition&d){s={};s.locked=d.startsLocked;s.motion=GarageDoorMotion::Closed;}
bool GarageDoorSystem::RequestOpen(GarageDoorState&s,const GarageDoorDefinition&d,bool accessGranted){if(s.locked&&!accessGranted)return false;if(accessGranted)s.locked=false;if(s.motion==GarageDoorMotion::Open||s.motion==GarageDoorMotion::Opening)return true;s.motion=GarageDoorMotion::Opening;s.autoCloseTimer=0;s.activeAnimation=d.openAnimation;s.animationTime=0;s.animationNormalized=s.openFraction;return true;}
void GarageDoorSystem::RequestClose(GarageDoorState&s){if(s.motion!=GarageDoorMotion::Closed){s.motion=GarageDoorMotion::Closing;s.animationTime=0;}s.autoCloseTimer=0;}
void GarageDoorSystem::Unlock(GarageDoorState&s){s.locked=false;}void GarageDoorSystem::Lock(GarageDoorState&s){if(s.openFraction<=0.001f)s.locked=true;}
void GarageDoorSystem::Update(GarageDoorState&s,const GarageDoorDefinition&d,float dt){dt=std::clamp(dt,0.0f,0.1f);if(s.motion==GarageDoorMotion::Opening){if(s.activeAnimation.empty())s.activeAnimation=d.openAnimation;s.animationTime+=dt;s.openFraction=std::min(1.0f,s.openFraction+dt/std::max(0.15f,d.openDuration));s.animationNormalized=s.openFraction;if(s.openFraction>=1.0f){s.motion=GarageDoorMotion::Open;s.autoCloseTimer=d.autoClose?d.autoCloseDelay:0;s.animationNormalized=1.0f;}}else if(s.motion==GarageDoorMotion::Open&&d.autoClose&&s.autoCloseTimer>0){s.autoCloseTimer-=dt;if(s.autoCloseTimer<=0){s.motion=GarageDoorMotion::Closing;s.activeAnimation=d.closeAnimation;s.animationTime=0;s.animationNormalized=0;}}else if(s.motion==GarageDoorMotion::Closing){if(s.activeAnimation.empty())s.activeAnimation=d.closeAnimation;s.animationTime+=dt;s.openFraction=std::max(0.0f,s.openFraction-dt/std::max(0.15f,d.closeDuration));s.animationNormalized=1.0f-s.openFraction;if(s.openFraction<=0){s.motion=GarageDoorMotion::Closed;s.animationNormalized=1.0f;if(d.startsLocked)s.locked=true;}}}

bool PasslockRegistry::RegisterPasslock(const PasslockDefinition&d){if(d.id.empty()||d.accessCode.empty())return false;PasslockDefinition c=d;c.minDigits=std::clamp(c.minDigits,1,16);c.maxDigits=std::clamp(c.maxDigits,c.minDigits,32);c.maxAttempts=std::clamp(c.maxAttempts,1,20);c.lockoutSeconds=std::clamp(c.lockoutSeconds,0.0f,300.0f);if((int)c.accessCode.size()<c.minDigits||(int)c.accessCode.size()>c.maxDigits)return false;for(char ch:c.accessCode)if(!std::isdigit((unsigned char)ch))return false;passlocks[c.id]=std::move(c);return true;}
bool PasslockRegistry::RegisterPasslockValue(const VekValue&v,std::string*error){if(!v.IsMap()){if(error)*error="passlock_register expects a map";return false;}PasslockDefinition d;d.id=v.Get("id").AsString();d.displayName=v.Get("display_name").AsString();if(d.displayName.empty())d.displayName="Access Control";d.accessCode=v.Get("code").AsString();d.linkedGarageId=v.Get("garage_id").AsString();d.promptId=v.Get("prompt_id").AsString();d.minDigits=(int)v.Get("min_digits").AsNumber(4);d.maxDigits=(int)v.Get("max_digits").AsNumber(8);d.maxAttempts=(int)v.Get("max_attempts").AsNumber(5);d.lockoutSeconds=(float)v.Get("lockout_seconds").AsNumber(10);auto ak=v.Get("allow_keyboard");d.allowKeyboard=ak.IsNil()?true:ak.AsBool();auto am=v.Get("allow_mouse");d.allowMouse=am.IsNil()?true:am.AsBool();auto mi=v.Get("mask_input");d.maskInput=mi.IsNil()?true:mi.AsBool();if(!RegisterPasslock(d)){if(error)*error="invalid passlock definition";return false;}if(error)error->clear();return true;}
const PasslockDefinition*PasslockRegistry::Find(const std::string&id)const{auto it=passlocks.find(id);return it==passlocks.end()?nullptr:&it->second;}void PasslockRegistry::Clear(){passlocks.clear();}std::size_t PasslockRegistry::Size()const{return passlocks.size();}
void PasslockRegistry::RegisterNatives(VekScriptEngine&e){e.RegisterNative("passlock_register",[this](const std::vector<VekValue>&a){std::string er;return VekValue(!a.empty()&&RegisterPasslockValue(a[0],&er));});e.RegisterNative("passlock_exists",[this](const std::vector<VekValue>&a){return VekValue(!a.empty()&&Find(a[0].AsString())!=nullptr);});e.RegisterNative("passlock_max_digits",[this](const std::vector<VekValue>&a){auto*d=a.empty()?nullptr:Find(a[0].AsString());return VekValue(d?d->maxDigits:0);});}
void PasslockSystem::Reset(PasslockState&s){s={};}void PasslockSystem::Update(PasslockState&s,float dt){s.lockoutRemaining=std::max(0.0f,s.lockoutRemaining-std::clamp(dt,0.0f,0.1f));if(s.lockoutRemaining<=0&&s.failedAttempts<0)s.failedAttempts=0;}
PasslockResult PasslockSystem::Submit(PasslockState&s,const PasslockDefinition&d,const std::string&code){if(s.lockoutRemaining>0)return PasslockResult::LockedOut;if((int)code.size()<d.minDigits||(int)code.size()>d.maxDigits)return PasslockResult::InvalidInput;for(char ch:code)if(!std::isdigit((unsigned char)ch))return PasslockResult::InvalidInput;if(code==d.accessCode){s.granted=true;s.failedAttempts=0;s.lockoutRemaining=0;return PasslockResult::Granted;}s.granted=false;++s.failedAttempts;if(s.failedAttempts>=d.maxAttempts){s.failedAttempts=0;s.lockoutRemaining=d.lockoutSeconds;return PasslockResult::LockedOut;}return PasslockResult::Denied;}

void GuiSystem::BeginFrame(){commands.clear();}
void GuiSystem::EndFrame(){}
void GuiSystem::SetStyle(const GuiStyle&s){style=s;}
const GuiStyle&GuiSystem::GetStyle()const{return style;}
void GuiSystem::DefineStyle(const std::string&id,const GuiStyle&s){styles[id]=s;}
const GuiStyle*GuiSystem::FindStyle(const std::string&id)const{auto it=styles.find(id);return it==styles.end()?nullptr:&it->second;}
static GuiCommand MakeContainer(GuiCommandType t,const std::string&id,GuiRect r,const std::string&style){GuiCommand c;c.type=t;c.id=id;c.rect=r;c.styleId=style;return c;}
void GuiSystem::BeginWindow(const std::string&id,const std::string&title,GuiRect r,const std::string&st){auto c=MakeContainer(GuiCommandType::BeginWindow,id,r,st);c.text=title;commands.push_back(std::move(c));}void GuiSystem::EndWindow(){commands.push_back(MakeContainer(GuiCommandType::EndWindow,"",{},""));}
void GuiSystem::BeginModal(const std::string&id,const std::string&title,GuiRect r,const std::string&st){auto c=MakeContainer(GuiCommandType::BeginModal,id,r,st);c.text=title;commands.push_back(std::move(c));}void GuiSystem::EndModal(){commands.push_back(MakeContainer(GuiCommandType::EndModal,"",{},""));}
void GuiSystem::BeginPanel(const std::string&id,GuiRect r,const std::string&st){commands.push_back(MakeContainer(GuiCommandType::BeginPanel,id,r,st));}void GuiSystem::EndPanel(){commands.push_back(MakeContainer(GuiCommandType::EndPanel,"",{},""));}
void GuiSystem::BeginDockPanel(const std::string&id,GuiRect r,const std::string&st){commands.push_back(MakeContainer(GuiCommandType::BeginDockPanel,id,r,st));}void GuiSystem::EndDockPanel(){commands.push_back(MakeContainer(GuiCommandType::EndDockPanel,"",{},""));}
void GuiSystem::BeginScrollPanel(const std::string&id,GuiRect r,const std::string&st){commands.push_back(MakeContainer(GuiCommandType::BeginScrollPanel,id,r,st));}void GuiSystem::EndScrollPanel(){commands.push_back(MakeContainer(GuiCommandType::EndScrollPanel,"",{},""));}
void GuiSystem::BeginGrid(const std::string&id,int cols,GuiRect r,const std::string&st){auto c=MakeContainer(GuiCommandType::BeginGrid,id,r,st);c.columns=std::max(1,cols);commands.push_back(std::move(c));}void GuiSystem::EndGrid(){commands.push_back(MakeContainer(GuiCommandType::EndGrid,"",{},""));}
void GuiSystem::BeginHorizontal(const std::string&id,const std::string&st){commands.push_back(MakeContainer(GuiCommandType::BeginHorizontal,id,{},st));}void GuiSystem::EndHorizontal(){commands.push_back(MakeContainer(GuiCommandType::EndHorizontal,"",{},""));}
void GuiSystem::BeginVertical(const std::string&id,const std::string&st){commands.push_back(MakeContainer(GuiCommandType::BeginVertical,id,{},st));}void GuiSystem::EndVertical(){commands.push_back(MakeContainer(GuiCommandType::EndVertical,"",{},""));}
void GuiSystem::BeginTabs(const std::string&id,const std::vector<std::string>&items,const std::string&st){auto c=MakeContainer(GuiCommandType::BeginTabs,id,{},st);c.items=items;commands.push_back(std::move(c));}void GuiSystem::EndTabs(){commands.push_back(MakeContainer(GuiCommandType::EndTabs,"",{},""));}
void GuiSystem::BeginTree(const std::string&id,const std::string&label,const std::string&st){auto c=MakeContainer(GuiCommandType::BeginTree,id,{},st);c.text=label;commands.push_back(std::move(c));}void GuiSystem::EndTree(){commands.push_back(MakeContainer(GuiCommandType::EndTree,"",{},""));}
void GuiSystem::BeginList(const std::string&id,const std::string&st){commands.push_back(MakeContainer(GuiCommandType::BeginList,id,{},st));}void GuiSystem::EndList(){commands.push_back(MakeContainer(GuiCommandType::EndList,"",{},""));}
void GuiSystem::BeginContextMenu(const std::string&id,GuiRect r,const std::string&st){commands.push_back(MakeContainer(GuiCommandType::BeginContextMenu,id,r,st));}void GuiSystem::EndContextMenu(){commands.push_back(MakeContainer(GuiCommandType::EndContextMenu,"",{},""));}
void GuiSystem::BeginPropertyGrid(const std::string&id,const std::string&st){commands.push_back(MakeContainer(GuiCommandType::BeginPropertyGrid,id,{},st));}void GuiSystem::EndPropertyGrid(){commands.push_back(MakeContainer(GuiCommandType::EndPropertyGrid,"",{},""));}
void GuiSystem::Label(const std::string&t,GuiRect r,const std::string&st){GuiCommand c;c.type=GuiCommandType::Label;c.text=t;c.rect=r;c.styleId=st;commands.push_back(std::move(c));}
bool GuiSystem::Button(const std::string&id,const std::string&t,GuiRect r,const std::string&st){GuiCommand c;c.type=GuiCommandType::Button;c.id=id;c.text=t;c.rect=r;c.styleId=st;commands.push_back(c);auto it=pressed.find(id);bool hit=it!=pressed.end()&&it->second;if(hit)it->second=false;return hit;}
bool GuiSystem::ImageButton(const std::string&id,const std::string&img,const std::string&t,GuiRect r,const std::string&st){GuiCommand c;c.type=GuiCommandType::ImageButton;c.id=id;c.aux=img;c.text=t;c.rect=r;c.styleId=st;commands.push_back(c);auto it=pressed.find(id);bool hit=it!=pressed.end()&&it->second;if(hit)it->second=false;return hit;}
bool GuiSystem::Checkbox(const std::string&id,const std::string&t,bool v,GuiRect r,const std::string&st){GuiCommand c;c.type=GuiCommandType::Checkbox;c.id=id;c.text=t;c.rect=r;c.checked=v;c.styleId=st;commands.push_back(c);auto it=pressed.find(id);if(it!=pressed.end()&&it->second){it->second=false;return !v;}return v;}
float GuiSystem::Slider(const std::string&id,const std::string&t,float v,float lo,float hi,GuiRect r,const std::string&st){auto it=values.find(id);if(it!=values.end())v=it->second;v=std::clamp(v,lo,hi);GuiCommand c;c.type=GuiCommandType::Slider;c.id=id;c.text=t;c.rect=r;c.value=v;c.minValue=lo;c.maxValue=hi;c.styleId=st;commands.push_back(c);return v;}
void GuiSystem::ProgressBar(const std::string&id,float v,float lo,float hi,GuiRect r,const std::string&st){GuiCommand c;c.type=GuiCommandType::ProgressBar;c.id=id;c.rect=r;c.value=std::clamp(v,lo,hi);c.minValue=lo;c.maxValue=hi;c.styleId=st;commands.push_back(c);}
std::string GuiSystem::TextInput(const std::string&id,const std::string&v,GuiRect r,const std::string&st){std::string cur=v;auto it=texts.find(id);if(it!=texts.end())cur=it->second;GuiCommand c;c.type=GuiCommandType::TextInput;c.id=id;c.text=cur;c.rect=r;c.styleId=st;commands.push_back(c);return cur;}
std::string GuiSystem::PasswordInput(const std::string&id,const std::string&v,GuiRect r,const std::string&st){std::string cur=v;auto it=texts.find(id);if(it!=texts.end())cur=it->second;GuiCommand c;c.type=GuiCommandType::PasswordInput;c.id=id;c.text=cur;c.rect=r;c.styleId=st;commands.push_back(c);return cur;}
std::string GuiSystem::SearchBox(const std::string&id,const std::string&v,GuiRect r,const std::string&st){std::string cur=v;auto it=texts.find(id);if(it!=texts.end())cur=it->second;GuiCommand c;c.type=GuiCommandType::SearchBox;c.id=id;c.text=cur;c.rect=r;c.styleId=st;commands.push_back(c);return cur;}
int GuiSystem::ComboBox(const std::string&id,const std::vector<std::string>&items,int idx,GuiRect r,const std::string&st){auto it=indices.find(id);if(it!=indices.end())idx=it->second;if(items.empty())idx=-1;else idx=std::clamp(idx,0,(int)items.size()-1);GuiCommand c;c.type=GuiCommandType::ComboBox;c.id=id;c.items=items;c.value=(float)idx;c.rect=r;c.styleId=st;commands.push_back(c);return idx;}
int GuiSystem::Dropdown(const std::string&id,const std::vector<std::string>&items,int idx,GuiRect r,const std::string&st){auto it=indices.find(id);if(it!=indices.end())idx=it->second;if(items.empty())idx=-1;else idx=std::clamp(idx,0,(int)items.size()-1);GuiCommand c;c.type=GuiCommandType::Dropdown;c.id=id;c.items=items;c.value=(float)idx;c.rect=r;c.styleId=st;commands.push_back(c);return idx;}
void GuiSystem::StatusBadge(const std::string&id,const std::string&text,const std::string&status,GuiRect r,const std::string&st){GuiCommand c;c.type=GuiCommandType::StatusBadge;c.id=id;c.text=text;c.aux=status;c.rect=r;c.styleId=st;commands.push_back(std::move(c));}
void GuiSystem::Keypad(const std::string&id,const std::vector<std::string>&items,GuiRect r,const std::string&st){GuiCommand c;c.type=GuiCommandType::Keypad;c.id=id;c.items=items;c.rect=r;c.styleId=st;commands.push_back(std::move(c));}
void GuiSystem::Tooltip(const std::string&id,const std::string&t){GuiCommand c;c.type=GuiCommandType::Tooltip;c.id=id;c.text=t;commands.push_back(std::move(c));}void GuiSystem::Separator(){commands.push_back(MakeContainer(GuiCommandType::Separator,"",{},""));}void GuiSystem::Spacer(float a){GuiCommand c;c.type=GuiCommandType::Spacer;c.value=a;commands.push_back(c);}void GuiSystem::SetPressed(const std::string&id,bool v){pressed[id]=v;}void GuiSystem::SetValue(const std::string&id,float v){values[id]=v;}void GuiSystem::SetText(const std::string&id,std::string v){texts[id]=std::move(v);}void GuiSystem::SetIndex(const std::string&id,int i){indices[id]=i;}const std::vector<GuiCommand>&GuiSystem::Commands()const{return commands;}
static std::vector<std::string> GuiItems(const VekValue&v){std::vector<std::string>o;if(auto*a=v.AsArray())for(auto&x:*a)o.push_back(x.AsString());return o;}
static GuiStyle StyleFromMap(const VekValue&v,GuiStyle s={}){if(!v.IsMap())return s;s.padding=(float)v.Get("padding").AsNumber(s.padding);s.cornerRadius=(float)v.Get("corner_radius").AsNumber(s.cornerRadius);s.spacing=(float)v.Get("spacing").AsNumber(s.spacing);s.opacity=(float)v.Get("opacity").AsNumber(s.opacity);auto col=[&](const char*k,GuiColor&c){auto x=v.Get(k);if(x.IsMap()){c.r=(float)x.Get("r").AsNumber(c.r);c.g=(float)x.Get("g").AsNumber(c.g);c.b=(float)x.Get("b").AsNumber(c.b);c.a=(float)x.Get("a").AsNumber(c.a);}};col("background",s.background);col("foreground",s.foreground);col("accent",s.accent);col("border",s.border);return s;}
void GuiSystem::RegisterNatives(VekScriptEngine&e){
 e.RegisterNative("gui_define_style",[this](const std::vector<VekValue>&a){if(a.size()>=2)DefineStyle(a[0].AsString(),StyleFromMap(a[1],style));return VekValue();});
 e.RegisterNative("gui_begin_window",[this](const std::vector<VekValue>&a){if(a.size()>=2)BeginWindow(a[0].AsString(),a[1].AsString(),{},a.size()>2?a[2].AsString():"");return VekValue();});e.RegisterNative("gui_end_window",[this](const std::vector<VekValue>&){EndWindow();return VekValue();});
 e.RegisterNative("gui_begin_modal",[this](const std::vector<VekValue>&a){if(a.size()>=2)BeginModal(a[0].AsString(),a[1].AsString(),{},a.size()>2?a[2].AsString():"");return VekValue();});e.RegisterNative("gui_end_modal",[this](const std::vector<VekValue>&){EndModal();return VekValue();});
 e.RegisterNative("gui_begin_panel",[this](const std::vector<VekValue>&a){BeginPanel(a.empty()?"panel":a[0].AsString(),{},a.size()>1?a[1].AsString():"");return VekValue();});e.RegisterNative("gui_end_panel",[this](const std::vector<VekValue>&){EndPanel();return VekValue();});
 e.RegisterNative("gui_begin_dock",[this](const std::vector<VekValue>&a){BeginDockPanel(a.empty()?"dock":a[0].AsString());return VekValue();});e.RegisterNative("gui_end_dock",[this](const std::vector<VekValue>&){EndDockPanel();return VekValue();});
 e.RegisterNative("gui_begin_scroll",[this](const std::vector<VekValue>&a){BeginScrollPanel(a.empty()?"scroll":a[0].AsString());return VekValue();});e.RegisterNative("gui_end_scroll",[this](const std::vector<VekValue>&){EndScrollPanel();return VekValue();});
 e.RegisterNative("gui_begin_grid",[this](const std::vector<VekValue>&a){BeginGrid(a.empty()?"grid":a[0].AsString(),a.size()>1?(int)a[1].AsNumber(1):1);return VekValue();});e.RegisterNative("gui_end_grid",[this](const std::vector<VekValue>&){EndGrid();return VekValue();});
 e.RegisterNative("gui_begin_horizontal",[this](const std::vector<VekValue>&a){BeginHorizontal(a.empty()?"row":a[0].AsString());return VekValue();});e.RegisterNative("gui_end_horizontal",[this](const std::vector<VekValue>&){EndHorizontal();return VekValue();});
 e.RegisterNative("gui_begin_vertical",[this](const std::vector<VekValue>&a){BeginVertical(a.empty()?"column":a[0].AsString());return VekValue();});e.RegisterNative("gui_end_vertical",[this](const std::vector<VekValue>&){EndVertical();return VekValue();});
 e.RegisterNative("gui_begin_tabs",[this](const std::vector<VekValue>&a){BeginTabs(a.empty()?"tabs":a[0].AsString(),a.size()>1?GuiItems(a[1]):std::vector<std::string>{});return VekValue();});e.RegisterNative("gui_end_tabs",[this](const std::vector<VekValue>&){EndTabs();return VekValue();});
 e.RegisterNative("gui_begin_tree",[this](const std::vector<VekValue>&a){BeginTree(a.empty()?"tree":a[0].AsString(),a.size()>1?a[1].AsString():"");return VekValue();});e.RegisterNative("gui_end_tree",[this](const std::vector<VekValue>&){EndTree();return VekValue();});
 e.RegisterNative("gui_begin_list",[this](const std::vector<VekValue>&a){BeginList(a.empty()?"list":a[0].AsString());return VekValue();});e.RegisterNative("gui_end_list",[this](const std::vector<VekValue>&){EndList();return VekValue();});
 e.RegisterNative("gui_begin_property_grid",[this](const std::vector<VekValue>&a){BeginPropertyGrid(a.empty()?"props":a[0].AsString());return VekValue();});e.RegisterNative("gui_end_property_grid",[this](const std::vector<VekValue>&){EndPropertyGrid();return VekValue();});
 e.RegisterNative("gui_label",[this](const std::vector<VekValue>&a){if(!a.empty())Label(a[0].AsString(),{},a.size()>1?a[1].AsString():"");return VekValue();});e.RegisterNative("gui_button",[this](const std::vector<VekValue>&a){return VekValue(a.size()>=2&&Button(a[0].AsString(),a[1].AsString(),{},a.size()>2?a[2].AsString():""));});e.RegisterNative("gui_image_button",[this](const std::vector<VekValue>&a){return VekValue(a.size()>=3&&ImageButton(a[0].AsString(),a[1].AsString(),a[2].AsString()));});e.RegisterNative("gui_checkbox",[this](const std::vector<VekValue>&a){return VekValue(a.size()>=3&&Checkbox(a[0].AsString(),a[1].AsString(),a[2].AsBool()));});e.RegisterNative("gui_slider",[this](const std::vector<VekValue>&a){return VekValue(a.size()>=5?Slider(a[0].AsString(),a[1].AsString(),(float)a[2].AsNumber(),(float)a[3].AsNumber(),(float)a[4].AsNumber()):0.0);});e.RegisterNative("gui_progress",[this](const std::vector<VekValue>&a){if(a.size()>=4)ProgressBar(a[0].AsString(),(float)a[1].AsNumber(),(float)a[2].AsNumber(),(float)a[3].AsNumber());return VekValue();});e.RegisterNative("gui_text_input",[this](const std::vector<VekValue>&a){return VekValue(a.size()>=2?TextInput(a[0].AsString(),a[1].AsString()):"");});e.RegisterNative("gui_password_input",[this](const std::vector<VekValue>&a){return VekValue(a.size()>=2?PasswordInput(a[0].AsString(),a[1].AsString()):"");});e.RegisterNative("gui_search_box",[this](const std::vector<VekValue>&a){return VekValue(a.size()>=2?SearchBox(a[0].AsString(),a[1].AsString()):"");});e.RegisterNative("gui_combo",[this](const std::vector<VekValue>&a){return VekValue(a.size()>=3?ComboBox(a[0].AsString(),GuiItems(a[1]),(int)a[2].AsNumber()):-1);});e.RegisterNative("gui_dropdown",[this](const std::vector<VekValue>&a){return VekValue(a.size()>=3?Dropdown(a[0].AsString(),GuiItems(a[1]),(int)a[2].AsNumber()):-1);});e.RegisterNative("gui_status_badge",[this](const std::vector<VekValue>&a){if(a.size()>=3)StatusBadge(a[0].AsString(),a[1].AsString(),a[2].AsString());return VekValue();});e.RegisterNative("gui_keypad",[this](const std::vector<VekValue>&a){if(a.size()>=2)Keypad(a[0].AsString(),GuiItems(a[1]));return VekValue();});e.RegisterNative("gui_tooltip",[this](const std::vector<VekValue>&a){if(a.size()>=2)Tooltip(a[0].AsString(),a[1].AsString());return VekValue();});e.RegisterNative("gui_separator",[this](const std::vector<VekValue>&){Separator();return VekValue();});e.RegisterNative("gui_spacer",[this](const std::vector<VekValue>&a){Spacer(a.empty()?0.0f:(float)a[0].AsNumber());return VekValue();});
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
