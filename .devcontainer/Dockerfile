FROM ubuntu:24.04

# Install essential tools for C development and Ruby
RUN apt-get update && apt-get install -y \
    gcc \
    clang \
    make \
    git \
    ruby-full \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

# Set the default shell
ENV SHELL /bin/bash
