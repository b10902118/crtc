FROM debian:trixie

ARG DEBIAN_FRONTEND=noninteractive

# Configure APT to be more robust against network/mirror issues
# Not tested (extremely slow)
#RUN echo 'Acquire::ForceIPv4 "true";' > /etc/apt/apt.conf.d/99force-ipv4 && \
#    echo 'Acquire::Retries "3";' > /etc/apt/apt.conf.d/80-retries

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
		libegl1-mesa-dev \
        libgles2-mesa-dev \
        cmake \
        clang \
        python3 \
        patchelf \
        unzip \
        file \
        gdb
        