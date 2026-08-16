// FFI module: optional C-compatible exports
// This file can be used to expose Rodo functionality via C ABI for other languages.
// Currently, it's a placeholder.

use crate::rodo::Rodo;
use std::ffi::{CStr, CString};
use std::os::raw::c_char;

#[no_mangle]
pub extern "C" fn rodo_link(path: *const c_char) -> *mut Rodo {
    if path.is_null() {
        return std::ptr::null_mut();
    }
    let c_str = unsafe { CStr::from_ptr(path) };
    match c_str.to_str() {
        Ok(s) => match Rodo::link(s) {
            Ok(r) => Box::into_raw(Box::new(r)),
            Err(_) => std::ptr::null_mut(),
        },
        Err(_) => std::ptr::null_mut(),
    }
}

#[no_mangle]
pub extern "C" fn rodo_close(ptr: *mut Rodo) {
    if !ptr.is_null() {
        unsafe {
            let rodo = Box::from_raw(ptr);
            let _ = rodo.close();
        }
    }
}