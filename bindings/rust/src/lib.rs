use std::ffi::{CStr,CString};
use std::os::raw::{c_char,c_double,c_int};
#[repr(C)] #[derive(Clone,Copy)] pub struct VekValue{pub kind:c_int,pub number:c_double,pub boolean:c_int,pub string_value:*const c_char}
#[repr(C)] pub struct RawRuntime{_private:[u8;0]}
extern "C"{fn vek_create()->*mut RawRuntime;fn vek_destroy(r:*mut RawRuntime);fn vek_load_file(r:*mut RawRuntime,p:*const c_char)->c_int;fn vek_call(r:*mut RawRuntime,n:*const c_char,a:*const VekValue,c:usize)->VekValue;fn vek_last_error(r:*mut RawRuntime)->*const c_char;}
pub struct Runtime{raw:*mut RawRuntime}
impl Runtime{pub fn new()->Result<Self,String>{let r=unsafe{vek_create()};if r.is_null(){Err("VEK runtime creation failed".into())}else{Ok(Self{raw:r})}}pub fn load_file(&mut self,path:&str)->Result<(),String>{let p=CString::new(path).map_err(|e|e.to_string())?;if unsafe{vek_load_file(self.raw,p.as_ptr())}!=0{Ok(())}else{Err(self.last_error())}}pub fn call_number(&mut self,name:&str,args:&[f64])->Result<f64,String>{let n=CString::new(name).map_err(|e|e.to_string())?;let a:Vec<VekValue>=args.iter().map(|v|VekValue{kind:1,number:*v,boolean:0,string_value:std::ptr::null()}).collect();let r=unsafe{vek_call(self.raw,n.as_ptr(),a.as_ptr(),a.len())};if r.kind==1{Ok(r.number)}else{Err(self.last_error())}}fn last_error(&self)->String{unsafe{let p=vek_last_error(self.raw);if p.is_null(){String::new()}else{CStr::from_ptr(p).to_string_lossy().into_owned()}}}}
impl Drop for Runtime{fn drop(&mut self){unsafe{vek_destroy(self.raw)}}}
