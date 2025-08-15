FROM ghcr.io/therainbowphoenix/gint-docker:main

# RUN echo "export SDK_DIR=${SDK_DIR}" >> ~/.bashrc
# RUN echo "export OLD_SDK_DIR=${OLD_SDK_DIR}" >> ~/.bashrc

# Switch to root to create user and install sudo
USER root

# Install sudo if not already installed
RUN apt-get update && apt-get install -y sudo && rm -rf /var/lib/apt/lists/*

# Create user 'dev' with password 'dev' and add to sudo group
RUN echo "dev:dev" | chpasswd \
    && usermod -aG sudo dev

# Optional: prevent sudo from asking for a password
RUN echo "dev ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

WORKDIR /workspace

