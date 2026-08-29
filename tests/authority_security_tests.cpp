#include <vek/VekAuthoritySystems.h>
#include <vek/VekScriptEngine.h>
#include <cassert>
#include <iostream>

int main(){
    using namespace vek;
    auto serverPolicy=SecurityPolicyFactory::ForTier(SecurityTier::HardenedServer);
    auto devPolicy=SecurityPolicyFactory::ForTier(SecurityTier::Development);
    assert(serverPolicy.maxInstructionsPerCall < devPolicy.maxInstructionsPerCall);

    VekScriptEngine vm;
    AuthorityActionRegistry actions;
    ReplicationSchemaRegistry replication;
    VekRegisterStandardLibrary(vm);
    VekRegisterAuthorityLibrary(vm,&actions,&replication);
    vm.SealNativeRegistry();
    const char* src=R"VEK(
        fn setup(){
            authority_action_register({id:"economy.purchase",server_authoritative:true,allow_client_request:true,require_sequence:true,replay_protected:true,max_requests_per_second:2,burst:2,max_payload_bytes:128,required_capability:"shop.buy"});
            replication_schema_register({id:"economy",max_hz:4,reliable:true,fields:[{name:"money",type:"int",server_owned:true},{name:"xp",type:"int",server_owned:true}]});
            return true;
        }
    )VEK";
    assert(vm.LoadSource(src,"authority-test"));
    assert(vm.Call("setup").AsBool());
    assert(actions.Find("economy.purchase"));
    assert(replication.Find("economy"));

    CapabilityManifest caps;assert(caps.Grant("shop.buy"));caps.Seal();assert(!caps.Grant("admin"));
    ServerAuthoritySystem server(HostAuthorityRole::DedicatedServer);
    AuthorityRequest r;r.actionId="economy.purchase";r.actorId="p1";r.sequence=1;r.nonce="n1";r.payload=VekValue(VekMap{{"item",VekValue("wheel")}});
    assert(server.ValidateClientRequest(r,actions,caps,1.0f).allowed);
    assert(!server.ValidateClientRequest(r,actions,caps,1.1f).allowed); // replay/out-of-order
    r.sequence=2;r.nonce="n2";assert(server.ValidateClientRequest(r,actions,caps,1.2f).allowed);
    r.sequence=3;r.nonce="n3";assert(!server.ValidateClientRequest(r,actions,caps,1.3f).allowed); // rate limit

    ServerAuthoritySystem client(HostAuthorityRole::Client);
    assert(!client.ValidateAuthoritativeCommit("economy.purchase",actions).allowed);
    assert(server.ValidateAuthoritativeCommit("economy.purchase",actions).allowed);
    std::cout << "VEK 1.9 authority/security tests: PASS\n";
    return 0;
}
