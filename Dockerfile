# syntax=docker/dockerfile:1

# This file specifies the dev container used for LPL flight software development. It includes all dependencies
# required to build, test, and flash.

FROM ubuntu:noble

# Sensible utilities that every system should probably have
RUN << EOF
apt update
apt -y upgrade
apt -y install sudo nano git curl wget jq yq iproute2 netcat-openbsd gh locales
EOF
ENV LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8

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
. "$HOME/.local/bin/env"
cd $HOME && uv venv
uv tool install ruff@latest
uv pip install west
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
cat << "END" > ~/.oh-my-bash/themes/lpl-term/lpl-term.theme.sh
#! bash oh-my-bash.module

# Derivative of https://github.com/ohmyzsh/ohmyzsh/blob/master/themes/tonotdo.zsh-theme, but with a delta as the terminator.

SCM_THEME_PROMPT_PREFIX=" ${_omb_prompt_purple}"
SCM_THEME_PROMPT_SUFFIX=" ${_omb_prompt_normal}"
SCM_THEME_PROMPT_DIRTY=" ${_omb_prompt_brown}✗"
SCM_THEME_PROMPT_CLEAN=" ${_omb_prompt_green}✓"
SCM_GIT_SHOW_DETAILS="false"

function _omb_theme_PROMPT_COMMAND() {
  PS1="${_omb_prompt_olive}\u${_omb_prompt_normal}${_omb_prompt_teal}@\h${_omb_prompt_normal}${_omb_prompt_purple} ${_omb_prompt_normal}${_omb_prompt_green}\w${_omb_prompt_normal}${_omb_prompt_red} Δ${_omb_prompt_normal} "
}

_omb_util_add_prompt_command _omb_theme_PROMPT_COMMAND
END
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

# Ensure we are in venv
echo '. "$HOME/.venv/bin/activate"' >> "$HOME/.bashrc"
EOF
