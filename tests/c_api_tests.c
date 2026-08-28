#include <vek/vek_c.h>
#include <assert.h>
#include <stdio.h>
int main(void){vek_runtime*r=vek_create();assert(r);assert(vek_load_source(r,"fn add(a,b){return a+b;} event Ping(x){return x+1;}","<c-test>"));vek_value a[2]={{VEK_NUMBER,2,0,0},{VEK_NUMBER,5,0,0}};vek_value z=vek_call(r,"add",a,2);assert(z.type==VEK_NUMBER&&z.number==7);assert(vek_has_event(r,"Ping"));vek_value p={VEK_NUMBER,9,0,0};z=vek_emit_event(r,"Ping",&p,1);assert(z.number==10);printf("VEK C ABI tests: PASS\n");vek_destroy(r);return 0;}
