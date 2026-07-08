# Cutter MCP Server

Model Context Protocol (MCP) server integration for Cutter reverse engineering tool. This allows AI agents to interact with Cutter through a standardized protocol.

## Features

- **Project Management**: Open, save, and manage Cutter projects
- **Function Analysis**: List, search, and rename functions
- **Comment Management**: Add and manage comments at any address
- **Graph Generation**: Generate call graphs and control flow graphs in DOT format
- **Analysis Export**: Export comprehensive analysis reports in JSON, Markdown, or text format
- **Multiple Transports**: Support for stdio, SSE, and HTTP transports

## Installation

### Prerequisites

- Python 3.8+
- Cutter with Python support enabled
- Optional: `aiohttp` for SSE/HTTP transports

### Install from source

```bash
cd ctf-mcp-repos/cutter
pip install -e .
```

### Manual installation

```bash
# Install dependencies
pip install mcp

# For SSE/HTTP transport support
pip install aiohttp
```

## Usage

### As a Cutter Plugin

The MCP server can be loaded as a Cutter plugin. Add the `src/mcp` directory to your Cutter plugins path.

### Standalone Server

```bash
# Run with stdio transport (default)
python -m src.mcp.server

# Run with SSE transport
python -m src.mcp.server --transport sse --host localhost --port 8080

# Run with HTTP transport
python -m src.mcp.server --transport http --host localhost --port 8080
```

### Python API

```python
from src.mcp.server import create_server
import cutter

# Create server with Cutter core
core = cutter.core()
server = create_server(core, transport="stdio")

# Run server
import asyncio
asyncio.run(server.run())
```

## Available Tools

### 1. open_project

Open or create a Cutter project.

**Parameters:**
- `path` (string, required): Project file path (.cutter) or binary file path
- `binary` (string, optional): Binary file path (if path is a project file)

**Example:**
```json
{
  "name": "open_project",
  "arguments": {
    "path": "/path/to/binary"
  }
}
```

### 2. save_project

Save the current project.

**Parameters:**
- `path` (string, optional): Save path (defaults to current project path)

**Example:**
```json
{
  "name": "save_project",
  "arguments": {
    "path": "/path/to/save.cutter"
  }
}
```

### 3. get_functions

Get list of functions in the binary.

**Parameters:**
- `limit` (integer, optional): Maximum number of functions (default: 100)
- `offset` (integer, optional): Starting offset (default: 0)

**Example:**
```json
{
  "name": "get_functions",
  "arguments": {
    "limit": 50,
    "offset": 0
  }
}
```

### 4. rename_function

Rename a function.

**Parameters:**
- `address` (string, required): Function address (hex or decimal)
- `new_name` (string, required): New function name

**Example:**
```json
{
  "name": "rename_function",
  "arguments": {
    "address": "0x401000",
    "new_name": "main_entry"
  }
}
```

### 5. add_comment

Add a comment at an address.

**Parameters:**
- `address` (string, required): Address to comment (hex or decimal)
- `content` (string, required): Comment content
- `comment_type` (string, optional): Type (regular, code, function, variable)

**Example:**
```json
{
  "name": "add_comment",
  "arguments": {
    "address": "0x401000",
    "content": "Entry point of the program",
    "comment_type": "function"
  }
}
```

### 6. generate_callgraph

Generate call graph in DOT format.

**Parameters:**
- `address` (string, optional): Function address (None for global)
- `depth` (integer, optional): Graph depth (default: 2)

**Example:**
```json
{
  "name": "generate_callgraph",
  "arguments": {
    "address": "0x401000",
    "depth": 3
  }
}
```

### 7. generate_cfg

Generate control flow graph for a function.

**Parameters:**
- `address` (string, required): Function address (hex or decimal)

**Example:**
```json
{
  "name": "generate_cfg",
  "arguments": {
    "address": "0x401000"
  }
}
```

