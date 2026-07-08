"""
Cutter MCP Tools - Core tools for interacting with Cutter
"""

import json
import os
from typing import List, Optional, Dict, Any
from datetime import datetime

from .models import (
    Project, Function, Graph, GraphNode, GraphEdge,
    Comment, CommentType, AnalysisReport
)


class CutterTools:
    """
    Cutter tools wrapper for MCP integration.
    Provides high-level API for common Cutter operations.
    """

    def __init__(self, cutter_core=None):
        """
        Initialize CutterTools

        Args:
            cutter_core: Cutter core instance (from cutter.core() or mock)
        """
        self.core = cutter_core
        self.current_project: Optional[Project] = None

    def _ensure_project(self):
        """Ensure a project is open"""
        if not self.current_project:
            raise RuntimeError("No project is currently open. Use open_project first.")

    def _cmd(self, command: str) -> str:
        """Execute a Cutter command"""
        if self.core is None:
            raise RuntimeError("Cutter core not initialized")
        return self.core.cmd(command)

    def _cmdj(self, command: str) -> Any:
        """Execute a Cutter JSON command"""
        if self.core is None:
            raise RuntimeError("Cutter core not initialized")
        if hasattr(self.core, 'cmdj'):
            return self.core.cmdj(command)
        else:
            # Fallback for mock
            result = self.core.cmd(command)
            return json.loads(result) if result else {}

    def open_project(self, path: str, binary: Optional[str] = None) -> Project:
        """
        Open or create a Cutter project

        Args:
            path: Project file path (.cutter) or binary file path
            binary: Optional binary file path (if path is a project file)

        Returns:
            Project information
        """
        if path.endswith('.cutter') or path.endswith('.json'):
            # Open existing project
            if self.core:
                self._cmd(f"Po {path}")
            project_name = os.path.basename(path).replace('.cutter', '').replace('.json', '')
            self.current_project = Project(
                name=project_name,
                path=path,
                binary=binary,
                modified_at=datetime.now().isoformat()
            )
        else:
            # Open binary and create new project
            if self.core:
                self._cmd(f"o {path}")
                # Run initial analysis
                self._cmd("aa")
            project_name = os.path.basename(path)
            self.current_project = Project(
                name=project_name,
                path=path,
                binary=path,
                created_at=datetime.now().isoformat()
            )

        return self.current_project

    def save_project(self, path: Optional[str] = None) -> str:
        """
        Save the current project

        Args:
            path: Optional save path (defaults to current project path)

        Returns:
            Save status message
        """
        self._ensure_project()

        save_path = path or self.current_project.path
        if not save_path.endswith('.cutter'):
            save_path += '.cutter'

        if self.core:
            self._cmd(f"Pfs {save_path}")

        self.current_project.path = save_path
        self.current_project.modified_at = datetime.now().isoformat()

        return f"Project saved to {save_path}"

    def get_functions(self, limit: int = 100, offset: int = 0) -> List[Function]:
        """
        Get list of functions in the binary

        Args:
            limit: Maximum number of functions to return
            offset: Starting offset

        Returns:
            List of Function objects
        """
        self._ensure_project()

        if self.core:
            # Get functions as JSON
            result = self._cmdj("aflj")
            if not result:
                return []

            functions = []
            for func_data in result[offset:offset + limit]:
                func = Function(
                    address=func_data.get('offset', 0),
                    name=func_data.get('name', 'unknown'),
                    size=func_data.get('size', 0)
                )

                # Get function comment if exists
                try:
                    comment = self._cmd(f"CC. @ {hex(func.address)}")
                    if comment and comment.strip():
                        func.comment = comment.strip()
                except:
                    pass

                functions.append(func)

            return functions
        else:
            # Mock mode
            return []

    def rename_function(self, address: int, new_name: str) -> bool:
        """
        Rename a function

        Args:
            address: Function address
            new_name: New function name

        Returns:
            True if successful
        """
        self._ensure_project()

        if self.core:
            self._cmd(f"afn {new_name} {hex(address)}")
            return True
        return False

    def add_comment(self, address: int, content: str, comment_type: str = "regular") -> bool:
        """
        Add a comment at an address

        Args:
            address: Address to comment
            content: Comment content
            comment_type: Type of comment (regular, code, function, variable)

        Returns:
            True if successful
        """
        self._ensure_project()

        if self.core:
            # Escape content for Cutter command
            escaped_content = content.replace('"', '\\"')
            self._cmd(f'CCu "{escaped_content}" @ {hex(address)}')
            return True
        return False

    def generate_callgraph(self, address: Optional[int] = None, depth: int = 2) -> Graph:
        """
        Generate call graph

        Args:
            address: Function address (None for global call graph)
            depth: Graph depth

        Returns:
            Graph object in DOT format
        """
        self._ensure_project()

        graph = Graph(name="CallGraph", graph_type="digraph")

        if self.core:
            if address:
                # Generate call graph for specific function
                result = self._cmd(f"agcj {hex(address)}")
            else:
                # Generate global call graph
                result = self._cmd("agcj")

            if result:
                try:
                    data = json.loads(result)
                    nodes_seen = set()

                    for node_data in data.get('nodes', []):
                        node_id = str(node_data.get('offset', ''))
                        if node_id not in nodes_seen:
                            graph.nodes.append(GraphNode(
                                id=node_id,
                                label=node_data.get('name', 'unknown'),
                                address=node_data.get('offset', 0)
                            ))
                            nodes_seen.add(node_id)

                    for edge_data in data.get('edges', []):
                        source = str(edge_data.get('from', ''))
                        target = str(edge_data.get('to', ''))
                        if source in nodes_seen and target in nodes_seen:
                            graph.edges.append(GraphEdge(
                                source=source,
                                target=target
                            ))
                except json.JSONDecodeError:
                    pass

        return graph

    def generate_cfg(self, address: int) -> Graph:
        """
        Generate control flow graph for a function

        Args:
            address: Function address

        Returns:
            Graph object in DOT format
        """
        self._ensure_project()

        graph = Graph(name=f"CFG_{hex(address)}", graph_type="digraph")

        if self.core:
            result = self._cmd(f"agfj {hex(address)}")

            if result:
                try:
                    data = json.loads(result)

                    for bb in data.get('blocks', []):
                        bb_addr = bb.get('offset', 0)
                        graph.nodes.append(GraphNode(
                            id=str(bb_addr),
                            label=f"BB @ {hex(bb_addr)}\n{bb.get('size', 0)} bytes",
                            address=bb_addr
                        ))

                        # Add edges for jumps
                        if 'jump' in bb:
                            graph.edges.append(GraphEdge(
                                source=str(bb_addr),
                                target=str(bb['jump']),
                                label="jump"
                            ))
                        if 'fail' in bb:
                            graph.edges.append(GraphEdge(
                                source=str(bb_addr),
                                target=str(bb['fail']),
                                label="fail"
                            ))
                except json.JSONDecodeError:
                    pass

        return graph

    def export_analysis(self, output_path: Optional[str] = None, format: str = "json") -> str:
        """
        Export analysis report

        Args:
            output_path: Output file path
            format: Output format (json, markdown, text)

        Returns:
            Export status or content
        """
        self._ensure_project()

        # Collect analysis data
        functions = self.get_functions(limit=1000)
        comments = []

        if self.core:
            # Get all comments
            result = self._cmdj("CCaj")
            if result:
                for c in result:
                    comments.append(Comment(
                        address=c.get('offset', 0),
                        content=c.get('text', ''),
                        comment_type=CommentType.REGULAR
                    ))

        report = AnalysisReport(
            project=self.current_project,
            functions=functions,
            comments=comments
        )

        # Generate output
        if format == "json":
            content = json.dumps(report.to_dict(), indent=2)
        elif format == "markdown":
            content = report.to_markdown()
        else:
            content = str(report.to_dict())

        # Save to file if path provided
        if output_path:
            with open(output_path, 'w', encoding='utf-8') as f:
                f.write(content)
            return f"Analysis exported to {output_path}"
        else:
            return content

    def get_function_info(self, address: int) -> Optional[Function]:
        """
        Get detailed information about a function

        Args:
            address: Function address

        Returns:
            Function object or None
        """
        self._ensure_project()

        if self.core:
            result = self._cmdj(f"afij @ {hex(address)}")
            if result and len(result) > 0:
                func_data = result[0]
                return Function(
                    address=func_data.get('offset', address),
                    name=func_data.get('name', 'unknown'),
                    size=func_data.get('size', 0),
                    xrefs_from=func_data.get('callin', []),
                    xrefs_to=func_data.get('callout', [])
                )
        return None

    def search_functions(self, pattern: str) -> List[Function]:
        """
        Search functions by name pattern

        Args:
            pattern: Search pattern (supports wildcards)

        Returns:
            List of matching functions
        """
        self._ensure_project()

        if self.core:
            result = self._cmdj(f"aflj")
            if result:
                matching = []
                for func_data in result:
                    name = func_data.get('name', '')
                    if pattern.lower() in name.lower():
                        matching.append(Function(
                            address=func_data.get('offset', 0),
                            name=name,
                            size=func_data.get('size', 0)
                        ))
                return matching
        return []

    def get_strings(self, limit: int = 100) -> List[str]:
        """
        Get strings found in the binary

        Args:
            limit: Maximum number of strings

        Returns:
            List of strings
        """
        self._ensure_project()

        if self.core:
            result = self._cmdj("izzj")
            if result:
                return [s.get('string', '') for s in result[:limit]]
        return []

    def get_imports(self) -> List[str]:
        """
        Get imported functions

        Returns:
            List of import names
        """
        self._ensure_project()

        if self.core:
            result = self._cmdj("iij")
            if result:
                return [imp.get('name', '') for imp in result]
        return []

    def get_exports(self) -> List[str]:
        """
        Get exported functions/symbols

        Returns:
            List of export names
        """
        self._ensure_project()

        if self.core:
            result = self._cmdj("iEj")
            if result:
                return [exp.get('name', '') for exp in result]
        return []
