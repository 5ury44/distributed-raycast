#!/usr/bin/env python3
"""
Distributed 3D Cube Game - Confirms load distribution to EKS workers
Shows actual 3D cubes with distributed raycasting calculations
"""

import grpc
import time
import sys
import os
import threading
import pygame
import math
import queue
from concurrent.futures import ThreadPoolExecutor
from dotenv import load_dotenv

# Load environment variables
load_dotenv()

# Add the parent directory to the Python path to import generated protobuf files
sys.path.append(os.path.join(os.path.dirname(__file__), '..', 'packages', 'worker'))

import worker_service_pb2 as worker_pb2
import worker_service_pb2_grpc as worker_grpc_pb2

class Distributed3DCubeGame:
    def __init__(self):
        # Load configuration from environment variables
        self.worker_endpoint = os.getenv('WORKER_ENDPOINT', 'localhost:50051')
        
        # Game state
        self.player_x = 8.0
        self.player_y = 8.0
        self.player_angle = 0.0
        self.player_pitch = 0.0
        self.move_speed = float(os.getenv('MOVE_SPEED', '0.1'))
        self.rot_speed = float(os.getenv('ROTATION_SPEED', '0.05'))
        
        # Display settings
        self.screen_width = int(os.getenv('SCREEN_WIDTH', '1024'))
        self.screen_height = int(os.getenv('SCREEN_HEIGHT', '768'))
        self.fov = float(os.getenv('FOV', '60.0'))
        
        # 3D cube world - define actual 3D cubes in the world
        self.cubes = [
            # Format: (x, y, z, width, height, depth, color)
            (5, 5, 0, 2, 2, 2, (255, 100, 100)),  # Red cube
            (12, 8, 0, 1.5, 1.5, 1.5, (100, 255, 100)),  # Green cube
            (8, 12, 0, 1, 1, 1, (100, 100, 255)),  # Blue cube
            (15, 15, 0, 2.5, 2.5, 2.5, (255, 255, 100)),  # Yellow cube
            (3, 15, 0, 1, 1, 1, (255, 100, 255)),  # Magenta cube
        ]
        
        # Room boundaries (walls)
        self.room_bounds = {
            'min_x': 1, 'max_x': 18,
            'min_y': 1, 'max_y': 18,
            'min_z': 0, 'max_z': 10
        }
        
        # Performance tracking
        self.frame_count = 0
        self.last_fps_time = time.time()
        self.fps = 0.0
        self.total_requests = 0
        self.total_response_time = 0.0
        self.worker_stats = {}
        
        # Distributed rendering
        self.worker_stubs = []
        self.request_queue = queue.Queue()
        self.result_queue = queue.Queue()
        self.running = True
        self.worker_threads = []
        
        # Load additional configuration
        self.max_worker_connections = int(os.getenv('MAX_WORKER_CONNECTIONS', '3'))
        self.fps_limit = int(os.getenv('FPS_LIMIT', '60'))
        
        # Pygame setup
        pygame.init()
        self.screen = pygame.display.set_mode((self.screen_width, self.screen_height))
        pygame.display.set_caption("Distributed 3D Cube Game - Load Distribution")
        self.clock = pygame.time.Clock()
        self.font = pygame.font.Font(None, 24)
        
        print("🎮 Distributed 3D Cube Game - Load Distribution")
        print("=" * 55)
        print(f"🔌 Worker endpoint: {self.worker_endpoint}")
        print(f"📐 Display resolution: {self.screen_width}x{self.screen_height}")
        print(f"🎯 Controls: WASD to move, Mouse to look, ESC to exit")
        print(f"🧊 Cubes: {len(self.cubes)} 3D cubes in the world")
        print(f"🏠 Room: {self.room_bounds['max_x'] - self.room_bounds['min_x']}x{self.room_bounds['max_y'] - self.room_bounds['min_y']} units")
        
        # Initialize worker connections
        self.connect_to_workers()
        self.start_worker_threads()
    
    def connect_to_workers(self):
        """Connect to distributed workers"""
        try:
            # Create multiple connections for load balancing
            for i in range(self.max_worker_connections):  # Use configurable number of connections
                channel = grpc.insecure_channel(self.worker_endpoint)
                worker_stub = worker_grpc_pb2.WorkerServiceStub(channel)
                self.worker_stubs.append(worker_stub)
            
            print(f"✅ Connected to worker service: {self.worker_endpoint}")
            print(f"🔧 Created {len(self.worker_stubs)} worker connections for load balancing")
        except Exception as e:
            print(f"❌ Failed to connect to workers: {e}")
    
    def start_worker_threads(self):
        """Start worker threads for concurrent processing"""
        num_threads = len(self.worker_stubs)
        for i in range(num_threads):
            thread = threading.Thread(target=self.worker_thread, args=(i,))
            thread.daemon = True
            thread.start()
            self.worker_threads.append(thread)
        
        print(f"🚀 Started {num_threads} worker threads for concurrent processing")
    
    def worker_thread(self, thread_id):
        """Worker thread for processing raycast requests"""
        while self.running:
            try:
                # Get request from queue
                request_data = self.request_queue.get(timeout=0.1)
                if request_data is None:
                    break
                
                start_time = time.time()
                
                # Create new connection for this request
                channel = grpc.insecure_channel(self.worker_endpoint)
                worker_stub = worker_grpc_pb2.WorkerServiceStub(channel)
                
                # Create render request
                request = worker_pb2.RenderRequest()
                request.request_id = f"distributed_3d_{request_data['request_id']}"
                request.player_id = "distributed_3d_player"
                
                # Create player object
                player = worker_pb2.Player()
                player.x = self.player_x
                player.y = self.player_y
                player.angle = self.player_angle
                player.pitch = self.player_pitch
                player.id = "distributed_3d_player"
                player.timestamp = int(time.time() * 1000)
                request.player.CopyFrom(player)
                
                request.screen_width = request_data['screen_width']
                request.screen_height = request_data['screen_height']
                request.fov = self.fov
                request.start_column = request_data['start_column']
                request.end_column = request_data['end_column']
                
                # Add map data (simplified for 3D cubes)
                map_data = [
                    [1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1],
                    [1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
                    [1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
                    [1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
                    [1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
                    [1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
                    [1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
                    [1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
                    [1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
                    [1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
                    [1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
                    [1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
                    [1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
                    [1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
                    [1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
                    [1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
                    [1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
                    [1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
                    [1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1],
                    [1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1]
                ]
                
                for row in map_data:
                    request.map.extend(row)
                request.map_width = 20
                request.map_height = 20
                request.timestamp = int(time.time() * 1000)
                
                # Send to worker
                response = worker_stub.ProcessRenderRequest(request, timeout=5)
                
                end_time = time.time()
                response_time_ms = (end_time - start_time) * 1000
                
                # Close connection
                channel.close()
                
                # Track worker usage
                worker_id = response.worker_id
                self.worker_stats[worker_id] = self.worker_stats.get(worker_id, 0) + 1
                self.total_requests += 1
                self.total_response_time += response_time_ms
                
                # Put result in queue
                self.result_queue.put({
                    'success': True,
                    'worker_id': worker_id,
                    'response_time': response_time_ms,
                    'request_id': request_data['request_id'],
                    'results': response.results,
                    'thread_id': thread_id,
                    'start_column': request_data['start_column'],
                    'end_column': request_data['end_column']
                })
                
                # Print worker usage
                if self.total_requests % 20 == 0:  # Print every 20 requests
                    print(f"🔧 Worker {worker_id} (Thread {thread_id}) processed request #{self.total_requests} in {response_time_ms:.1f}ms")
                
            except queue.Empty:
                continue
            except Exception as e:
                self.result_queue.put({
                    'success': False,
                    'error': str(e),
                    'request_id': request_data.get('request_id', 'unknown'),
                    'thread_id': thread_id
                })
    
    def raycast_to_cube(self, start_x, start_y, angle, max_distance=20):
        """Local raycast to find intersection with 3D cubes"""
        dx = math.cos(angle)
        dy = math.sin(angle)
        
        # Step along the ray
        step_size = 0.1
        distance = 0
        
        while distance < max_distance:
            # Check collision with room boundaries
            if (start_x + dx * distance < self.room_bounds['min_x'] or 
                start_x + dx * distance > self.room_bounds['max_x'] or
                start_y + dy * distance < self.room_bounds['min_y'] or 
                start_y + dy * distance > self.room_bounds['max_y']):
                return {
                    'distance': distance,
                    'wall_type': 1,
                    'color': (200, 200, 200),
                    'cube_id': 'wall'
                }
            
            # Check collision with cubes
            ray_x = start_x + dx * distance
            ray_y = start_y + dy * distance
            
            for i, cube in enumerate(self.cubes):
                cube_x, cube_y, cube_z, cube_w, cube_h, cube_d, cube_color = cube
                
                # Check if ray intersects with cube (2D projection)
                if (cube_x <= ray_x <= cube_x + cube_w and 
                    cube_y <= ray_y <= cube_y + cube_h):
                    return {
                        'distance': distance,
                        'wall_type': 2,
                        'color': cube_color,
                        'cube_id': f'cube_{i}',
                        'cube_info': cube
                    }
            
            distance += step_size
        
        return None
    
    def render_3d_scene(self):
        """Render the 3D scene with distributed raycasting"""
        # Clear screen
        self.screen.fill((0, 0, 0))
        
        # Draw sky (gradient from light blue to darker blue)
        for y in range(self.screen_height // 2):
            color_intensity = int(135 + (y / (self.screen_height // 2)) * 50)
            color = (color_intensity, color_intensity + 20, 255)
            pygame.draw.line(self.screen, color, (0, y), (self.screen_width, y))
        
        # Draw floor (gradient from dark to lighter)
        for y in range(self.screen_height // 2, self.screen_height):
            floor_y = y - self.screen_height // 2
            color_intensity = int(85 - (floor_y / (self.screen_height // 2)) * 30)
            color = (color_intensity, color_intensity, color_intensity)
            pygame.draw.line(self.screen, color, (0, y), (self.screen_width, y))
        
        # Send distributed raycast requests
        self.send_distributed_requests()
        
        # Process results and render
        self.process_distributed_results()
    
    def send_distributed_requests(self):
        """Send raycast requests to workers"""
        # Divide screen into chunks for distributed processing
        chunk_size = self.screen_width // 4  # 4 chunks
        
        for i in range(4):
            start_col = i * chunk_size
            end_col = min((i + 1) * chunk_size - 1, self.screen_width - 1)
            
            request_data = {
                'request_id': f"chunk_{i}_{int(time.time() * 1000)}",
                'screen_width': end_col - start_col + 1,
                'screen_height': self.screen_height,
                'start_column': start_col,
                'end_column': end_col
            }
            
            self.request_queue.put(request_data)
    
    def process_distributed_results(self):
        """Process results from distributed workers"""
        results_processed = 0
        current_frame_data = {}
        
        while not self.result_queue.empty():
            try:
                result = self.result_queue.get_nowait()
                results_processed += 1
                
                if result['success']:
                    # Store results by column
                    for i, ray_result in enumerate(result['results']):
                        if ray_result.distance > 0:
                            current_frame_data[result['start_column'] + i] = ray_result
                    
                    print(f"✅ Worker {result['worker_id']} (Thread {result['thread_id']}) completed chunk in {result['response_time']:.1f}ms")
                else:
                    print(f"❌ Worker error: {result['error']}")
                    
            except queue.Empty:
                break
        
        if results_processed > 0:
            self.frame_count += results_processed
        
        # Render the frame with distributed results
        self.draw_distributed_frame(current_frame_data)
    
    def draw_distributed_frame(self, frame_data):
        """Draw the frame using distributed results"""
        for x in range(self.screen_width):
            if x in frame_data:
                result = frame_data[x]
                
                # Calculate wall height based on distance
                line_height = int(self.screen_height / result.distance)
                draw_start = -line_height // 2 + self.screen_height // 2
                draw_end = line_height // 2 + self.screen_height // 2
                
                # Clamp to screen bounds
                draw_start = max(0, draw_start)
                draw_end = min(self.screen_height, draw_end)
                
                # Apply distance shading
                intensity = max(0.3, 1.0 - result.distance / 15.0)
                color = (int(200 * intensity), int(200 * intensity), int(200 * intensity))
                
                # Draw the wall slice
                pygame.draw.line(self.screen, color, (x, draw_start), (x, draw_end), 2)
            else:
                # Fallback to local raycast for missing columns
                ray_angle = self.player_angle + math.atan((x - self.screen_width // 2) / (self.screen_width // 2)) * (self.fov * math.pi / 180)
                hit = self.raycast_to_cube(self.player_x, self.player_y, ray_angle)
                
                if hit:
                    line_height = int(self.screen_height / hit['distance'])
                    draw_start = -line_height // 2 + self.screen_height // 2
                    draw_end = line_height // 2 + self.screen_height // 2
                    
                    draw_start = max(0, draw_start)
                    draw_end = min(self.screen_height, draw_end)
                    
                    intensity = max(0.3, 1.0 - hit['distance'] / 15.0)
                    color = tuple(int(c * intensity) for c in hit['color'])
                    
                    pygame.draw.line(self.screen, color, (x, draw_start), (x, draw_end), 2)
    
    def draw_3d_minimap(self):
        """Draw a 3D-style minimap showing cubes and player"""
        minimap_size = 250
        minimap_x = self.screen_width - minimap_size - 10
        minimap_y = 10
        
        # Draw minimap background
        pygame.draw.rect(self.screen, (30, 30, 30), (minimap_x, minimap_y, minimap_size, minimap_size))
        pygame.draw.rect(self.screen, (255, 255, 255), (minimap_x, minimap_y, minimap_size, minimap_size), 2)
        
        # Calculate scale
        scale_x = minimap_size / (self.room_bounds['max_x'] - self.room_bounds['min_x'])
        scale_y = minimap_size / (self.room_bounds['max_y'] - self.room_bounds['min_y'])
        
        # Draw room boundaries
        room_x = minimap_x + int(self.room_bounds['min_x'] * scale_x)
        room_y = minimap_y + int(self.room_bounds['min_y'] * scale_y)
        room_w = int((self.room_bounds['max_x'] - self.room_bounds['min_x']) * scale_x)
        room_h = int((self.room_bounds['max_y'] - self.room_bounds['min_y']) * scale_y)
        pygame.draw.rect(self.screen, (100, 100, 100), (room_x, room_y, room_w, room_h), 2)
        
        # Draw cubes
        for i, cube in enumerate(self.cubes):
            cube_x, cube_y, cube_z, cube_w, cube_h, cube_d, cube_color = cube
            
            # Convert to minimap coordinates
            map_x = minimap_x + int(cube_x * scale_x)
            map_y = minimap_y + int(cube_y * scale_y)
            map_w = max(2, int(cube_w * scale_x))
            map_h = max(2, int(cube_h * scale_y))
            
            # Draw cube as rectangle
            pygame.draw.rect(self.screen, cube_color, (map_x, map_y, map_w, map_h))
            pygame.draw.rect(self.screen, (255, 255, 255), (map_x, map_y, map_w, map_h), 1)
            
            # Draw cube label
            label = self.font.render(f"C{i}", True, (255, 255, 255))
            self.screen.blit(label, (map_x, map_y - 15))
        
        # Draw player
        player_x = minimap_x + int(self.player_x * scale_x)
        player_y = minimap_y + int(self.player_y * scale_y)
        pygame.draw.circle(self.screen, (255, 0, 0), (player_x, player_y), 4)
        
        # Draw player direction
        dir_length = 15
        dir_x = player_x + int(math.cos(self.player_angle) * dir_length)
        dir_y = player_y + int(math.sin(self.player_angle) * dir_length)
        pygame.draw.line(self.screen, (255, 0, 0), (player_x, player_y), (dir_x, dir_y), 3)
        
        # Draw FOV cone
        fov_rad = math.radians(self.fov)
        left_angle = self.player_angle - fov_rad / 2
        right_angle = self.player_angle + fov_rad / 2
        
        left_x = player_x + int(math.cos(left_angle) * dir_length)
        left_y = player_y + int(math.sin(left_angle) * dir_length)
        right_x = player_x + int(math.cos(right_angle) * dir_length)
        right_y = player_y + int(math.sin(right_angle) * dir_length)
        
        pygame.draw.line(self.screen, (255, 255, 0), (player_x, player_y), (left_x, left_y), 1)
        pygame.draw.line(self.screen, (255, 255, 0), (player_x, player_y), (right_x, right_y), 1)
    
    def draw_ui(self):
        """Draw UI overlay with stats and worker information"""
        # FPS counter
        fps_text = self.font.render(f"FPS: {self.fps:.1f}", True, (255, 255, 255))
        self.screen.blit(fps_text, (10, 10))
        
        # Request count
        req_text = self.font.render(f"Requests: {self.total_requests}", True, (255, 255, 255))
        self.screen.blit(req_text, (10, 35))
        
        # Player position
        pos_text = self.font.render(f"Pos: ({self.player_x:.1f}, {self.player_y:.1f})", True, (255, 255, 255))
        self.screen.blit(pos_text, (10, 60))
        
        # Angle
        angle_text = self.font.render(f"Angle: {math.degrees(self.player_angle):.1f}°", True, (255, 255, 255))
        self.screen.blit(angle_text, (10, 85))
        
        # Worker distribution
        if self.worker_stats:
            worker_text = f"Workers: {', '.join([f'W{worker_id}:{count}' for worker_id, count in sorted(self.worker_stats.items())])}"
            if len(worker_text) > 50:
                worker_text = worker_text[:47] + "..."
            worker_surface = self.font.render(worker_text, True, (0, 255, 0))
            self.screen.blit(worker_surface, (10, 110))
        
        # Average response time
        if self.total_requests > 0:
            avg_response = self.total_response_time / self.total_requests
            response_text = self.font.render(f"Avg Response: {avg_response:.1f}ms", True, (255, 255, 0))
            self.screen.blit(response_text, (10, 135))
        
        # Cube count
        cube_text = self.font.render(f"Cubes: {len(self.cubes)}", True, (255, 255, 255))
        self.screen.blit(cube_text, (10, 160))
        
        # Room info
        room_text = self.font.render(f"Room: {self.room_bounds['max_x'] - self.room_bounds['min_x']}x{self.room_bounds['max_y'] - self.room_bounds['min_y']}", True, (255, 255, 255))
        self.screen.blit(room_text, (10, 185))
        
        # Controls
        controls_text = self.font.render("WASD: Move, Mouse: Look, ESC: Exit", True, (255, 255, 255))
        self.screen.blit(controls_text, (10, self.screen_height - 25))
        
        # Cube legend
        legend_y = self.screen_height - 120
        legend_text = self.font.render("Cubes:", True, (255, 255, 255))
        self.screen.blit(legend_text, (10, legend_y))
        
        for i, cube in enumerate(self.cubes):
            cube_x, cube_y, cube_z, cube_w, cube_h, cube_d, cube_color = cube
            cube_info = f"C{i}: ({cube_x},{cube_y}) {cube_w}x{cube_h}x{cube_d}"
            cube_surface = self.font.render(cube_info, True, cube_color)
            self.screen.blit(cube_surface, (10, legend_y + 20 + i * 15))
    
    def handle_input(self, keys, mouse_rel):
        """Handle keyboard and mouse input with collision detection"""
        old_x = self.player_x
        old_y = self.player_y
        old_angle = self.player_angle
        
        # Movement
        if keys[pygame.K_w]:
            self.player_x += self.move_speed * math.cos(self.player_angle)
            self.player_y += self.move_speed * math.sin(self.player_angle)
        if keys[pygame.K_s]:
            self.player_x -= self.move_speed * math.cos(self.player_angle)
            self.player_y -= self.move_speed * math.sin(self.player_angle)
        if keys[pygame.K_a]:
            self.player_x += self.move_speed * math.sin(self.player_angle)
            self.player_y -= self.move_speed * math.cos(self.player_angle)
        if keys[pygame.K_d]:
            self.player_x -= self.move_speed * math.sin(self.player_angle)
            self.player_y += self.move_speed * math.cos(self.player_angle)
        
        # Mouse look
        if mouse_rel[0] != 0:
            self.player_angle += mouse_rel[0] * 0.002
        if mouse_rel[1] != 0:
            self.player_pitch -= mouse_rel[1] * 0.002
            self.player_pitch = max(-math.pi/2, min(math.pi/2, self.player_pitch))
        
        # Collision detection with room boundaries
        if (self.player_x < self.room_bounds['min_x'] or 
            self.player_x > self.room_bounds['max_x'] or
            self.player_y < self.room_bounds['min_y'] or 
            self.player_y > self.room_bounds['max_y']):
            self.player_x = old_x
            self.player_y = old_y
        
        # Collision detection with cubes
        for cube in self.cubes:
            cube_x, cube_y, cube_z, cube_w, cube_h, cube_d, cube_color = cube
            
            if (cube_x <= self.player_x <= cube_x + cube_w and 
                cube_y <= self.player_y <= cube_y + cube_h):
                # Player is inside a cube, push them out
                self.player_x = old_x
                self.player_y = old_y
                break
    
    def run_distributed_game(self):
        """Main game loop with distributed rendering"""
        print("\n🎮 Starting Distributed 3D Cube Game...")
        print("   Use WASD to move, mouse to look around")
        print("   Press ESC to exit")
        print("   Watch the worker distribution in the console!")
        
        # Hide mouse cursor
        pygame.mouse.set_visible(False)
        pygame.event.set_grab(True)
        
        try:
            while True:
                # Handle events
                for event in pygame.event.get():
                    if event.type == pygame.QUIT:
                        return
                    elif event.type == pygame.KEYDOWN:
                        if event.key == pygame.K_ESCAPE:
                            return
                    elif event.type == pygame.MOUSEBUTTONDOWN:
                        if event.button == 1:  # Left click
                            pygame.event.set_grab(True)
                            pygame.mouse.set_visible(False)
                
                # Get input
                keys = pygame.key.get_pressed()
                mouse_rel = pygame.mouse.get_rel()
                
                # Handle input
                self.handle_input(keys, mouse_rel)
                
                # Render 3D scene with distributed processing
                self.render_3d_scene()
                
                # Draw minimap
                self.draw_3d_minimap()
                
                # Draw UI
                self.draw_ui()
                
                # Update display
                pygame.display.flip()
                
                # Update FPS
                self.frame_count += 1
                now = time.time()
                if now - self.last_fps_time >= 1.0:
                    self.fps = self.frame_count / (now - self.last_fps_time)
                    self.frame_count = 0
                    self.last_fps_time = now
                
                # Limit FPS
                self.clock.tick(self.fps_limit)
                
        except KeyboardInterrupt:
            pass
        finally:
            # Cleanup
            self.running = False
            for _ in range(len(self.worker_threads)):
                self.request_queue.put(None)
            
            for thread in self.worker_threads:
                thread.join(timeout=1)
            
            pygame.quit()
        
        print("\n👋 Distributed 3D game ended!")
        print(f"📊 Final stats: {self.total_requests} requests, {self.fps:.1f} FPS")
        print(f"📊 Worker distribution: {dict(self.worker_stats)}")

def main():
    try:
        game = Distributed3DCubeGame()
        game.run_distributed_game()
    except Exception as e:
        print(f"❌ Error: {e}")
        return 1
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
