# ============================================================================
# DOCKERFILE FOR MYGIT SERVER
# ============================================================================
# This is a multi-stage build - it creates a tiny production image!
# Stage 1: Build the Go binary
# Stage 2: Copy only the binary to a minimal image

# STAGE 1: Builder
# ----------------
# Use official Go image for building
FROM golang:1.21-alpine AS builder

# Set working directory inside container
WORKDIR /app

# Copy go.mod first (for better caching)
COPY go.mod ./

# Copy source code
COPY *.go ./

# Build the binary
# CGO_ENABLED=0 = no C dependencies (pure Go)
# -ldflags="-s -w" = strip debug info (smaller binary)
RUN CGO_ENABLED=0 GOOS=linux go build -ldflags="-s -w" -o mygit-server

# STAGE 2: Production
# -------------------
# Use tiny Alpine image (only ~5MB!)
FROM alpine:latest

# Set working directory
WORKDIR /app

# Copy ONLY the binary from builder stage
COPY --from=builder /app/mygit-server .

# Create directory for git data (will be mounted as volume)
RUN mkdir -p .mygit

# Expose port 8080
EXPOSE 8080

# Run the server
CMD ["./mygit-server"]
