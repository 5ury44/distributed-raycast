# Environment Configuration

This project now uses environment variables for configuration instead of hardcoded values. This makes the application more flexible and secure.

## Setup

1. **Copy the example environment file:**

   ```bash
   cp .env.example .env
   ```

2. **Install Python dependencies:**

   ```bash
   pip install -r requirements.txt
   ```

3. **Update the `.env` file with your specific values:**
   - Replace `WORKER_ENDPOINT` with your actual load balancer endpoint
   - Adjust other values as needed for your environment

## Environment Variables

### Load Balancer Configuration

- `WORKER_ENDPOINT`: The endpoint URL for the worker service (e.g., `your-load-balancer:50051`)

### Server Configuration

- `WORKER_SERVER_ADDRESS`: Address the worker server binds to (default: `0.0.0.0:50051`)
- `WORKER_SERVICE_NAME`: Kubernetes service name for workers (default: `raycast-worker`)
- `WORKER_NAMESPACE`: Kubernetes namespace (default: `default`)

### Port Configuration

- `WORKER_PORT`: Port number for worker service (default: `50051`)
- `MASTER_PORT`: Port number for master service (default: `50052`)

### Health Check Configuration

- `HEALTH_CHECK_INTERVAL_SECONDS`: How often to check worker health (default: `30`)
- `HEALTH_CHECK_TIMEOUT_SECONDS`: Timeout for health checks (default: `5`)
- `REQUEST_TIMEOUT_SECONDS`: Timeout for render requests (default: `30`)

### Worker Pool Configuration

- `DISCOVERY_INTERVAL_SECONDS`: How often to discover new workers (default: `30`)
- `MAX_WORKER_CONNECTIONS`: Number of connections to create for load balancing (default: `3`)

### Game Configuration

- `SCREEN_WIDTH`: Game window width (default: `1024`)
- `SCREEN_HEIGHT`: Game window height (default: `768`)
- `FOV`: Field of view in degrees (default: `60.0`)
- `FPS_LIMIT`: Maximum FPS (default: `60`)

### Player Configuration

- `MOVE_SPEED`: Player movement speed (default: `0.1`)
- `ROTATION_SPEED`: Player rotation speed (default: `0.05`)

## Security Notes

- The `.env` file is included in `.gitignore` to prevent sensitive information from being committed
- Always use `.env.example` as a template for new environments
- Never commit actual credentials or sensitive endpoints to version control

## Running the Application

The application will automatically load environment variables from the `.env` file when started. You can also override any variable by setting it in your shell environment:

```bash
# Override a single variable
WORKER_ENDPOINT=localhost:50051 python local_client/distributed_3d_game.py

# Or export it for the session
export WORKER_ENDPOINT=localhost:50051
python local_client/distributed_3d_game.py
```
