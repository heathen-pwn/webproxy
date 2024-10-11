# Web Proxy API

This project is a custom-built web proxy API designed to handle URL requests, forward them to the intended destination, and return the results to the user. The backend is written in C, leveraging `libcurl` for HTTP operations and `json-c` for configuration management. The API is optimized for high performance, with short-lived session handling and inter-website hopping for a smooth transition across websites through the proxy. 

## Features

- **High-Performance Proxy**: Developed in C for speed and efficiency.
- **Session Management**: Lightweight session handling with hash tables and dynamic resizing, supporting both persistent and non-persistent HTTP connections.
- **Configurable Settings**: The `json-c` library is used for configuration management, allowing easy adjustments to session thresholds, timeouts, and table sizes.
- **Thread Management**: Implements a thread pool to handle requests efficiently, with garbage collection for session cleanup.
- **Docker Support**: The application runs in a Docker container for isolated and consistent environment deployment.

## Prerequisites

- **Docker**: Ensure Docker is installed for deployment.
- **libcurl**: Required for handling HTTP requests.
- **json-c**: For configuration management.
- **GCC**: For compiling the C code.
  
## Setup and Installation

1. Clone this repository:
   ```bash
   git clone https://github.com/yourusername/web-proxy-api.git
   cd web-proxy-api
   ```

2. Build and run the Docker container:
   ```bash
   docker build -t web-proxy-api .
   docker run -p <port>:<port> web-proxy-api
   ```

3. (Optional) Adjust configurations by modifying `config.json`. Key configuration options include:
   - `default_table_size`: Sets the initial hash table size for session management.
   - `port`: Defines the API's listening port.
   - `sessions_threshold` and `sessions_timeout`: Set session behavior parameters.

## Usage

1. Access the API via `/proxy?q=xxxx`. It only responds to requests matching this format.
2. The proxy handles binary data in requests without null terminators, and uses an efficient memory management system to ensure high performance.
3. All data is handled in-memory with no hard disk footprint.

## Session Management

- Sessions are managed in-memory using hash tables and UUIDs for session identifiers. 
- The API uses a garbage collector running in a separate thread to clean up expired sessions.
- Dynamic resizing ensures the hash table adjusts based on session load.

## Security

- Data served by the proxy is sanitized to prevent unexpected redirections by HTML and/or Javascript directives.

## Contributing

Contributions are welcome. Fork the project, submit issues, or make pull requests. 

## License

This project is licensed under the MIT License. See the LICENSE file for details.
