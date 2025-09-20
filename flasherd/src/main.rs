mod server;
use anyhow::Result;
use log::LevelFilter;
use server::Server;
use simplelog::{ColorChoice, CombinedLogger, Config, TermLogger, TerminalMode, WriteLogger};

#[tokio::main]

async fn main() -> Result<()> {
    std::fs::

    CombinedLogger::init(vec![
        TermLogger::new(LevelFilter::Info,
            Config::default(),
            TerminalMode::Mixed,
            ColorChoice::Auto
        ),
        WriteLogger::new(
            LevelFilter::Debug,
            Config::default(),

        )
    ]);

    let server = Server::new().await?;
    server.serve().await
}
