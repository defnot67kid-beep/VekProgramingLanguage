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


void GuiSystem::BeginFrame(){commands.clear();}
void GuiSystem::EndFrame(){}
void GuiSystem::SetStyle(const GuiStyle&s){style=s;}
const GuiStyle&GuiSystem::GetStyle()const{return style;}
void GuiSystem::DefineStyle(const std::string&id,const GuiStyle&s){styles[id]=s;}
const GuiStyle*GuiSystem::FindStyle(const std::string&id)const{auto it=styles.find(id);return it==styles.end()?nullptr:&it->second;}
static GuiCommand MakeContainer(GuiCommandType t,const std::string&id,GuiRect r,const std::string&style){GuiCommand c;c.type=t;c.id=id;c.rect=r;c.styleId=style;return c;}
void GuiSystem::BeginWindow(const std::string&id,const std::string&title,GuiRect r,const std::string&st){auto c=MakeContainer(GuiCommandType::BeginWindow,id,r,st);c.text=title;commands.push_back(std::move(c));}void GuiSystem::EndWindow(){commands.push_back(MakeContainer(GuiCommandType::EndWindow,"",{},""));}
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
std::string GuiSystem::SearchBox(const std::string&id,const std::string&v,GuiRect r,const std::string&st){std::string cur=v;auto it=texts.find(id);if(it!=texts.end())cur=it->second;GuiCommand c;c.type=GuiCommandType::SearchBox;c.id=id;c.text=cur;c.rect=r;c.styleId=st;commands.push_back(c);return cur;}
int GuiSystem::ComboBox(const std::string&id,const std::vector<std::string>&items,int idx,GuiRect r,const std::string&st){auto it=indices.find(id);if(it!=indices.end())idx=it->second;if(items.empty())idx=-1;else idx=std::clamp(idx,0,(int)items.size()-1);GuiCommand c;c.type=GuiCommandType::ComboBox;c.id=id;c.items=items;c.value=(float)idx;c.rect=r;c.styleId=st;commands.push_back(c);return idx;}
int GuiSystem::Dropdown(const std::string&id,const std::vector<std::string>&items,int idx,GuiRect r,const std::string&st){auto it=indices.find(id);if(it!=indices.end())idx=it->second;if(items.empty())idx=-1;else idx=std::clamp(idx,0,(int)items.size()-1);GuiCommand c;c.type=GuiCommandType::Dropdown;c.id=id;c.items=items;c.value=(float)idx;c.rect=r;c.styleId=st;commands.push_back(c);return idx;}
void GuiSystem::Tooltip(const std::string&id,const std::string&t){GuiCommand c;c.type=GuiCommandType::Tooltip;c.id=id;c.text=t;commands.push_back(std::move(c));}void GuiSystem::Separator(){commands.push_back(MakeContainer(GuiCommandType::Separator,"",{},""));}void GuiSystem::Spacer(float a){GuiCommand c;c.type=GuiCommandType::Spacer;c.value=a;commands.push_back(c);}void GuiSystem::SetPressed(const std::string&id,bool v){pressed[id]=v;}void GuiSystem::SetValue(const std::string&id,float v){values[id]=v;}void GuiSystem::SetText(const std::string&id,std::string v){texts[id]=std::move(v);}void GuiSystem::SetIndex(const std::string&id,int i){indices[id]=i;}const std::vector<GuiCommand>&GuiSystem::Commands()const{return commands;}
static std::vector<std::string> GuiItems(const VekValue&v){std::vector<std::string>o;if(auto*a=v.AsArray())for(auto&x:*a)o.push_back(x.AsString());return o;}
static GuiStyle StyleFromMap(const VekValue&v,GuiStyle s={}){if(!v.IsMap())return s;s.padding=(float)v.Get("padding").AsNumber(s.padding);s.cornerRadius=(float)v.Get("corner_radius").AsNumber(s.cornerRadius);s.spacing=(float)v.Get("spacing").AsNumber(s.spacing);s.opacity=(float)v.Get("opacity").AsNumber(s.opacity);auto col=[&](const char*k,GuiColor&c){auto x=v.Get(k);if(x.IsMap()){c.r=(float)x.Get("r").AsNumber(c.r);c.g=(float)x.Get("g").AsNumber(c.g);c.b=(float)x.Get("b").AsNumber(c.b);c.a=(float)x.Get("a").AsNumber(c.a);}};col("background",s.background);col("foreground",s.foreground);col("accent",s.accent);col("border",s.border);return s;}
void GuiSystem::RegisterNatives(VekScriptEngine&e){
 e.RegisterNative("gui_define_style",[this](const std::vector<VekValue>&a){if(a.size()>=2)DefineStyle(a[0].AsString(),StyleFromMap(a[1],style));return VekValue();});
 e.RegisterNative("gui_begin_window",[this](const std::vector<VekValue>&a){if(a.size()>=2)BeginWindow(a[0].AsString(),a[1].AsString(),{},a.size()>2?a[2].AsString():"");return VekValue();});e.RegisterNative("gui_end_window",[this](const std::vector<VekValue>&){EndWindow();return VekValue();});
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
 e.RegisterNative("gui_label",[this](const std::vector<VekValue>&a){if(!a.empty())Label(a[0].AsString(),{},a.size()>1?a[1].AsString():"");return VekValue();});e.RegisterNative("gui_button",[this](const std::vector<VekValue>&a){return VekValue(a.size()>=2&&Button(a[0].AsString(),a[1].AsString(),{},a.size()>2?a[2].AsString():""));});e.RegisterNative("gui_image_button",[this](const std::vector<VekValue>&a){return VekValue(a.size()>=3&&ImageButton(a[0].AsString(),a[1].AsString(),a[2].AsString()));});e.RegisterNative("gui_checkbox",[this](const std::vector<VekValue>&a){return VekValue(a.size()>=3&&Checkbox(a[0].AsString(),a[1].AsString(),a[2].AsBool()));});e.RegisterNative("gui_slider",[this](const std::vector<VekValue>&a){return VekValue(a.size()>=5?Slider(a[0].AsString(),a[1].AsString(),(float)a[2].AsNumber(),(float)a[3].AsNumber(),(float)a[4].AsNumber()):0.0);});e.RegisterNative("gui_progress",[this](const std::vector<VekValue>&a){if(a.size()>=4)ProgressBar(a[0].AsString(),(float)a[1].AsNumber(),(float)a[2].AsNumber(),(float)a[3].AsNumber());return VekValue();});e.RegisterNative("gui_text_input",[this](const std::vector<VekValue>&a){return VekValue(a.size()>=2?TextInput(a[0].AsString(),a[1].AsString()):"");});e.RegisterNative("gui_search_box",[this](const std::vector<VekValue>&a){return VekValue(a.size()>=2?SearchBox(a[0].AsString(),a[1].AsString()):"");});e.RegisterNative("gui_combo",[this](const std::vector<VekValue>&a){return VekValue(a.size()>=3?ComboBox(a[0].AsString(),GuiItems(a[1]),(int)a[2].AsNumber()):-1);});e.RegisterNative("gui_dropdown",[this](const std::vector<VekValue>&a){return VekValue(a.size()>=3?Dropdown(a[0].AsString(),GuiItems(a[1]),(int)a[2].AsNumber()):-1);});e.RegisterNative("gui_tooltip",[this](const std::vector<VekValue>&a){if(a.size()>=2)Tooltip(a[0].AsString(),a[1].AsString());return VekValue();});e.RegisterNative("gui_separator",[this](const std::vector<VekValue>&){Separator();return VekValue();});e.RegisterNative("gui_spacer",[this](const std::vector<VekValue>&a){Spacer(a.empty()?0.0f:(float)a[0].AsNumber());return VekValue();});
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
