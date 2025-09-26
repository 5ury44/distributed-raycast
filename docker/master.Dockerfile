FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    libprotobuf-dev \
    protobuf-compiler \
    libgrpc++-dev \
    libgrpc-dev \
    protobuf-compiler-grpc \
    && rm -rf /var/lib/apt/lists/*

# Copy source code
COPY packages/master /app/master
COPY packages/shared /app/shared
COPY packages/worker /app/worker

# Build the master server
WORKDIR /app
RUN mkdir build && cd build && \
    cmake ../master && \
    make -j$(nproc)

# Runtime stage
FROM ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libgrpc++1 \
    libprotobuf23 \
    && rm -rf /var/lib/apt/lists/*

# Copy built binary
COPY --from=builder /app/build/raycast_master /usr/local/bin/

# Create non-root user
RUN useradd -m -u 1000 master
USER master

EXPOSE 50052

CMD ["raycast_master"]
