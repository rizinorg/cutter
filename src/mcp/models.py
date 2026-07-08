"""
Data models for Cutter MCP Server
"""

from dataclasses import dataclass, field
from typing import List, Optional
from enum import Enum


class CommentType(Enum):
    """Comment types supported by Cutter"""
    REGULAR = "regular"
    CODE = "code"
    FUNCTION = "function"
    VARIABLE = "variable"


@dataclass
class Project:
    """Project information"""
    name: str
    path: str
    binary: Optional[str] = None
    author: Optional[str] = None
    description: Optional[str] = None
    created_at: Optional[str] = None
    modified_at: Optional[str] = None

    def to_dict(self) -> dict:
        """Convert to dictionary"""
        return {
            "name": self.name,
            "path": self.path,
            "binary": self.binary,
            "author": self.author,
            "description": self.description,
            "created_at": self.created_at,
            "modified_at": self.modified_at
        }


@dataclass
class Function:
    """Function information"""
    address: int
    name: str
    size: int = 0
    comment: Optional[str] = None
    xrefs_from: List[int] = field(default_factory=list)
    xrefs_to: List[int] = field(default_factory=list)
    variables: List[str] = field(default_factory=list)

    def to_dict(self) -> dict:
        """Convert to dictionary"""
        return {
            "address": hex(self.address),
            "name": self.name,
            "size": self.size,
            "comment": self.comment,
            "xrefs_from": [hex(x) for x in self.xrefs_from],
            "xrefs_to": [hex(x) for x in self.xrefs_to],
            "variables": self.variables
        }


@dataclass
class GraphNode:
    """Graph node"""
    id: str
    label: str
    address: Optional[int] = None
    attributes: dict = field(default_factory=dict)


@dataclass
class GraphEdge:
    """Graph edge"""
    source: str
    target: str
    label: Optional[str] = None
    attributes: dict = field(default_factory=dict)


@dataclass
class Graph:
    """Graph data structure"""
    nodes: List[GraphNode] = field(default_factory=list)
    edges: List[GraphEdge] = field(default_factory=list)
    graph_type: str = "digraph"
    name: Optional[str] = None

    def to_dot(self) -> str:
        """Convert to DOT format"""
        lines = [f"{self.graph_type} {self.name or 'G'} {{"]

        for node in self.nodes:
            attrs = [f'label="{node.label}"']
            if node.address:
                attrs.append(f'address="{hex(node.address)}"')
            for k, v in node.attributes.items():
                attrs.append(f'{k}="{v}"')
            lines.append(f'  {node.id} [{", ".join(attrs)}];')

        for edge in self.edges:
            attrs = []
            if edge.label:
                attrs.append(f'label="{edge.label}"')
            for k, v in edge.attributes.items():
                attrs.append(f'{k}="{v}"')
            attr_str = f' [{", ".join(attrs)}]' if attrs else ""
            lines.append(f'  {edge.source} -> {edge.target}{attr_str};')

        lines.append("}")
        return "\n".join(lines)

    def to_dict(self) -> dict:
        """Convert to dictionary"""
        return {
            "nodes": [
                {
                    "id": n.id,
                    "label": n.label,
                    "address": hex(n.address) if n.address else None,
                    "attributes": n.attributes
                }
                for n in self.nodes
            ],
            "edges": [
                {
                    "source": e.source,
                    "target": e.target,
                    "label": e.label,
                    "attributes": e.attributes
                }
                for e in self.edges
            ],
            "graph_type": self.graph_type,
            "name": self.name
        }


@dataclass
class Comment:
    """Comment information"""
    address: int
    content: str
    comment_type: CommentType = CommentType.REGULAR
    function_name: Optional[str] = None

    def to_dict(self) -> dict:
        """Convert to dictionary"""
        return {
            "address": hex(self.address),
            "content": self.content,
            "type": self.comment_type.value,
            "function_name": self.function_name
        }


@dataclass
class AnalysisReport:
    """Analysis report"""
    project: Project
    functions: List[Function] = field(default_factory=list)
    comments: List[Comment] = field(default_factory=list)
    strings: List[str] = field(default_factory=list)
    imports: List[str] = field(default_factory=list)
    exports: List[str] = field(default_factory=list)
    metadata: dict = field(default_factory=dict)

    def to_dict(self) -> dict:
        """Convert to dictionary"""
        return {
            "project": self.project.to_dict(),
            "functions": [f.to_dict() for f in self.functions],
            "comments": [c.to_dict() for c in self.comments],
            "strings": self.strings,
            "imports": self.imports,
            "exports": self.exports,
            "metadata": self.metadata
        }

    def to_markdown(self) -> str:
        """Convert to Markdown format"""
        lines = [
            f"# Analysis Report: {self.project.name}",
            "",
            f"**Binary**: {self.project.binary or 'N/A'}",
            f"**Path**: {self.project.path}",
            "",
            "## Summary",
            f"- Functions: {len(self.functions)}",
            f"- Comments: {len(self.comments)}",
            f"- Strings: {len(self.strings)}",
            f"- Imports: {len(self.imports)}",
            f"- Exports: {len(self.exports)}",
            "",
            "## Functions",
            ""
        ]

        for func in self.functions[:50]:  # Limit to first 50
            lines.append(f"### {func.name}")
            lines.append(f"- Address: {hex(func.address)}")
            lines.append(f"- Size: {func.size} bytes")
            if func.comment:
                lines.append(f"- Comment: {func.comment}")
            lines.append("")

        if self.comments:
            lines.extend(["## Comments", ""])
            for comment in self.comments[:20]:  # Limit to first 20
                lines.append(f"- **{hex(comment.address)}**: {comment.content}")
            lines.append("")

        if self.strings:
            lines.extend(["## Strings", ""])
            for string in self.strings[:20]:  # Limit to first 20
                lines.append(f"- {string}")
            lines.append("")

        return "\n".join(lines)
