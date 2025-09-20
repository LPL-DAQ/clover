use anyhow::Result;
use tokio::net::TcpListener;

pub struct Server {
    listener: TcpListener,
}

impl Server {
    pub async fn new() -> Result<Self> {
        let listener = TcpListener::bind("127.0.0.1:8080").await?;

        Ok(Server {
            listener
        })
    }

    pub async fn serve(&self) -> ! {
        loop {

        }
    }
}
