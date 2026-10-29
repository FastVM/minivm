import os
import re
import subprocess
import sys

base: str = os.path.dirname(os.path.abspath(__file__))
root: str = os.path.abspath(os.path.join(base, "..", ".."))
minivm: str = os.path.join(root, "build", "bin", "minivm")

usage: str = "usage: profile.py [--top N] [--only FUNCTION] [--lines FUNCTION] [--calls] [--callers FUNCTION] [--inclusive] [--cwd DIR] [--exe INTERPRETER] [--event EVENT] -- ARGS..."

cost_line: re.Pattern[str] = re.compile(r"^\d+( \d+)+$")


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
    callers_of: str | None = None
    i: int = 0
    while i < len(opts):
        match opts[i]:
            case "--calls":
                is_calls = True
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
        f"--dump-line={'yes' if lines is not None else 'no'}",
        "--dump-instr=no",
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
