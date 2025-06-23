# Specifies the dev container used for LPL flight software development. Includes all dependencies required to 
# build and flash

FROM ubuntu:noble

# Sensible packages that every system should probably have
RUN apt update && \ 
    apt -y upgrade && \
    apt -y install sudo nano git curl wget

# Create non-root user
RUN groupadd --gid 1000 lpl && \
    useradd --create-home --shell /bin/bash --gid 1000 --groups sudo lpl && \
    echo lpl ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/lpl && \
    chmod 0440 /etc/sudoers.d/lpl
USER lpl

# Apply vanity theme
RUN bash -c "$(curl -fsSL https://raw.githubusercontent.com/ohmybash/oh-my-bash/a59433af4d5c68861d66b2b9dcf8ada7f1b0e6f5/tools/install.sh)" --unattended && \
    mkdir -p ~/.oh-my-bash/themes/lpl-term && \
    curl -s -o ~/.oh-my-bash/themes/lpl-term/lpl-term.theme.sh https://gist.githubusercontent.com/jamm-es/341e7649817aa18a1fc2b96a6b69bf00/raw/d637de3a6e8e03c997d2413c7dd236a3da163b56/lpl-term.theme.sh && \
    sed -i 's/^OSH_THEME=.*$/OSH_THEME="lpl-term"/' ~/.bashrc


RUN diff 
