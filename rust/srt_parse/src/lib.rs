use std::ffi::CStr;
use std::fs;
use std::os::raw::c_char;

// Types
#[repr(C)]
pub struct TimeStamp {
    h: u32,
    m: u32,
    s: u32,
    ms: u32,
}

#[repr(C)]
pub struct TimePeriod {
    start: TimeStamp,
    end: TimeStamp,
}

#[repr(C)]
pub struct Section {
    num: u32,
    period: TimePeriod,
}

#[repr(C)]
pub struct SrtHandle {
    sections_len: usize,
    sections_ptr: *mut Section,
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
    sections.push(section);
    sections
}

fn parse_sections(sections_text: Vec<String>) -> Vec<Section> {
    let mut sections: Vec<Section> = Vec::new();

    for section_text in sections_text {
        let section_num: u32;
        let mut section_time_period: TimePeriod = TimePeriod {
            start: TimeStamp {
                h: 0,
                m: 0,
                s: 0,
                ms: 0,
            },
            end: TimeStamp {
                h: 0,
                m: 0,
                s: 0,
                ms: 0,
            },
        };
        let mut section_lines = section_text.lines();
        if let Some(num_line) = section_lines.next() {
            section_num = match num_line.parse() {
                Ok(value) => value,
                Err(err) => {
                    println!(
                        "srt_parse::There was a problen parsing this line::{} => {}",
                        num_line, err
                    );
                    continue;
                }
            }
        } else {
            continue;
        }

        if let Some(time_line) = section_lines.next() {
            let parts: Vec<&str> = time_line.split(" --> ").collect();
            if parts.len() != 2 {
                println!(
                    "srt_parse::There was a problem parsing this line::{}",
                    time_line
                );
                continue;
            }

            let segments = parts[0].split([':', ',']);

            let maybe_numbers: Option<Vec<u32>> = segments.map(|s| s.parse::<u32>().ok()).collect();

            let numbers = match maybe_numbers {
                Some(nums) if nums.len() == 4 => nums,
                _ => {
                    println!(
                        "srt_parse::There was a problem parsing this line::{}",
                        time_line
                    );
                    continue;
                }
            };

            section_time_period.start = TimeStamp {
                h: numbers[0],
                m: numbers[1],
                s: numbers[2],
                ms: numbers[3],
            };

            let segments = parts[1].split([':', ',']);

            let maybe_numbers: Option<Vec<u32>> = segments.map(|s| s.parse::<u32>().ok()).collect();

            let numbers = match maybe_numbers {
                Some(nums) if nums.len() == 4 => nums,
                _ => {
                    println!(
                        "srt_parse::There was a problem parsing this line::{}",
                        time_line
                    );
                    continue;
                }
            };

            section_time_period.end = TimeStamp {
                h: numbers[0],
                m: numbers[1],
                s: numbers[2],
                ms: numbers[3],
            };
        } else {
            println!(
                "srt_parse::There was a problem parsing section number::{}",
                section_num
            );
            continue;
        }

        let mut rem = String::new();
        for r in section_lines {
            rem.push_str(r);
        }

        sections.push(Section {
            num: section_num,
            period: section_time_period,
        });
    }

    sections
}

// Main interface
#[unsafe(no_mangle)]
pub extern "C" fn process_srt(path: *const c_char) -> SrtHandle {
    let mut srt_handle: SrtHandle = SrtHandle {
        sections_len: 0,
        sections_ptr: std::ptr::null_mut(),
    };

    let path = unsafe { CStr::from_ptr(path) };
    let path = match path.to_str() {
        Ok(str) => str,
        Err(_) => {
            println!("srt_parse::Unicode conversion failed.");
            return srt_handle;
        }
    };

    let srt_text = match fs::read_to_string(path) {
        Ok(str) => str,
        Err(err) => {
            println!("srt_parse::There was a problem reading {} => {}", path, err);
            return srt_handle;
        }
    };

    let sections_text: Vec<String> = break_sections(srt_text);
    let mut sections: Vec<Section> = parse_sections(sections_text);
    sections.shrink_to_fit();
    srt_handle.sections_len = sections.len();
    let ptr = sections.as_mut_ptr();
    std::mem::forget(sections);
    srt_handle.sections_ptr = ptr;

    srt_handle
}

#[unsafe(no_mangle)]
pub extern "C" fn free_srt(srt_handle: SrtHandle) {
    if srt_handle.sections_ptr.is_null() {
        return;
    }

    unsafe {
        let slice: &mut [Section] =
            std::slice::from_raw_parts_mut(srt_handle.sections_ptr, srt_handle.sections_len);
        let boxed: Box<[Section]> = Box::from_raw(slice);
        drop(boxed);
    }
}
