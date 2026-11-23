#!/usr/bin/env python3
"""
Pattern Blocks & CFG Geography Analyzer for MiniVM JIT.

Analyzes basic blocks as discrete 'compute blocks' (Lego-like typed units)
and evaluates their Control Flow Graph (CFG) geography:
- Pegs & Sockets: Register type signatures on block entry vs exit.
- Stitchability: Can adjacent blocks snap together without stub dispatch?
- Topography: Forward acyclic traces vs cyclic loop back-edges.
"""

from dataclasses import dataclass, field
from enum import Enum
from typing import Dict, List, Optional, Set, Tuple


class Kind(Enum):
    NIL = 0
    BOOLEAN = 1
    INT = 2
    FLOAT = 3
    STRING = 4
    CLOSURE = 5
    FFI = 6
    BLOCK = 7
    ERROR = 8
    USERDATA = 9
    TABLE = 10


class Topography(Enum):
    ENTRY = "entry"
    DOWNSTREAM = "downstream"   # Forward acyclic edge (id < target_id)
    CYCLIC_LOOP = "cyclic_loop" # Backward edge (id >= target_id)
    TERMINAL = "terminal"       # Return / exit block


@dataclass
class ComputeBlock:
    """A discrete basic-block compute unit ('Lego' block)."""
    block_id: int
    func_name: str
    input_pegs: Tuple[Kind, ...]       # Expected types on entry
    output_pegs: Tuple[Kind, ...]      # Produced types on exit
    targets: List[int] = field(default_factory=list)
    ninstrs: int = 0

    @property
    def packed_key(self) -> int:
        """64-bit bit-packed signature representation."""
        if len(self.input_pegs) > 6:
            return 0
        key = (len(self.input_pegs) + 1) << 56
        for i, k in enumerate(self.input_pegs):
            key |= (k.value & 0xFF) << (8 * i)
        return key

    def edge_topography(self, target_id: int) -> Topography:
        if target_id <= self.block_id:
            return Topography.CYCLIC_LOOP
        return Topography.DOWNSTREAM


@dataclass
class StitchResult:
    source_id: int
    target_id: int
    can_snap: bool
    reason: str


class BlockGeographyAnalyzer:
    """Analyzes spatial topology and stitching patterns across compute blocks."""

    def __init__(self, blocks: List[ComputeBlock]):
        self.blocks: Dict[int, ComputeBlock] = {b.block_id: b for b in blocks}

    def check_stitch(self, source_id: int, target_id: int) -> StitchResult:
        if source_id not in self.blocks or target_id not in self.blocks:
            return StitchResult(source_id, target_id, False, "block_not_found")

        src = self.blocks[source_id]
        tgt = self.blocks[target_id]

        # Rule 1: Backward edges cannot be linearly stitched (forms a loop back-edge)
        if tgt.block_id <= src.block_id:
            return StitchResult(source_id, target_id, False, "backward_edge_loop")

        # Rule 2: Peg compatibility (output types must match target input expectations)
        min_len = min(len(src.output_pegs), len(tgt.input_pegs))
        for i in range(min_len):
            if src.output_pegs[i] != tgt.input_pegs[i]:
                return StitchResult(
                    source_id,
                    target_id,
                    False,
                    f"type_mismatch: reg[{i}] produces {src.output_pegs[i].name} but target expects {tgt.input_pegs[i].name}"
                )

        return StitchResult(source_id, target_id, True, "compatible_types_and_forward_flow")

    def find_linear_superblocks(self) -> List[List[int]]:
        """Identifies straight-line superblocks where Lego pieces snap together."""
        visited: Set[int] = set()
        superblocks: List[List[int]] = []

        for b_id in sorted(self.blocks.keys()):
            if b_id in visited:
                continue

            current_chain = [b_id]
            visited.add(b_id)

            curr = self.blocks[b_id]
            while len(curr.targets) == 1:
                target_id = curr.targets[0]
                stitch = self.check_stitch(curr.block_id, target_id)
                if stitch.can_snap and target_id not in visited:
                    current_chain.append(target_id)
                    visited.add(target_id)
                    curr = self.blocks[target_id]
                else:
                    break

            if len(current_chain) > 1:
                superblocks.append(current_chain)

        return superblocks

    def report_geography(self) -> Dict[str, any]:
        total_edges = 0
        forward_edges = 0
        loop_edges = 0
        snappable_stitches = 0

        for b in self.blocks.values():
            for t_id in b.targets:
                total_edges += 1
                topo = b.edge_topography(t_id)
                if topo == Topography.CYCLIC_LOOP:
                    loop_edges += 1
                else:
                    forward_edges += 1

                if self.check_stitch(b.block_id, t_id).can_snap:
                    snappable_stitches += 1

        superblocks = self.find_linear_superblocks()
        efficiency = (snappable_stitches / total_edges * 100.0) if total_edges > 0 else 0.0

        return {
            "total_compute_blocks": len(self.blocks),
            "total_cfg_edges": total_edges,
            "forward_edges": forward_edges,
            "loop_back_edges": loop_edges,
            "snappable_stitches": snappable_stitches,
            "stitch_efficiency_pct": round(efficiency, 2),
            "contiguous_superblocks": len(superblocks),
            "longest_superblock_length": max((len(s) for s in superblocks), default=1),
        }


def self_test():
    """Validates compute block pattern analysis on a canonical loop + branch graph."""
    sample_blocks = [
        ComputeBlock(0, "main", (Kind.INT,), (Kind.INT, Kind.INT), [1], ninstrs=4),
        ComputeBlock(1, "main", (Kind.INT, Kind.INT), (Kind.INT, Kind.INT), [2, 3], ninstrs=6),
        ComputeBlock(2, "main", (Kind.INT, Kind.INT), (Kind.INT, Kind.INT), [4], ninstrs=3),
        ComputeBlock(3, "main", (Kind.INT, Kind.INT), (Kind.INT, Kind.INT), [4], ninstrs=3),
        ComputeBlock(4, "main", (Kind.INT, Kind.INT), (Kind.INT,), [1, 5], ninstrs=5), # Loop back to 1
        ComputeBlock(5, "main", (Kind.INT,), (), [], ninstrs=2),                       # Exit
    ]

    analyzer = BlockGeographyAnalyzer(sample_blocks)
    report = analyzer.report_geography()
    superblocks = analyzer.find_linear_superblocks()

    print("==================================================")
    print("  MINIVM JIT COMPUTE BLOCK GEOGRAPHY REPORT")
    print("==================================================")
    for k, v in report.items():
        print(f"  {k:28}: {v}")
    print("  Identified Superblock Chains:", superblocks)
    assert report["total_compute_blocks"] == 6
    assert report["loop_back_edges"] == 1
    assert report["snappable_stitches"] >= 3
    print("  ✓ Self-test passed cleanly.")


if __name__ == "__main__":
    self_test()
