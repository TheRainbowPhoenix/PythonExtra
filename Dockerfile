FROM ghcr.io/therainbowphoenix/gint-docker:main

USER root
ENV PATH="/root/.local/bin:$PATH"
WORKDIR /tmp/giteapc-install/giteapc
RUN python3 giteapc.py install Lephenixnoir/gint@dev -y
# RUN python3 giteapc.py install Lephenixnoir/JustUI -y


# RUN echo "export SDK_DIR=${SDK_DIR}" >> ~/.bashrc
# RUN echo "export OLD_SDK_DIR=${OLD_SDK_DIR}" >> ~/.bashrc

WORKDIR /workspace

