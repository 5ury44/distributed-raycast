# docker/worker.Dockerfile
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
    libsdl2-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy source code
COPY packages/worker /app/worker
COPY packages/shared /app/shared

# Build the worker
WORKDIR /app
RUN mkdir build && cd build && \
    cmake ../worker && \
    make -j$(nproc)

# Runtime stage
FROM ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libgrpc++1 \
    libprotobuf23 \
    libsdl2-2.0-0 \
    && rm -rf /var/lib/apt/lists/*

# Copy built binary
COPY --from=builder /app/build/raycast_worker /usr/local/bin/

# Create non-root user
RUN useradd -m -u 1000 worker
USER worker

EXPOSE 50051

CMD ["raycast_worker"]