FROM ubuntu:24.04

# Install essential tools for C99 development and Ruby
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    pkg-config \
    ca-certificates \
    python3 \
    ruby \
    ruby-dev \
    && rm -rf /var/lib/apt/lists/*

# Install RuboCop gem for compatibility checks
RUN gem install rubocop --no-document

# Set the default shell
ENV SHELL /bin/bash
