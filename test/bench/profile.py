import os
import re
import subprocess
import sys

base: str = os.path.dirname(os.path.abspath(__file__))
root: str = os.path.abspath(os.path.join(base, "..", ".."))
minivm: str = os.path.join(root, "build", "bin", "minivm")
run_source: str = os.path.join(root, "src", "run", "run.c")
run_fn: str = "mx_run_body"

usage: str = "usage: profile.py [--top N] [--only FUNCTION] [--lines FUNCTION] [--hops] [--calls] [--callers FUNCTION] [--inclusive] [--cwd DIR] [--exe INTERPRETER] [--event EVENT] -- ARGS..."

cost_line: re.Pattern[str] = re.compile(r"^\d+( \d+)+$")
hop_label: re.Pattern[str] = re.compile(r"^(L_\w+|[a-z_]+):")
obj_symbol: re.Pattern[str] = re.compile(r"^([0-9a-f]+) <(.+)>:$")
obj_place: re.Pattern[str] = re.compile(r"^(?:inlined by )?(\S+):(\d+)")
obj_insn: re.Pattern[str] = re.compile(r"^\s+([0-9a-f]+):\t")


def event_cost(fields: list[str], column: int) -> int:
    if column < len(fields):
        return int(fields[column])
    else:
        return 0


# With is_inclusive, the cost of each call is charged to the calling function
# or line as well. Recursive functions (mx_run_body re-entered through C) count
# their nested calls again, so their totals can pass 100%.
def collect(text: str, event: str, target: str | None, is_inclusive: bool) -> tuple[dict[str, int], int]:
    costs: dict[str, int] = {}
    total: int = 0
    column: int = 0
    current: str | None = None
    file: str = "?"
    after_call: bool = False
    for line in text.splitlines():
        if line.startswith("events:"):
            column = line.split()[1:].index(event)
        elif line.startswith("fl="):
            file = os.path.basename(line[3:])
        elif line.startswith("fi=") or line.startswith("fe="):
            file = os.path.basename(line[3:])
        elif line.startswith("fn="):
            current = re.sub(r"'\d+$", "", line[3:])
            after_call = False
        elif line.startswith("calls="):
            after_call = True
        elif line.startswith("summary:"):
            total = event_cost(line.split()[1:], column)
        elif current is not None and cost_line.match(line):
            if after_call and not is_inclusive:
                after_call = False
                continue
            after_call = False
            if target is None:
                costs[current] = costs.get(current, 0) + event_cost(line.split()[1:], column)
            elif current == target:
                fields: list[str] = line.split()
                key: str = f"{file}:{fields[0]}"
                costs[key] = costs.get(key, 0) + event_cost(fields[1:], column)
    return costs, total


def collect_addrs(text: str, event: str, target: str) -> tuple[dict[int, int], int]:
    costs: dict[int, int] = {}
    total: int = 0
    column: int = 0
    current: str | None = None
    after_call: bool = False
    for line in text.splitlines():
        if line.startswith("events:"):
            column = line.split()[1:].index(event)
        elif line.startswith("fn="):
            current = re.sub(r"'\d+$", "", line[3:])
            after_call = False
        elif line.startswith("calls="):
            after_call = True
        elif line.startswith("summary:"):
            total = event_cost(line.split()[1:], column)
        elif line.startswith("0x"):
            if current == target and not after_call:
                fields: list[str] = line.split()
                addr: int = int(fields[0], 16)
                costs[addr] = costs.get(addr, 0) + event_cost(fields[2:], column)
            after_call = False
    return costs, total


# Each instruction is charged to the label in run.c that encloses its
# outermost source line, so code inlined from headers counts toward the hop
# that uses it.
def hop_of_lines() -> list[str]:
    labels: list[str] = ["entry"]
    current: str = "entry"
    with open(run_source) as f:
        for line in f:
            got = hop_label.match(line)
            if got is not None:
                current = got.group(1)
            labels.append(current)
    return labels


def place_of_addrs(exe: str) -> tuple[dict[int, tuple[str, int]], int]:
    proc = subprocess.run(
        ["objdump", "-d", "-l", "--inlines", "--no-show-raw-insn", f"--disassemble={run_fn}", exe],
        capture_output=True,
        text=True,
    )
    places: dict[int, tuple[str, int]] = {}
    start: int = 0
    place: tuple[str, int] = ("?", 0)
    for line in proc.stdout.splitlines():
        if (sym := obj_symbol.match(line)) is not None:
            if sym.group(2) == run_fn:
                start = int(sym.group(1), 16)
        elif (got := obj_place.match(line)) is not None:
            place = (os.path.basename(got.group(1)), int(got.group(2)))
        elif (insn := obj_insn.match(line)) is not None:
            places[int(insn.group(1), 16)] = place
    return places, start


def collect_hops(text: str, event: str, exe: str) -> tuple[dict[str, int], int, int]:
    costs, total = collect_addrs(text, event, run_fn)
    places, start = place_of_addrs(exe)
    labels: list[str] = hop_of_lines()
    hops: dict[str, int] = {}
    body: int = 0
    if not costs:
        return hops, total, body
    # Valgrind loads a PIE binary at its own base; the function entry is the
    # lowest address that runs, so it fixes the offset.
    shift: int = min(costs) - start
    for addr, cost in costs.items():
        file, at = places.get(addr - shift, ("?", 0))
        hop: str
        if file == os.path.basename(run_source) and at < len(labels):
            hop = labels[at]
        else:
            hop = f"{file}:{at}"
        hops[hop] = hops.get(hop, 0) + cost
        body += cost
    return hops, total, body


