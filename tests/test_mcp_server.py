"""
Tests for Cutter MCP Server
"""

import pytest
import json
import asyncio
from unittest.mock import Mock, MagicMock, patch

# Import MCP modules
from src.mcp.models import (
    Project, Function, Graph, GraphNode, GraphEdge,
    Comment, CommentType, AnalysisReport
)
from src.mcp.tools import CutterTools
from src.mcp.server import CutterMCPServer, TransportType, create_server


class TestModels:
    """Test data models"""

    def test_project_creation(self):
        """Test Project model creation"""
        project = Project(
            name="test_project",
            path="/path/to/project.cutter",
            binary="/path/to/binary"
        )
        assert project.name == "test_project"
        assert project.path == "/path/to/project.cutter"
        assert project.binary == "/path/to/binary"

    def test_project_to_dict(self):
        """Test Project to_dict"""
        project = Project(name="test", path="/test")
        data = project.to_dict()
        assert data["name"] == "test"
        assert data["path"] == "/test"

    def test_function_creation(self):
        """Test Function model creation"""
        func = Function(
            address=0x401000,
            name="main",
            size=100
        )
        assert func.address == 0x401000
        assert func.name == "main"
        assert func.size == 100

    def test_function_to_dict(self):
        """Test Function to_dict"""
        func = Function(address=0x401000, name="main", size=100)
        data = func.to_dict()
        assert data["address"] == "0x401000"
        assert data["name"] == "main"
        assert data["size"] == 100

    def test_graph_to_dot(self):
        """Test Graph to_dot"""
        graph = Graph(name="TestGraph")
        graph.nodes.append(GraphNode(id="n1", label="Node 1"))
        graph.nodes.append(GraphNode(id="n2", label="Node 2"))
        graph.edges.append(GraphEdge(source="n1", target="n2"))

        dot = graph.to_dot()
        assert "digraph TestGraph" in dot
        assert "n1" in dot
        assert "n2" in dot
        assert "n1 -> n2" in dot

    def test_comment_creation(self):
        """Test Comment model creation"""
        comment = Comment(
            address=0x401000,
            content="Test comment",
            comment_type=CommentType.REGULAR
        )
        assert comment.address == 0x401000
        assert comment.content == "Test comment"
        assert comment.comment_type == CommentType.REGULAR

    def test_analysis_report_to_markdown(self):
        """Test AnalysisReport to_markdown"""
        project = Project(name="test", path="/test")
        report = AnalysisReport(project=project)
        report.functions.append(Function(address=0x401000, name="main"))

        md = report.to_markdown()
        assert "# Analysis Report: test" in md
        assert "main" in md


class TestCutterTools:
    """Test CutterTools"""

    def test_tools_initialization(self):
        """Test CutterTools initialization"""
        tools = CutterTools()
        assert tools.core is None
        assert tools.current_project is None

    def test_tools_with_mock_core(self):
        """Test CutterTools with mock core"""
        mock_core = Mock()
        mock_core.cmd.return_value = "test output"

        tools = CutterTools(mock_core)
        assert tools.core is not None

    def test_open_project_binary(self):
        """Test open_project with binary"""
        mock_core = Mock()
        mock_core.cmd.return_value = ""

        tools = CutterTools(mock_core)
        project = tools.open_project("/path/to/binary")

        assert project.name == "binary"
        assert project.binary == "/path/to/binary"
        mock_core.cmd.assert_called()

    def test_open_project_cutter_file(self):
        """Test open_project with .cutter file"""
        mock_core = Mock()
        mock_core.cmd.return_value = ""

        tools = CutterTools(mock_core)
        project = tools.open_project("/path/to/project.cutter")

        assert project.name == "project"
        assert project.path == "/path/to/project.cutter"

    def test_save_project(self):
        """Test save_project"""
        mock_core = Mock()
        mock_core.cmd.return_value = ""

        tools = CutterTools(mock_core)
        tools.open_project("/path/to/binary")
        result = tools.save_project("/path/to/save.cutter")

        assert "saved" in result.lower()

    def test_get_functions(self):
        """Test get_functions"""
        mock_core = Mock()
        mock_core.cmd.return_value = json.dumps([
            {"offset": 0x401000, "name": "main", "size": 100},
            {"offset": 0x401100, "name": "foo", "size": 50}
        ])

        # Mock cmdj to return parsed JSON
        mock_core.cmdj = Mock(return_value=[
            {"offset": 0x401000, "name": "main", "size": 100},
            {"offset": 0x401100, "name": "foo", "size": 50}
        ])

        tools = CutterTools(mock_core)
        tools.open_project("/path/to/binary")
        functions = tools.get_functions()

        assert len(functions) == 2
        assert functions[0].name == "main"
        assert functions[1].name == "foo"

    def test_rename_function(self):
        """Test rename_function"""
        mock_core = Mock()
        mock_core.cmd.return_value = ""

        tools = CutterTools(mock_core)
        tools.open_project("/path/to/binary")
        result = tools.rename_function(0x401000, "new_main")

        assert result is True
        mock_core.cmd.assert_called()

    def test_add_comment(self):
        """Test add_comment"""
        mock_core = Mock()
        mock_core.cmd.return_value = ""

        tools = CutterTools(mock_core)
        tools.open_project("/path/to/binary")
        result = tools.add_comment(0x401000, "Test comment")

        assert result is True
        mock_core.cmd.assert_called()

    def test_generate_callgraph(self):
        """Test generate_callgraph"""
        mock_core = Mock()
        mock_core.cmd.return_value = json.dumps({
            "nodes": [
                {"offset": 0x401000, "name": "main"},
                {"offset": 0x401100, "name": "foo"}
            ],
            "edges": [
                {"from": 0x401000, "to": 0x401100}
            ]
        })

        tools = CutterTools(mock_core)
        tools.open_project("/path/to/binary")
        graph = tools.generate_callgraph()

        assert len(graph.nodes) == 2
        assert len(graph.edges) == 1

    def test_generate_cfg(self):
        """Test generate_cfg"""
        mock_core = Mock()
        mock_core.cmd.return_value = json.dumps({
            "blocks": [
                {"offset": 0x401000, "size": 20, "jump": 0x401020},
                {"offset": 0x401020, "size": 10}
            ]
        })

        tools = CutterTools(mock_core)
        tools.open_project("/path/to/binary")
        graph = tools.generate_cfg(0x401000)

        assert len(graph.nodes) == 2
        assert len(graph.edges) >= 1

    def test_export_analysis(self):
        """Test export_analysis"""
        mock_core = Mock()
        mock_core.cmd.return_value = ""
        mock_core.cmdj = Mock(return_value=[])

        tools = CutterTools(mock_core)
        tools.open_project("/path/to/binary")

        # Export as JSON
        result = tools.export_analysis(format="json")
        assert "project" in result

        # Export as Markdown
        result = tools.export_analysis(format="markdown")
        assert "# Analysis Report" in result


