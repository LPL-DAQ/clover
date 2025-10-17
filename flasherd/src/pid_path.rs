use dirs::data_dir;

pub(crate) fn pid_path() -> std::path::PathBuf {
    data_dir().unwrap().join("flasherd.pid")
}
