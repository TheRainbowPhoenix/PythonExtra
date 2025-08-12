FROM debian:stable-slim AS base-gpc

USER root

RUN apt-get update
RUN apt-get install curl python3-pil git python3 build-essential pkg-config -y

ENV PATH="/root/.local/bin:$PATH"
RUN mkdir /tmp/giteapc-install
WORKDIR /tmp/giteapc-install

RUN curl "https://git.planet-casio.com/Lephenixnoir/GiteaPC/archive/master.tar.gz" -o giteapc-master.tar.gz
RUN tar -xzf giteapc-master.tar.gz
WORKDIR /tmp/giteapc-install/giteapc
RUN python3 giteapc.py install Lephenixnoir/GiteaPC -y

# Install the C/C++ SDK and compiler
RUN apt install cmake python3-pil libusb-1.0-0-dev libsdl2-dev libudisks2-dev libglib2.0-dev libpng-dev libncurses5-dev -y
RUN apt install libmpfr-dev libmpc-dev libgmp-dev libppl-dev flex texinfo python-is-python3 -y

RUN python3 giteapc.py install Lephenixnoir/fxsdk Lephenixnoir/sh-elf-binutils Lephenixnoir/sh-elf-gcc Lephenixnoir/sh-elf-gdb -y

RUN python3 giteapc.py install Lephenixnoir/OpenLibm Vhex-Kernel-Core/fxlibc -y
RUN python3 giteapc.py install Lephenixnoir/sh-elf-gcc -y

RUN python3 giteapc.py install Lephenixnoir/gint -y
RUN python3 giteapc.py install Lephenixnoir/JustUI -y


RUN apt-get install less vim nano sudo tree clangd-13 clang-format-13 -qqy && apt-get -qqy clean

# Codespace
ENV USERNAME="dev"
RUN mkdir ~/tmp

RUN useradd -rm -d /home/$USERNAME -s /bin/bash -g root -G sudo -u 1001 -p "$(openssl passwd -1 ${USERNAME})" $USERNAME
SHELL ["/bin/bash", "-o", "pipefail", "-c"]

# USER $USERNAME
# WORKDIR /home/$USERNAME

# RUN echo "export SDK_DIR=${SDK_DIR}" >> ~/.bashrc
# RUN echo "export OLD_SDK_DIR=${OLD_SDK_DIR}" >> ~/.bashrc

WORKDIR /workspace

