"""
Cutter MCP Server - Main server implementation
Supports stdio, SSE, and HTTP transports
"""

import asyncio
import json
import logging
from typing import Optional, Dict, Any, List
from enum import Enum

from .tools import CutterTools
from .models import Project, Function, Graph, Comment

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)


class TransportType(Enum):
    """MCP Transport types"""
    STDIO = "stdio"
    SSE = "sse"
    HTTP = "http"


class CutterMCPServer:
    """
    MCP Server for Cutter integration.
    Provides tools for reverse engineering tasks through MCP protocol.
    """

    def __init__(self, cutter_core=None, transport: TransportType = TransportType.STDIO):
        """
        Initialize MCP Server

        Args:
            cutter_core: Cutter core instance (from cutter.core())
            transport: Transport type (stdio, sse, http)
        """
        self.tools = CutterTools(cutter_core)
        self.transport = transport
        self.running = False

        # Register tools
        self.tool_handlers = {
            "open_project": self._handle_open_project,
            "save_project": self._handle_save_project,
            "get_functions": self._handle_get_functions,
            "rename_function": self._handle_rename_function,
            "add_comment": self._handle_add_comment,
            "generate_callgraph": self._handle_generate_callgraph,
            "generate_cfg": self._handle_generate_cfg,
            "export_analysis": self._handle_export_analysis,
        }

    def get_tool_definitions(self) -> List[Dict[str, Any]]:
        """Get MCP tool definitions"""
        return [
            {
                "name": "open_project",
                "description": "Open or create a Cutter project",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "path": {
                            "type": "string",
                            "description": "Project file path (.cutter) or binary file path"
                        },
                        "binary": {
                            "type": "string",
                            "description": "Optional binary file path (if path is a project file)"
                        }
                    },
                    "required": ["path"]
                }
            },
            {
                "name": "save_project",
                "description": "Save the current project",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "path": {
                            "type": "string",
                            "description": "Optional save path (defaults to current project path)"
                        }
                    }
                }
            },
            {
                "name": "get_functions",
                "description": "Get list of functions in the binary",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "limit": {
                            "type": "integer",
                            "description": "Maximum number of functions to return",
                            "default": 100
                        },
                        "offset": {
                            "type": "integer",
                            "description": "Starting offset",
                            "default": 0
                        }
                    }
                }
            },
            {
                "name": "rename_function",
                "description": "Rename a function",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "address": {
                            "type": "string",
                            "description": "Function address (hex string or decimal)"
                        },
                        "new_name": {
                            "type": "string",
                            "description": "New function name"
                        }
                    },
                    "required": ["address", "new_name"]
                }
            },
            {
                "name": "add_comment",
                "description": "Add a comment at an address",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "address": {
                            "type": "string",
                            "description": "Address to comment (hex string or decimal)"
                        },
                        "content": {
                            "type": "string",
                            "description": "Comment content"
                        },
                        "comment_type": {
                            "type": "string",
                            "description": "Type of comment (regular, code, function, variable)",
                            "default": "regular"
                        }
                    },
                    "required": ["address", "content"]
                }
            },
            {
                "name": "generate_callgraph",
                "description": "Generate call graph (DOT format)",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "address": {
                            "type": "string",
                            "description": "Function address (optional, None for global)"
                        },
                        "depth": {
                            "type": "integer",
                            "description": "Graph depth",
                            "default": 2
                        }
                    }
                }
            },
            {
                "name": "generate_cfg",
                "description": "Generate control flow graph for a function",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "address": {
                            "type": "string",
                            "description": "Function address (hex string or decimal)"
                        }
                    },
                    "required": ["address"]
                }
            },
            {
                "name": "export_analysis",
                "description": "Export analysis report",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "output_path": {
                            "type": "string",
                            "description": "Output file path"
                        },
                        "format": {
                            "type": "string",
                            "description": "Output format (json, markdown, text)",
                            "default": "json"
                        }
                    }
                }
            }
        ]

    def _parse_address(self, address_str: str) -> int:
        """Parse address string (hex or decimal)"""
        if isinstance(address_str, int):
            return address_str
        address_str = str(address_str).strip()
        if address_str.startswith('0x') or address_str.startswith('0X'):
            return int(address_str, 16)
        try:
            return int(address_str)
        except ValueError:
            return int(address_str, 16)

    async def _handle_open_project(self, args: Dict[str, Any]) -> Dict[str, Any]:
        """Handle open_project tool call"""
        try:
            path = args.get("path")
            binary = args.get("binary")

            if not path:
                return {"error": "path is required"}

            project = self.tools.open_project(path, binary)
            return {
                "success": True,
                "project": project.to_dict()
            }
        except Exception as e:
            logger.error(f"Error in open_project: {e}")
            return {"error": str(e)}

    async def _handle_save_project(self, args: Dict[str, Any]) -> Dict[str, Any]:
        """Handle save_project tool call"""
        try:
            path = args.get("path")
            result = self.tools.save_project(path)
            return {"success": True, "message": result}
        except Exception as e:
            logger.error(f"Error in save_project: {e}")
            return {"error": str(e)}

    async def _handle_get_functions(self, args: Dict[str, Any]) -> Dict[str, Any]:
        """Handle get_functions tool call"""
        try:
            limit = args.get("limit", 100)
            offset = args.get("offset", 0)

            functions = self.tools.get_functions(limit=limit, offset=offset)
            return {
                "success": True,
                "functions": [f.to_dict() for f in functions],
                "count": len(functions)
            }
        except Exception as e:
            logger.error(f"Error in get_functions: {e}")
            return {"error": str(e)}

    async def _handle_rename_function(self, args: Dict[str, Any]) -> Dict[str, Any]:
        """Handle rename_function tool call"""
        try:
            address = self._parse_address(args.get("address"))
            new_name = args.get("new_name")

            if not new_name:
                return {"error": "new_name is required"}

            success = self.tools.rename_function(address, new_name)
            return {"success": success}
        except Exception as e:
            logger.error(f"Error in rename_function: {e}")
            return {"error": str(e)}

    async def _handle_add_comment(self, args: Dict[str, Any]) -> Dict[str, Any]:
        """Handle add_comment tool call"""
        try:
            address = self._parse_address(args.get("address"))
            content = args.get("content")
            comment_type = args.get("comment_type", "regular")

            if not content:
                return {"error": "content is required"}

            success = self.tools.add_comment(address, content, comment_type)
            return {"success": success}
        except Exception as e:
            logger.error(f"Error in add_comment: {e}")
            return {"error": str(e)}

    async def _handle_generate_callgraph(self, args: Dict[str, Any]) -> Dict[str, Any]:
        """Handle generate_callgraph tool call"""
        try:
            address_str = args.get("address")
            depth = args.get("depth", 2)

            address = self._parse_address(address_str) if address_str else None
            graph = self.tools.generate_callgraph(address=address, depth=depth)

            return {
                "success": True,
                "graph": graph.to_dict(),
                "dot": graph.to_dot()
            }
        except Exception as e:
            logger.error(f"Error in generate_callgraph: {e}")
            return {"error": str(e)}

    async def _handle_generate_cfg(self, args: Dict[str, Any]) -> Dict[str, Any]:
        """Handle generate_cfg tool call"""
        try:
            address = self._parse_address(args.get("address"))
            graph = self.tools.generate_cfg(address)

            return {
                "success": True,
                "graph": graph.to_dict(),
                "dot": graph.to_dot()
            }
        except Exception as e:
            logger.error(f"Error in generate_cfg: {e}")
            return {"error": str(e)}

    async def _handle_export_analysis(self, args: Dict[str, Any]) -> Dict[str, Any]:
        """Handle export_analysis tool call"""
        try:
            output_path = args.get("output_path")
            format = args.get("format", "json")

            result = self.tools.export_analysis(output_path=output_path, format=format)

            if output_path:
                return {"success": True, "message": result}
            else:
                return {"success": True, "content": result}
        except Exception as e:
            logger.error(f"Error in export_analysis: {e}")
            return {"error": str(e)}

    async def handle_request(self, request: Dict[str, Any]) -> Dict[str, Any]:
        """
        Handle MCP request

        Args:
            request: MCP request dict

        Returns:
            Response dict
        """
        method = request.get("method")
        params = request.get("params", {})
        request_id = request.get("id")

        logger.info(f"Handling request: {method}")

        if method == "initialize":
            return {
                "jsonrpc": "2.0",
                "id": request_id,
                "result": {
                    "protocolVersion": "2024-11-05",
                    "capabilities": {
                        "tools": {}
                    },
                    "serverInfo": {
                        "name": "cutter-mcp-server",
                        "version": "1.0.0"
                    }
                }
            }

        elif method == "tools/list":
            return {
                "jsonrpc": "2.0",
                "id": request_id,
                "result": {
                    "tools": self.get_tool_definitions()
                }
            }

        elif method == "tools/call":
            tool_name = params.get("name")
            tool_args = params.get("arguments", {})

            handler = self.tool_handlers.get(tool_name)
            if not handler:
                return {
                    "jsonrpc": "2.0",
                    "id": request_id,
                    "error": {
                        "code": -32601,
                        "message": f"Unknown tool: {tool_name}"
                    }
                }

            result = await handler(tool_args)
            return {
                "jsonrpc": "2.0",
                "id": request_id,
                "result": {
                    "content": [
                        {
                            "type": "text",
                            "text": json.dumps(result, indent=2)
                        }
                    ]
                }
            }

        else:
            return {
                "jsonrpc": "2.0",
                "id": request_id,
                "error": {
                    "code": -32601,
                    "message": f"Unknown method: {method}"
                }
            }

    async def run_stdio(self):
        """Run server with stdio transport"""
        logger.info("Starting Cutter MCP Server (stdio)")
        self.running = True

        reader = asyncio.StreamReader()
        protocol = asyncio.StreamReaderProtocol(reader)
        await asyncio.get_event_loop().connect_read_pipe(lambda: protocol, asyncio.sys.stdin)

        w_transport, w_protocol = await asyncio.get_event_loop().connect_write_pipe(
            asyncio.streams.FlowControlMixin, asyncio.sys.stdout
        )
        writer = asyncio.StreamWriter(w_transport, w_protocol, reader, asyncio.get_event_loop())

        while self.running:
            try:
                line = await reader.readline()
                if not line:
                    break

                request = json.loads(line.decode())
                response = await self.handle_request(request)
                response_str = json.dumps(response) + "\n"
                writer.write(response_str.encode())
                await writer.drain()

            except Exception as e:
                logger.error(f"Error handling request: {e}")
                break

    async def run_sse(self, host: str = "localhost", port: int = 8080):
        """Run server with SSE transport"""
        try:
            from aiohttp import web
        except ImportError:
            raise ImportError("aiohttp is required for SSE transport. Install with: pip install aiohttp")

        logger.info(f"Starting Cutter MCP Server (SSE) on {host}:{port}")

        async def handle_sse(request):
            response = web.StreamResponse()
            response.headers['Content-Type'] = 'text/event-stream'
            response.headers['Cache-Control'] = 'no-cache'
            response.headers['Connection'] = 'keep-alive'

            await response.prepare(request)

            while True:
                try:
                    data = await request.content.readline()
                    if not data:
                        break

                    request_data = json.loads(data.decode())
                    response_data = await self.handle_request(request_data)

                    await response.write(f"data: {json.dumps(response_data)}\n\n".encode())

                except Exception as e:
                    logger.error(f"SSE error: {e}")
                    break

            return response

        app = web.Application()
        app.router.add_post('/mcp', handle_sse)

        runner = web.AppRunner(app)
        await runner.setup()
        site = web.TCPSite(runner, host, port)
        await site.start()

        # Keep running
        while self.running:
            await asyncio.sleep(1)

    async def run_http(self, host: str = "localhost", port: int = 8080):
        """Run server with HTTP transport"""
        try:
            from aiohttp import web
        except ImportError:
            raise ImportError("aiohttp is required for HTTP transport. Install with: pip install aiohttp")

        logger.info(f"Starting Cutter MCP Server (HTTP) on {host}:{port}")

        async def handle_request(request):
            try:
                data = await request.json()
                response = await self.handle_request(data)
                return web.json_response(response)
            except Exception as e:
                logger.error(f"HTTP error: {e}")
                return web.json_response({"error": str(e)}, status=500)

        app = web.Application()
        app.router.add_post('/mcp', handle_request)

        runner = web.AppRunner(app)
        await runner.setup()
        site = web.TCPSite(runner, host, port)
        await site.start()

        # Keep running
        while self.running:
            await asyncio.sleep(1)

    async def run(self, **kwargs):
        """Run server with configured transport"""
        if self.transport == TransportType.STDIO:
            await self.run_stdio()
        elif self.transport == TransportType.SSE:
            await self.run_sse(**kwargs)
        elif self.transport == TransportType.HTTP:
            await self.run_http(**kwargs)
        else:
            raise ValueError(f"Unknown transport: {self.transport}")

    def stop(self):
        """Stop the server"""
        self.running = False


def create_server(cutter_core=None, transport: str = "stdio") -> CutterMCPServer:
    """
    Factory function to create MCP server

    Args:
        cutter_core: Cutter core instance
        transport: Transport type (stdio, sse, http)

    Returns:
        CutterMCPServer instance
    """
    transport_type = TransportType(transport)
    return CutterMCPServer(cutter_core, transport_type)


def main():
    """Main entry point"""
    import sys
    import argparse

    parser = argparse.ArgumentParser(description="Cutter MCP Server")
    parser.add_argument("--transport", choices=["stdio", "sse", "http"], default="stdio")
    parser.add_argument("--host", default="localhost")
    parser.add_argument("--port", type=int, default=8080)

    args = parser.parse_args()

    # Try to import Cutter
    try:
        import cutter
        core = cutter.core()
    except ImportError:
        logger.warning("Cutter not available, running in standalone mode")
        core = None

    server = create_server(core, args.transport)

    try:
        if args.transport == "stdio":
            asyncio.run(server.run())
        else:
            asyncio.run(server.run(host=args.host, port=args.port))
    except KeyboardInterrupt:
        logger.info("Server stopped by user")
        server.stop()


if __name__ == "__main__":
    main()
