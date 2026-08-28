#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <vek/VekScriptEngine.h>
class VekScriptEngine;
namespace vek {
struct GravitySettings{float acceleration=15.5f,terminalFallSpeed=32.0f,groundedVelocity=0.0f;};
struct GravityState{float verticalVelocity=0,airborneTime=0,lastLandingSpeed=0;bool grounded=true;};
class GravitySystem{public:static bool BeginJump(GravityState&,float);static bool Step(GravityState&,const GravitySettings&,float,float,float&);};
struct HealthSettings{float maxHealth=100,hurtThreshold=0.35f;};struct HealthState{float health=100;bool alive=true;};
class HealthSystem{public:static void Reset(HealthState&,const HealthSettings&);static float ApplyDamage(HealthState&,const HealthSettings&,float);static float Heal(HealthState&,const HealthSettings&,float);static void SetHealth(HealthState&,const HealthSettings&,float);static float Normalized(const HealthState&,const HealthSettings&);static bool IsHurt(const HealthState&,const HealthSettings&);};
enum class RagdollPhase{Animated,Ragdoll,Recovering};
struct RagdollSettings{float minimumImpactSpeed=13,fatalImpactSpeed=25,minimumDuration=0.8f,maximumDuration=3,linearDamping=3.2f,angularDamping=4.4f,recoveryDuration=0.65f;};
struct RagdollState{RagdollPhase phase=RagdollPhase::Animated;float time=0,targetDuration=0,blend=0,pitch=0,roll=0,yawOffset=0,pitchVelocity=0,rollVelocity=0,yawVelocity=0,bodyDrop=0,armSpread=0,legSpread=0;};
class RagdollSystem{public:static bool ShouldTrigger(float,float,float,const RagdollSettings&);static void Trigger(RagdollState&,float,float,const RagdollSettings&);static void Update(RagdollState&,float,const RagdollSettings&);static void BeginRecovery(RagdollState&,const RagdollSettings&);static bool Active(const RagdollState&);};

enum class GuiCommandType{
 BeginWindow,EndWindow,BeginPanel,EndPanel,BeginDockPanel,EndDockPanel,BeginScrollPanel,EndScrollPanel,BeginGrid,EndGrid,BeginHorizontal,EndHorizontal,BeginVertical,EndVertical,BeginTabs,EndTabs,BeginTree,EndTree,BeginList,EndList,BeginContextMenu,EndContextMenu,BeginPropertyGrid,EndPropertyGrid,
 Label,Button,ImageButton,Checkbox,Slider,ProgressBar,TextInput,SearchBox,ComboBox,Dropdown,Tooltip,Separator,Spacer
};
struct GuiRect{float x=0,y=0,width=0,height=0;};
struct GuiColor{float r=0,g=0,b=0,a=255;};
struct GuiStyle{float scale=1,opacity=1,cornerRadius=8,padding=10,spacing=6;GuiColor background{18,27,33,245},foreground{235,242,244,255},accent{57,174,188,255},border{74,102,110,255};};
struct GuiCommand{GuiCommandType type=GuiCommandType::Label;std::string id,text,styleId,aux;GuiRect rect;float value=0,minValue=0,maxValue=1;bool checked=false,enabled=true;int columns=1;std::vector<std::string>items;};
class GuiSystem{
public:
 void BeginFrame();void EndFrame();void SetStyle(const GuiStyle&);const GuiStyle&GetStyle()const;void DefineStyle(const std::string&,const GuiStyle&);const GuiStyle*FindStyle(const std::string&)const;
 void BeginWindow(const std::string&,const std::string&,GuiRect={},const std::string&style={});void EndWindow();void BeginPanel(const std::string&,GuiRect={},const std::string&style={});void EndPanel();
 void BeginDockPanel(const std::string&,GuiRect={},const std::string&style={});void EndDockPanel();void BeginScrollPanel(const std::string&,GuiRect={},const std::string&style={});void EndScrollPanel();void BeginGrid(const std::string&,int columns,GuiRect={},const std::string&style={});void EndGrid();void BeginHorizontal(const std::string&,const std::string&style={});void EndHorizontal();void BeginVertical(const std::string&,const std::string&style={});void EndVertical();void BeginTabs(const std::string&,const std::vector<std::string>&,const std::string&style={});void EndTabs();void BeginTree(const std::string&,const std::string&,const std::string&style={});void EndTree();void BeginList(const std::string&,const std::string&style={});void EndList();void BeginContextMenu(const std::string&,GuiRect={},const std::string&style={});void EndContextMenu();void BeginPropertyGrid(const std::string&,const std::string&style={});void EndPropertyGrid();
 void Label(const std::string&,GuiRect={},const std::string&style={});bool Button(const std::string&,const std::string&,GuiRect={},const std::string&style={});bool ImageButton(const std::string&,const std::string&,const std::string&,GuiRect={},const std::string&style={});bool Checkbox(const std::string&,const std::string&,bool,GuiRect={},const std::string&style={});float Slider(const std::string&,const std::string&,float,float,float,GuiRect={},const std::string&style={});void ProgressBar(const std::string&,float,float,float,GuiRect={},const std::string&style={});std::string TextInput(const std::string&,const std::string&,GuiRect={},const std::string&style={});std::string SearchBox(const std::string&,const std::string&,GuiRect={},const std::string&style={});int ComboBox(const std::string&,const std::vector<std::string>&,int,GuiRect={},const std::string&style={});int Dropdown(const std::string&,const std::vector<std::string>&,int,GuiRect={},const std::string&style={});void Tooltip(const std::string&,const std::string&);void Separator();void Spacer(float);
 void SetPressed(const std::string&,bool);void SetValue(const std::string&,float);void SetText(const std::string&,std::string);void SetIndex(const std::string&,int);const std::vector<GuiCommand>&Commands()const;void RegisterNatives(VekScriptEngine&);
private:GuiStyle style;std::unordered_map<std::string,GuiStyle>styles;std::vector<GuiCommand>commands;std::unordered_map<std::string,bool>pressed;std::unordered_map<std::string,float>values;std::unordered_map<std::string,std::string>texts;std::unordered_map<std::string,int>indices;
};
void VekRegisterGameplayLibrary(VekScriptEngine&engine);
} // namespace vek