def collect_calls(text: str) -> dict[str, int]:
    calls: dict[str, int] = {}
    callee: str | None = None
    for line in text.splitlines():
        if line.startswith("cfn="):
            callee = re.sub(r"'\d+$", "", line[4:])
        elif line.startswith("calls=") and callee is not None:
            calls[callee] = calls.get(callee, 0) + int(line[6:].split()[0])
    return calls


def collect_callers(text: str, event: str, target: str) -> dict[str, tuple[int, int]]:
    callers: dict[str, tuple[int, int]] = {}
    column: int = 0
    current: str | None = None
    callee: str | None = None
    count: int = 0
    after_call: bool = False
    for line in text.splitlines():
        if line.startswith("events:"):
            column = line.split()[1:].index(event)
        elif line.startswith("fn="):
            current = re.sub(r"'\d+$", "", line[3:])
        elif line.startswith("cfn="):
            callee = re.sub(r"'\d+$", "", line[4:])
        elif line.startswith("calls="):
            count = int(line[6:].split()[0])
            after_call = True
        elif after_call and cost_line.match(line):
            after_call = False
            if callee == target and current is not None:
                calls, cost = callers.get(current, (0, 0))
                callers[current] = (calls + count, cost + event_cost(line.split()[1:], column))
    return callers


def main() -> None:
    argv: list[str] = sys.argv[1:]
    if "--" not in argv:
        print(usage)
        sys.exit(2)
    split: int = argv.index("--")
    opts: list[str] = argv[:split]
    args: list[str] = argv[split + 1 :]
    top: int = 30
    only: str | None = None
    lines: str | None = None
    cwd: str = os.getcwd()
    exe: str = minivm
    event: str = "Ir"
    is_calls: bool = False
    is_inclusive: bool = False
    is_hops: bool = False
    callers_of: str | None = None
    i: int = 0
    while i < len(opts):
        match opts[i]:
            case "--calls":
                is_calls = True
                i += 1
                continue
            case "--hops":
                is_hops = True
                i += 1
                continue
            case "--inclusive":
                is_inclusive = True
                i += 1
                continue
            case "--top":
                top = int(opts[i + 1])
            case "--only":
                only = opts[i + 1]
            case "--lines":
                lines = opts[i + 1]
            case "--callers":
                callers_of = opts[i + 1]
            case "--cwd":
                cwd = opts[i + 1]
            case "--exe":
                exe = opts[i + 1]
                if os.path.sep in exe:
                    exe = os.path.abspath(exe)
            case "--event":
                event = opts[i + 1]
            case other:
                print(f"unknown option {other}\n{usage}")
                sys.exit(2)
        i += 2
    command: list[str] = [
        "valgrind",
        "--tool=callgrind",
        "--callgrind-out-file=/dev/stderr",
        f"--dump-line={'yes' if lines is not None or is_hops else 'no'}",
        f"--dump-instr={'yes' if is_hops else 'no'}",
        "--compress-strings=no",
        "--compress-pos=no",
    ]
    if event != "Ir":
        command.append("--cache-sim=yes")
        command.append("--branch-sim=yes")
    if only is not None:
        command.append(f"--toggle-collect={only}")
    command.append(exe)
    command.extend(args)
    proc = subprocess.run(command, cwd=cwd, capture_output=True, text=True, errors="replace")
    if callers_of is not None:
        ranked_callers: list[tuple[str, tuple[int, int]]] = sorted(collect_callers(proc.stderr, event, callers_of).items(), key=lambda kv: kv[1][1], reverse=True)
        for name, (count, cost) in ranked_callers[:top]:
            print(f"{count:>12,} {cost:>16,} {name}")
        return
    if is_hops:
        hops, total, body = collect_hops(proc.stderr, event, exe)
        print(f"total {event} {total:,}, {run_fn} {body:,}")
        ranked_hops: list[tuple[str, int]] = sorted(hops.items(), key=lambda kv: kv[1], reverse=True)
        for name, cost in ranked_hops[:top]:
            share: float = 100.0 * cost / total if total else 0.0
            print(f"{share:6.2f}% {cost:>14,} {name}")
        return
    if is_calls:
        ranked_calls: list[tuple[str, int]] = sorted(collect_calls(proc.stderr).items(), key=lambda kv: kv[1], reverse=True)
        for name, count in ranked_calls[:top]:
            print(f"{count:>12,} {name}")
        return
    costs, total = collect(proc.stderr, event, lines, is_inclusive)
    print(f"total {event} {total:,}")
    ranked: list[tuple[str, int]] = sorted(costs.items(), key=lambda kv: kv[1], reverse=True)
    for name, cost in ranked[:top]:
        share: float = 100.0 * cost / total if total else 0.0
        print(f"{share:6.2f}% {cost:>14,} {name}")


if __name__ == "__main__":
    main()
