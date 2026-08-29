#include <vek/vek_c.h>
#include <assert.h>
#include <stdio.h>
int main(void){
    vek_runtime*r=vek_create();assert(r);
    assert(vek_set_security_tier(r,2));
    assert(vek_set_authority_role(r,3));
    assert(vek_load_source(r,
        "fn add(a,b){return a+b;} event Ping(x){return x+1;} "
        "fn secure_setup(){ authority_action_register({id:\"shop.buy\",server_authoritative:true,allow_client_request:true,require_sequence:true,replay_protected:true,max_requests_per_second:3,burst:3,max_payload_bytes:256,required_capability:\"shop.buy\"}); return true; }",
        "<c-test>"));
    vek_value a[2]={{VEK_NUMBER,2,0,0},{VEK_NUMBER,5,0,0}};vek_value z=vek_call(r,"add",a,2);assert(z.type==VEK_NUMBER&&z.number==7);
    assert(vek_has_event(r,"Ping"));vek_value p={VEK_NUMBER,9,0,0};z=vek_emit_event(r,"Ping",&p,1);assert(z.number==10);
    z=vek_call(r,"secure_setup",0,0);assert(z.type==VEK_BOOL&&z.boolean);assert(vek_authority_action_count(r)==1);
    assert(vek_authority_validate_request(r,"shop.buy","p1",1,"nonce-1","{item:wheel}","shop.buy",1.0));
    assert(!vek_authority_validate_request(r,"shop.buy","p1",1,"nonce-1","{item:wheel}","shop.buy",1.1));
    printf("VEK C ABI tests: PASS\n");vek_destroy(r);return 0;
}
