mod pid_path;

use pid_path::pid_path;
use std::fs::read_to_string;
use sysinfo::{Pid, ProcessesToUpdate, System};

fn main() {
    let mut sys = System::new();
    let pid_path = pid_path();

    match read_to_string(&pid_path) {
        Err(err) => {
            if !matches!(err.kind(), std::io::ErrorKind::NotFound) {
                panic!("Failed to read pid path {pid_path:?}: {err:?}");
            }
            println!("[flasherd-cleaner] No pidfile found at {pid_path:?}");
        }
        Ok(pid) => {
            let pid = Pid::from(pid.parse::<usize>().unwrap());
            sys.refresh_processes(ProcessesToUpdate::Some(&[pid]), true);
            if let Some(process) = sys.process(pid)
                && let Some(bin_path) = process.exe()
                && let Some(bin_name) = bin_path.file_name()
                && bin_name.to_string_lossy().contains("flasherd")
            {
                println!("[flasherd-cleaner] Killing process {pid}...");
                _ = process.kill_and_wait().unwrap();
                println!("[flasherd-cleaner] Killed.");
            } else {
                println!(
                    "[flasherd-cleaner] No flasherd matching pid {pid} found, is it already dead?"
                )
            }
        }
    }
}
