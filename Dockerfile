# syntax=docker/dockerfile:1

# This file specifies the dev container used for LPL flight software development. It includes all dependencies
# required to build, test, and flash.

FROM ubuntu:noble

# Sensible utilities that every system should probably have
RUN << EOF
apt update
apt -y upgrade
apt -y install sudo nano git curl wget jq yq iproute2 netcat-openbsd gh
EOF

# Zephyr dependencies -- from https://docs.zephyrproject.org/latest/develop/getting_started/index.html, but we
# replace gcc/g++ with gcc-arm-none-eabi because in practice we're just building for Cortex M7's.
RUN << EOF
apt -y install --no-install-recommends git cmake ninja-build gperf \
    ccache dfu-util device-tree-compiler python3-dev python3-venv python3-tk \
    xz-utils file make gcc-arm-none-eabi libsdl2-dev libmagic1
EOF
ENV ZEPHYR_TOOLCHAIN_VARIANT=cross-compile CROSS_COMPILE=/usr/bin/arm-none-eabi-

# C/C++ linters, static analysis
RUN << EOF
apt -y install cppcheck clang-tidy
EOF

# Create non-root user
RUN << EOF
useradd --create-home --shell /bin/bash --gid 1000 --groups sudo lpl
echo lpl ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/lpl
chmod 0440 /etc/sudoers.d/lpl
EOF
USER lpl

# Python
RUN << EOF
curl -LsSf https://astral.sh/uv/install.sh | sh
uv tool install ruff@latest
EOF

# Rust
RUN << EOF
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
. "$HOME/.cargo/env"
rustup component add rustfmt
EOF

# Cosmetic changes
RUN << EOF
# Remove sudo tutorial from startup
touch ~/.sudo_as_admin_successful

# Oh-my-bash terminal theme
bash -c "$(curl -fsSL https://raw.githubusercontent.com/ohmybash/oh-my-bash/a59433af4d5c68861d66b2b9dcf8ada7f1b0e6f5/tools/install.sh)" --unattended
mkdir -p ~/.oh-my-bash/themes/lpl-term
curl -s -o ~/.oh-my-bash/themes/lpl-term/lpl-term.theme.sh https://gist.githubusercontent.com/jamm-es/341e7649817aa18a1fc2b96a6b69bf00/raw/d637de3a6e8e03c997d2413c7dd236a3da163b56/lpl-term.theme.sh
sed -i 's/^OSH_THEME=.*$/OSH_THEME="lpl-term"/' ~/.bashrc

# LPL logo
cat << "END" >> ~/.bashrc
lplred="\e[31;1m"
lplylw="\e[33;1m"
lplbld="\e[39;1m"
lplrst="\e[0m"
echo -e "   ${lplred}__   ${lplylw}___ ${lplred} __
${lplred}  / /  ${lplylw}/ _ \\\\${lplred}/ /
${lplred} / /__${lplylw}/ ___${lplred}/ /__
${lplred}/____${lplylw}/_/  ${lplred}/____/
${lplbld}Welcome!${lplrst}"
END
EOF
