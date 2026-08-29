#include <vek/VekAuthoritySystems.h>
#include <algorithm>
#include <cmath>

namespace vek {

VekSecurityPolicy SecurityPolicyFactory::ForTier(SecurityTier tier){
    VekSecurityPolicy p;
    if(tier==SecurityTier::Development){
        p.maxSourceBytes=1024u*1024u;p.maxTokens=262144;p.maxFunctions=2048;p.maxParametersPerFunction=96;
        p.maxStringBytes=64u*1024u;p.maxArgumentsPerCall=96;p.maxCallDepth=96;p.maxInstructionsPerCall=1500000;
        p.maxNativeCallsPerCall=32768;p.maxLoopIterationsPerCall=600000;p.maxContainerItems=16384;p.maxModuleCount=256;p.maxImportDepth=24;
    }else if(tier==SecurityTier::HardenedServer){
        p.maxSourceBytes=256u*1024u;p.maxTokens=65536;p.maxFunctions=512;p.maxParametersPerFunction=48;
        p.maxStringBytes=16u*1024u;p.maxArgumentsPerCall=48;p.maxCallDepth=40;p.maxInstructionsPerCall=400000;
        p.maxNativeCallsPerCall=8192;p.maxLoopIterationsPerCall=160000;p.maxContainerItems=4096;p.maxModuleCount=64;p.maxImportDepth=10;
    }else{
        p.maxSourceBytes=384u*1024u;p.maxTokens=98304;p.maxFunctions=768;p.maxParametersPerFunction=48;
        p.maxStringBytes=24u*1024u;p.maxArgumentsPerCall=48;p.maxCallDepth=48;p.maxInstructionsPerCall=500000;
        p.maxNativeCallsPerCall=10000;p.maxLoopIterationsPerCall=200000;p.maxContainerItems=4096;p.maxModuleCount=96;p.maxImportDepth=12;
    }
    return p;
}
void SecurityPolicyFactory::Apply(VekScriptEngine& e,SecurityTier tier){e.SetSecurityPolicy(ForTier(tier));}

bool CapabilityManifest::Grant(const std::string& c){if(sealed||c.empty()||c.size()>128)return false;return capabilities.insert(c).second;}
bool CapabilityManifest::Revoke(const std::string& c){if(sealed)return false;return capabilities.erase(c)>0;}
bool CapabilityManifest::Allows(const std::string& c)const{return c.empty()||capabilities.count(c)>0;}
void CapabilityManifest::Seal(){sealed=true;}

bool AuthorityActionRegistry::Register(const AuthorityActionDefinition& d){
    if(d.id.empty()||d.id.size()>128||!std::isfinite(d.maxRequestsPerSecond)||d.maxRequestsPerSecond<0.1f||d.maxRequestsPerSecond>1000.0f||d.burst<1||d.burst>10000||d.maxPayloadBytes<1||d.maxPayloadBytes>1024u*1024u)return false;
    AuthorityActionDefinition x=d;x.maxRequestsPerSecond=std::clamp(x.maxRequestsPerSecond,0.1f,1000.0f);x.burst=std::clamp(x.burst,1,10000);x.maxPayloadBytes=std::clamp<std::size_t>(x.maxPayloadBytes,1,1024u*1024u);actions[x.id]=std::move(x);return true;
}
bool AuthorityActionRegistry::RegisterValue(const VekValue& v,std::string* error){
    if(!v.IsMap()){if(error)*error="authority_action_register expects a map";return false;}
    AuthorityActionDefinition d;d.id=v.Get("id").AsString();
    auto b=v.Get("server_authoritative");if(!b.IsNil())d.serverAuthoritative=b.AsBool();
    b=v.Get("allow_client_request");if(!b.IsNil())d.allowClientRequest=b.AsBool();
    b=v.Get("require_sequence");if(!b.IsNil())d.requireSequence=b.AsBool();
    b=v.Get("replay_protected");if(!b.IsNil())d.replayProtected=b.AsBool();
    d.maxRequestsPerSecond=(float)v.Get("max_requests_per_second").AsNumber(d.maxRequestsPerSecond);
    d.burst=(int)v.Get("burst").AsNumber(d.burst);
    d.maxPayloadBytes=(std::size_t)std::max(1.0,v.Get("max_payload_bytes").AsNumber((double)d.maxPayloadBytes));
    d.requiredCapability=v.Get("required_capability").AsString();
    if(!Register(d)){if(error)*error="invalid authority action definition";return false;}if(error)error->clear();return true;
}
const AuthorityActionDefinition* AuthorityActionRegistry::Find(const std::string& id)const{auto it=actions.find(id);return it==actions.end()?nullptr:&it->second;}
void AuthorityActionRegistry::Clear(){actions.clear();}
void AuthorityActionRegistry::RegisterNatives(VekScriptEngine& e){e.RegisterNative("authority_action_register",[this](const std::vector<VekValue>&a){std::string er;return VekValue(!a.empty()&&RegisterValue(a[0],&er));});e.RegisterNative("authority_action_exists",[this](const std::vector<VekValue>&a){return VekValue(!a.empty()&&Find(a[0].AsString()));});}

bool ReplicationSchemaRegistry::Register(const ReplicationSchemaDefinition& d){if(d.id.empty()||d.id.size()>128||d.fields.empty()||d.fields.size()>256||!std::isfinite(d.maxHz)||d.maxHz<0.1f||d.maxHz>240.0f)return false;ReplicationSchemaDefinition x=d;x.maxHz=std::clamp(x.maxHz,0.1f,240.0f);for(auto&f:x.fields){if(f.name.empty()||f.name.size()>128)return false;if(f.type.empty())f.type="number";}schemas[x.id]=std::move(x);return true;}
bool ReplicationSchemaRegistry::RegisterValue(const VekValue&v,std::string*error){if(!v.IsMap()){if(error)*error="replication_schema_register expects a map";return false;}ReplicationSchemaDefinition d;d.id=v.Get("id").AsString();d.maxHz=(float)v.Get("max_hz").AsNumber(d.maxHz);d.reliable=v.Get("reliable").AsBool(false);if(auto a=v.Get("fields").AsArray())for(const auto&x:*a)if(x.IsMap()){ReplicationFieldDefinition f;f.name=x.Get("name").AsString();auto t=x.Get("type").AsString();if(!t.empty())f.type=t;auto b=x.Get("server_owned");if(!b.IsNil())f.serverOwned=b.AsBool();b=x.Get("owner_only");if(!b.IsNil())f.ownerOnly=b.AsBool();d.fields.push_back(std::move(f));}if(!Register(d)){if(error)*error="invalid replication schema";return false;}if(error)error->clear();return true;}
const ReplicationSchemaDefinition*ReplicationSchemaRegistry::Find(const std::string&id)const{auto it=schemas.find(id);return it==schemas.end()?nullptr:&it->second;}void ReplicationSchemaRegistry::Clear(){schemas.clear();}
void ReplicationSchemaRegistry::RegisterNatives(VekScriptEngine&e){e.RegisterNative("replication_schema_register",[this](const std::vector<VekValue>&a){std::string er;return VekValue(!a.empty()&&RegisterValue(a[0],&er));});e.RegisterNative("replication_schema_exists",[this](const std::vector<VekValue>&a){return VekValue(!a.empty()&&Find(a[0].AsString()));});}

void SecurityAuditBuffer::Push(SecurityAuditEvent e){if(maxEvents==0)return;events.push_back(std::move(e));while(events.size()>maxEvents)events.pop_front();}

ServerAuthoritySystem::ServerAuthoritySystem(HostAuthorityRole r):hostRole(r){}
void ServerAuthoritySystem::ResetActor(const std::string&a){for(auto it=state.begin();it!=state.end();){if(it->first.rfind(a+"\n",0)==0)it=state.erase(it);else ++it;}}
void ServerAuthoritySystem::Reset(){state.clear();audit.Clear();}
AuthorityDecision ServerAuthoritySystem::Deny(const AuthorityRequest&r,AuthorityDecisionCode c,const std::string&reason,float now){audit.Push({now,r.actorId,r.actionId,c,reason});return{c,false,reason};}
AuthorityDecision ServerAuthoritySystem::ValidateClientRequest(const AuthorityRequest&r,const AuthorityActionRegistry&reg,const CapabilityManifest&caps,float now){
    const auto*d=reg.Find(r.actionId);if(!d)return Deny(r,AuthorityDecisionCode::UnknownAction,"unknown action",now);
    if(hostRole!=HostAuthorityRole::ListenServer&&hostRole!=HostAuthorityRole::DedicatedServer)return Deny(r,AuthorityDecisionCode::WrongHostRole,"client requests must be validated by an authoritative server host",now);
    if(!d->allowClientRequest)return Deny(r,AuthorityDecisionCode::ClientRequestDisabled,"client request disabled",now);
    if(!caps.Allows(d->requiredCapability))return Deny(r,AuthorityDecisionCode::MissingCapability,"actor lacks required capability",now);
    std::size_t bytes=r.payload.IsNil()?0:r.payload.ToJson().size();if(bytes>d->maxPayloadBytes)return Deny(r,AuthorityDecisionCode::PayloadTooLarge,"payload exceeds action limit",now);
    auto key=r.actorId+"\n"+r.actionId;auto&s=state[key];
    if(d->requireSequence){if(s.hasSequence&&r.sequence<=s.lastSequence)return Deny(r,AuthorityDecisionCode::OutOfOrder,"sequence is stale or duplicated",now);s.lastSequence=r.sequence;s.hasSequence=true;}
    if(d->replayProtected){if(r.nonce.empty())return Deny(r,AuthorityDecisionCode::ReplayDetected,"missing replay nonce",now);if(s.nonceSet.count(r.nonce))return Deny(r,AuthorityDecisionCode::ReplayDetected,"replayed nonce",now);s.nonceSet.insert(r.nonce);s.recentNonces.push_back(r.nonce);while(s.recentNonces.size()>256){s.nonceSet.erase(s.recentNonces.front());s.recentNonces.pop_front();}}
    const float window=1.0f;if(s.requestCount==0||now-s.windowStart>=window){s.windowStart=now;s.requestCount=0;}int limit=std::max(d->burst,(int)std::ceil(d->maxRequestsPerSecond));if(s.requestCount>=limit)return Deny(r,AuthorityDecisionCode::RateLimited,"action rate limit exceeded",now);++s.requestCount;
    return{AuthorityDecisionCode::Allowed,true,"allowed"};
}
AuthorityDecision ServerAuthoritySystem::ValidateAuthoritativeCommit(const std::string&id,const AuthorityActionRegistry&reg)const{const auto*d=reg.Find(id);if(!d)return{AuthorityDecisionCode::UnknownAction,false,"unknown action"};if(d->serverAuthoritative&&hostRole==HostAuthorityRole::Client)return{AuthorityDecisionCode::ClientCannotCommit,false,"client cannot commit server-authoritative state"};return{AuthorityDecisionCode::Allowed,true,"allowed"};}

void VekRegisterAuthorityLibrary(VekScriptEngine& e,AuthorityActionRegistry* a,ReplicationSchemaRegistry*r){if(a)a->RegisterNatives(e);if(r)r->RegisterNatives(e);e.RegisterNative("security_tier_name",[](const std::vector<VekValue>&x){int v=x.empty()?1:(int)x[0].AsNumber(1);return VekValue(v==2?"hardened_server":(v==0?"development":"hardened_client"));});}

} // namespace vek
