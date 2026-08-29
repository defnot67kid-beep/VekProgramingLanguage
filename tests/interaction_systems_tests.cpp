#include <vek/VekGameSystems.h>
#include <vek/VekScriptEngine.h>
#include <cassert>
#include <iostream>

int main(){
    vek::AnimationLibrary animations;
    VekValue clip=VekValue::Map();
    clip.Set("id","door.open");
    clip.Set("duration",1.2);
    clip.Set("loop",false);
    VekValue markers=VekValue::Array();
    VekValue marker=VekValue::Map(); marker.Set("name","latch"); marker.Set("time",0.25);
    markers.Push(marker); clip.Set("markers",markers);
    std::string error;
    assert(animations.RegisterAnimationValue(clip,&error));
    auto* found=animations.Find("door.open"); assert(found&&found->duration>1.19f);

    vek::AnimationPlaybackState state,before;
    assert(vek::AnimationSystem::Play(state,*found));
    for(int i=0;i<2;++i) vek::AnimationSystem::Update(state,*found,0.1f);
    before=state; vek::AnimationSystem::Update(state,*found,0.1f);
    assert(vek::AnimationSystem::PassedMarker(before,state,*found,"latch"));

    vek::ProximityPromptRegistry prompts;
    VekValue prompt=VekValue::Map();
    prompt.Set("id","hangar.enter"); prompt.Set("action_text","Enter Workspace");
    prompt.Set("object_text","Engineering Hangar"); prompt.Set("input_key","E"); prompt.Set("max_distance",3.5);
    assert(prompts.RegisterPromptValue(prompt,&error));
    auto* pd=prompts.Find("hangar.enter"); assert(pd);
    vek::ProximityPromptState ps;
    assert(!vek::ProximityPromptSystem::Update(ps,*pd,5.0f,false,false,0.016f));
    assert(vek::ProximityPromptSystem::Update(ps,*pd,2.0f,true,true,0.016f));
    assert(ps.activated);

    vek::GarageDoorRegistry garages;
    VekValue garage=VekValue::Map();
    garage.Set("id","hangar.main"); garage.Set("width",24.0); garage.Set("height",8.0);
    garage.Set("panel_count",8); garage.Set("starts_locked",true); garage.Set("open_duration",2.0); garage.Set("allow_inside_egress",true); garage.Set("inside_open_distance",6.0);
    assert(garages.RegisterGarageValue(garage,&error));
    const auto* gd=garages.Find("hangar.main"); assert(gd);
    vek::GarageDoorState gs; vek::GarageDoorSystem::Reset(gs,*gd);
    assert(!vek::GarageDoorSystem::RequestOpen(gs,*gd,false));
    assert(vek::GarageDoorSystem::RequestOpen(gs,*gd,true));
    for(int i=0;i<25;++i) vek::GarageDoorSystem::Update(gs,*gd,0.1f);
    assert(gs.openFraction>0.99f); vek::GarageDoorSystem::HoldOpen(gs,*gd); assert(gs.autoCloseTimer>0.0f);

    vek::PasslockRegistry locks;
    VekValue lock=VekValue::Map();
    lock.Set("id","hangar.access"); lock.Set("display_name","Engineering Access");
    lock.Set("code","2580"); lock.Set("garage_id","hangar.main"); lock.Set("max_attempts",3); lock.Set("max_use_distance",4.5); lock.Set("outside_only",true); lock.Set("show_world_prompt",false); lock.Set("click_only",true);
    assert(locks.RegisterPasslockValue(lock,&error));
    const auto* ld=locks.Find("hangar.access"); assert(ld); assert(ld->clickOnly&&ld->outsideOnly&&!ld->showWorldPrompt&&ld->requiresLineOfSight);
    vek::PasslockState ls;
    assert(vek::PasslockSystem::Submit(ls,*ld,"1111")==vek::PasslockResult::Denied);
    assert(vek::PasslockSystem::Submit(ls,*ld,"2580")==vek::PasslockResult::Granted);

    vek::GuiSystem gui; gui.BeginFrame(); gui.BeginModal("access","GARAGE ACCESS");
    gui.PasswordInput("pin","****"); gui.StatusBadge("status","READY","neutral");
    gui.Keypad("pad",{"1","2","3","4","5","6","7","8","9","CLEAR","0","ENTER"}); gui.EndModal(); gui.EndFrame();
    bool sawModal=false,sawPassword=false,sawKeypad=false;
    for(const auto&c:gui.Commands()){sawModal|=c.type==vek::GuiCommandType::BeginModal;sawPassword|=c.type==vek::GuiCommandType::PasswordInput;sawKeypad|=c.type==vek::GuiCommandType::Keypad;}
    assert(sawModal&&sawPassword&&sawKeypad);

    vek::GuiTextPolicy textPolicy;
    textPolicy.fontSize=24.0f; textPolicy.minFontSize=10.0f; textPolicy.maxLines=2;
    textPolicy.autoFit=true; textPolicy.wrap=true; textPolicy.ellipsis=true; textPolicy.clip=true;
    auto layout=vek::GuiTextLayoutSystem::Layout(
        "Engineering garage access status text that must stay inside its panel",
        180.0f, 44.0f, textPolicy,
        [](const std::string& t,float fs){ return (float)t.size()*fs*0.52f; });
    assert(layout.fontSize<=24.0f&&layout.fontSize>=10.0f);
    assert(!layout.lines.empty()&&layout.lines.size()<=2);
    vek::GuiStyle fittedStyle; fittedStyle.text=textPolicy; gui.DefineStyle("fit",fittedStyle);
    gui.BeginFrame(); gui.Label("A long label that must fit",{0,0,120,30},"fit"); gui.EndFrame();
    assert(!gui.Commands().empty()&&gui.Commands()[0].textPolicy.autoFit&&gui.Commands()[0].textPolicy.maxLines==2);

    VekScriptEngine vm; VekRegisterStandardLibrary(vm);
    animations.RegisterNatives(vm); prompts.RegisterNatives(vm); garages.RegisterNatives(vm); locks.RegisterNatives(vm); gui.RegisterNatives(vm);
    assert(vm.LoadSource(R"(
      fn setup(){
        animation_register({id:"walk.to.door",duration:2.0,loop:false});
        prompt_register({id:"door.prompt",action_text:"Open",object_text:"Door",input_key:"E",max_distance:4});
        garage_register({id:"garage.script",width:18,height:7,panel_count:7,starts_locked:true});
        passlock_register({id:"lock.script",code:"2468",garage_id:"garage.script"});
        gui_begin_modal("lock","ACCESS"); gui_password_input("pin","****"); gui_status_badge("state","LOCKED","warning"); gui_end_modal();
        return animation_duration("walk.to.door") + prompt_max_distance("door.prompt") + garage_open_duration("garage.script") + passlock_max_digits("lock.script");
      }
    )","interaction_test.vek"));
    auto value=vm.Call("setup");
    assert(value.AsNumber()>5.9);
    std::cout<<"VEK interaction + responsive GUI tests: PASS\n";
    return 0;
}
