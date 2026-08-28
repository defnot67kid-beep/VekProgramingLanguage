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

    VekScriptEngine vm; VekRegisterStandardLibrary(vm);
    animations.RegisterNatives(vm); prompts.RegisterNatives(vm);
    assert(vm.LoadSource(R"(
      fn setup(){
        animation_register({id:"walk.to.door",duration:2.0,loop:false});
        prompt_register({id:"door.prompt",action_text:"Open",object_text:"Door",input_key:"E",max_distance:4});
        return animation_duration("walk.to.door") + prompt_max_distance("door.prompt");
      }
    )","interaction_test.vek"));
    auto value=vm.Call("setup");
    assert(value.AsNumber()>5.9);
    std::cout<<"VEK animation + proximity prompt tests: PASS\n";
    return 0;
}
