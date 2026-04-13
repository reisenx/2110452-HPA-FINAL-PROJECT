# Stage 1: Build Environment
FROM gcc:12 AS builder

# Set working directory
WORKDIR /app

# Copy the source code
COPY src/ /app/src/

# Compile the source code
RUN g++ -O3 -mavx2 -fopenmp src/solution.cpp -o solution

# Stage 2: Runtime Environment
FROM debian:bookworm-slim

# Set working directory
WORKDIR /app

# Install a library for C++ runtime and OpenMP
RUN apt-get update
RUN apt-get install -y libgomp1 libstdc++6 libgcc-s1

# Cleanup the package list after installation
RUN rm -rf /var/lib/apt/lists/*

# Copy only the compiled binary file from the builder
COPY --from=builder /app/solution /app/solution

# Command to run when the container starts
ENTRYPOINT [ "./solution" ]
