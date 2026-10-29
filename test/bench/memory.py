import os
import re
import subprocess
import sys

base: str = os.path.dirname(os.path.abspath(__file__))
root: str = os.path.abspath(os.path.join(base, "..", ".."))
minivm: str = os.path.join(root, "build", "bin", "minivm")

usage: str = "usage: memory.py [--depth N] [--min PERCENT] [--cwd DIR] [--exe INTERPRETER] -- ARGS..."

# The project's allocator wrappers. Massif charges memory to their callers, so
# the tree starts at the code that asked for the bytes.
alloc_fns: list[str] = [
    "mx_mem_alloc",
    "mx_mem_calloc",
    "mx_mem_realloc",
    "mx_arena_take",
    "mx_arena_alloc",
    "mx_arena_alloc_bytes",
    "mx_gc_pool_alloc",
    "mx_gc_pool_realloc",
    "mx_gc_pool_slab_refill",
    "mx_gc_map_aligned_block",
    "mx_gc_new_block",
]

node_line: re.Pattern[str] = re.compile(r"^( *)n\d+: (\d+) (.*)$")
frame_desc: re.Pattern[str] = re.compile(r"^0x[0-9A-Fa-f]+: (\S+)(?: \((.*)\))?$")


def describe(text: str) -> str:
    got = frame_desc.match(text)
    if got is None:
        return text
    elif got.group(2) is None:
        return got.group(1)
    else:
        return f"{got.group(1)} ({os.path.basename(got.group(2))})"


def peak_tree(text: str) -> tuple[int, int, list[tuple[int, int, str]]]:
    heap: int = 0
    extra: int = 0
    is_peak: bool = False
    nodes: list[tuple[int, int, str]] = []
    for line in text.splitlines():
        if line.startswith("snapshot="):
            if is_peak:
                break
            heap = 0
            extra = 0
        elif line.startswith("mem_heap_B="):
            heap = int(line.split("=")[1])
        elif line.startswith("mem_heap_extra_B="):
            extra = int(line.split("=")[1])
        elif line == "heap_tree=peak":
            is_peak = True
        elif is_peak and (got := node_line.match(line)) is not None:
            nodes.append((len(got.group(1)), int(got.group(2)), describe(got.group(3))))
    return heap, extra, nodes


def main() -> None:
    argv: list[str] = sys.argv[1:]
    if "--" not in argv:
        print(usage)
        sys.exit(2)
    split: int = argv.index("--")
    opts: list[str] = argv[:split]
    args: list[str] = argv[split + 1 :]
    depth: int = 3
    least: float = 1.0
    cwd: str = os.getcwd()
    exe: str = minivm
    i: int = 0
    while i < len(opts):
        match opts[i]:
            case "--depth":
                depth = int(opts[i + 1])
            case "--min":
                least = float(opts[i + 1])
            case "--cwd":
                cwd = opts[i + 1]
            case "--exe":
                exe = opts[i + 1]
                if os.path.sep in exe:
                    exe = os.path.abspath(exe)
            case other:
                print(f"unknown option {other}\n{usage}")
                sys.exit(2)
        i += 2
    command: list[str] = [
        "valgrind",
        "--tool=massif",
        "--massif-out-file=/dev/stderr",
        "--threshold=0.1",
        "--depth=40",
        *[f"--alloc-fn={fn}" for fn in alloc_fns],
        exe,
        *args,
    ]
    proc = subprocess.run(command, cwd=cwd, capture_output=True, text=True, errors="replace")
    heap, extra, nodes = peak_tree(proc.stderr)
    if not nodes:
        print("no peak snapshot in massif output")
        sys.exit(1)
    print(f"peak heap {heap:,} B, malloc overhead {extra:,} B")
    for level, size, desc in nodes:
        share: float = 100.0 * size / heap if heap else 0.0
        if 1 <= level <= depth and share >= least:
            print(f"{share:6.2f}% {size:>12,} {'  ' * (level - 1)}{desc}")


if __name__ == "__main__":
    main()