### 8. export_analysis

Export analysis report.

**Parameters:**
- `output_path` (string, optional): Output file path
- `format` (string, optional): Output format (json, markdown, text)

**Example:**
```json
{
  "name": "export_analysis",
  "arguments": {
    "output_path": "report.md",
    "format": "markdown"
  }
}
```

## AI Agent Integration

### Claude Integration

```python
import asyncio
from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client

async def analyze_binary():
    server_params = StdioServerParameters(
        command="python",
        args=["-m", "src.mcp.server"]
    )

    async with stdio_client(server_params) as (read, write):
        async with ClientSession(read, write) as session:
            await session.initialize()

            # Open binary
            await session.call_tool("open_project", {
                "path": "/path/to/target"
            })

            # Get functions
            result = await session.call_tool("get_functions", {
                "limit": 100
            })

            # Generate call graph
            graph = await session.call_tool("generate_callgraph", {
                "address": "0x401000"
            })

            # Export report
            await session.call_tool("export_analysis", {
                "output_path": "analysis.md",
                "format": "markdown"
            })

asyncio.run(analyze_binary())
```

### Example: Automated Analysis Workflow

```python
async def automated_analysis(binary_path: str):
    """Complete analysis workflow"""

    # 1. Open binary
    await session.call_tool("open_project", {"path": binary_path})

    # 2. Get all functions
    functions = await session.call_tool("get_functions", {"limit": 1000})

    # 3. Rename suspicious functions
    for func in functions['functions']:
        if "crypto" in func['name'].lower():
            await session.call_tool("rename_function", {
                "address": func['address'],
                "new_name": f"crypto_{func['name']}"
            })

    # 4. Add comments to key functions
    await session.call_tool("add_comment", {
        "address": "0x401000",
        "content": "Main entry point - initializes crypto context"
    })

    # 5. Generate call graph
    callgraph = await session.call_tool("generate_callgraph", {})

    # 6. Export final report
    await session.call_tool("export_analysis", {
        "output_path": "final_report.md",
        "format": "markdown"
    })
```

## Testing

Run the test suite:

```bash
# Install test dependencies
pip install pytest pytest-asyncio

# Run tests
pytest tests/test_mcp_server.py -v

# Run with coverage
pytest tests/test_mcp_server.py --cov=src.mcp --cov-report=term-missing
```

## Architecture

```
src/mcp/
├── __init__.py       # Package initialization
├── models.py         # Data models (Project, Function, Graph, Comment)
├── tools.py          # Cutter tools wrapper
└── server.py         # MCP server implementation

tests/
└── test_mcp_server.py  # Comprehensive test suite
```

### Components

- **models.py**: Data structures for projects, functions, graphs, and comments
- **tools.py**: High-level API wrapping Cutter's Python API
- **server.py**: MCP protocol server with stdio/SSE/HTTP transports

## Development

### Adding New Tools

1. Add tool implementation in `tools.py`
2. Add handler in `server.py`
3. Add tool definition in `get_tool_definitions()`
4. Add tests in `tests/test_mcp_server.py`

### Code Style

- Use type hints
- Follow PEP 8
- Write docstrings for all public functions
- Add tests for new functionality

## Troubleshooting

### Cutter Python API not available

If you see "Cutter not available" warning, ensure:
- Cutter is built with Python support (`-DCUTTER_ENABLE_PYTHON=ON`)
- Python bindings are in your Python path

### Transport errors

For SSE/HTTP transport errors:
```bash
pip install aiohttp
```

### Tests failing

Ensure test dependencies are installed:
```bash
pip install pytest pytest-asyncio
```

## License

This MCP server follows the same license as Cutter (GPLv3).

## Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Submit a pull request

## Resources

- [Cutter Documentation](https://cutter.re/docs/)
- [MCP Protocol](https://modelcontextprotocol.io/)
- [Rizin Documentation](https://rizin.re/docs/)
