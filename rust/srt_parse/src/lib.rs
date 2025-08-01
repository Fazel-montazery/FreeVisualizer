use core::str;
use std::ffi::CStr;
use std::fs;
use std::os::raw::c_char;

// Types
pub struct TimeStamp {
    h: u32,
    m: u32,
    s: u32,
    ms: u32,
}

pub struct TimePeriod {
    start: TimeStamp,
    end: TimeStamp,
}

pub struct Section {
    num: u32,
    period: TimePeriod,
}

// Helper
fn break_sections(srt_text: String) -> Vec<String> {
    let lines = srt_text.trim().lines();
    let mut sections: Vec<String> = Vec::new();
    let mut section: String = String::new();
    for line in lines {
        let line = line.trim();
        if line.is_empty() {
            if section.ends_with('\n') {
                section.pop();
            }
            if !section.is_empty() {
                sections.push(section)
            };
            section = String::new();
        } else {
            section.push_str(line);
            section.push('\n');
        }
    }
    sections
}

// Main interface
#[unsafe(no_mangle)]
pub extern "C" fn process_srt(path: *const c_char) {
    let path = unsafe { CStr::from_ptr(path) };
    let path = path.to_str().expect("Unicode conversion failed.");

    let srt_text = match fs::read_to_string(path) {
        Ok(str) => str,
        Err(err) => {
            println!("There was a problem reading {} => {}", path, err);
            return;
        }
    };

    let sections: Vec<String> = break_sections(srt_text);
}