class TestMCPServer:
    """Test CutterMCPServer"""

    def test_server_initialization(self):
        """Test server initialization"""
        server = CutterMCPServer()
        assert server.transport == TransportType.STDIO
        assert len(server.tool_handlers) == 8

    def test_get_tool_definitions(self):
        """Test get_tool_definitions"""
        server = CutterMCPServer()
        tools = server.get_tool_definitions()

        assert len(tools) == 8
        tool_names = [t["name"] for t in tools]
        assert "open_project" in tool_names
        assert "save_project" in tool_names
        assert "get_functions" in tool_names

    def test_parse_address_hex(self):
        """Test _parse_address with hex"""
        server = CutterMCPServer()
        assert server._parse_address("0x401000") == 0x401000
        assert server._parse_address("0X401000") == 0x401000

    def test_parse_address_decimal(self):
        """Test _parse_address with decimal"""
        server = CutterMCPServer()
        assert server._parse_address("4198400") == 4198400

    @pytest.mark.asyncio
    async def test_handle_initialize(self):
        """Test handle initialize request"""
        server = CutterMCPServer()
        request = {
            "method": "initialize",
            "params": {},
            "id": 1
        }

        response = await server.handle_request(request)
        assert response["result"]["protocolVersion"] == "2024-11-05"
        assert response["result"]["serverInfo"]["name"] == "cutter-mcp-server"

    @pytest.mark.asyncio
    async def test_handle_tools_list(self):
        """Test handle tools/list request"""
        server = CutterMCPServer()
        request = {
            "method": "tools/list",
            "params": {},
            "id": 1
        }

        response = await server.handle_request(request)
        assert len(response["result"]["tools"]) == 8

    @pytest.mark.asyncio
    async def test_handle_open_project(self):
        """Test handle open_project tool call"""
        mock_core = Mock()
        mock_core.cmd.return_value = ""

        server = CutterMCPServer(mock_core)
        request = {
            "method": "tools/call",
            "params": {
                "name": "open_project",
                "arguments": {"path": "/path/to/binary"}
            },
            "id": 1
        }

        response = await server.handle_request(request)
        assert "result" in response

    @pytest.mark.asyncio
    async def test_handle_get_functions(self):
        """Test handle get_functions tool call"""
        mock_core = Mock()
        mock_core.cmd.return_value = ""
        mock_core.cmdj = Mock(return_value=[])

        server = CutterMCPServer(mock_core)

        # First open a project
        await server.handle_request({
            "method": "tools/call",
            "params": {
                "name": "open_project",
                "arguments": {"path": "/path/to/binary"}
            },
            "id": 1
        })

        # Then get functions
        request = {
            "method": "tools/call",
            "params": {
                "name": "get_functions",
                "arguments": {"limit": 10}
            },
            "id": 2
        }

        response = await server.handle_request(request)
        assert "result" in response

    @pytest.mark.asyncio
    async def test_handle_unknown_tool(self):
        """Test handle unknown tool"""
        server = CutterMCPServer()
        request = {
            "method": "tools/call",
            "params": {
                "name": "unknown_tool",
                "arguments": {}
            },
            "id": 1
        }

        response = await server.handle_request(request)
        assert "error" in response
        assert response["error"]["code"] == -32601

    def test_create_server(self):
        """Test create_server factory"""
        server = create_server(None, "stdio")
        assert server.transport == TransportType.STDIO

        server = create_server(None, "sse")
        assert server.transport == TransportType.SSE

        server = create_server(None, "http")
        assert server.transport == TransportType.HTTP


class TestIntegration:
    """Integration tests"""

    @pytest.mark.asyncio
    async def test_full_workflow(self):
        """Test full workflow: open -> analyze -> export"""
        mock_core = Mock()
        mock_core.cmd.return_value = ""
        mock_core.cmdj = Mock(return_value=[])

        server = CutterMCPServer(mock_core)

        # Open project
        response = await server.handle_request({
            "method": "tools/call",
            "params": {
                "name": "open_project",
                "arguments": {"path": "/path/to/binary"}
            },
            "id": 1
        })
        assert "result" in response

        # Get functions
        response = await server.handle_request({
            "method": "tools/call",
            "params": {
                "name": "get_functions",
                "arguments": {}
            },
            "id": 2
        })
        assert "result" in response

        # Export analysis
        response = await server.handle_request({
            "method": "tools/call",
            "params": {
                "name": "export_analysis",
                "arguments": {"format": "json"}
            },
            "id": 3
        })
        assert "result" in response


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
