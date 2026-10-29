import argparse
import dataclasses
import os
import resource
import shlex
import subprocess
import sys
import time

base: str = os.path.dirname(os.path.abspath(__file__))
root: str = os.path.abspath(os.path.join(base, "..", ".."))
minivm: list[str] = [os.path.join(root, "build", "bin", "minivm")]
skipped: set[str] = {"all.lua", "bwcoercion.lua", "tracegc.lua", "heavy.lua"}


@dataclasses.dataclass
class Result:
    status: str
    seconds: float
    message: str


def limit_stack() -> None:
    hard: int = resource.getrlimit(resource.RLIMIT_STACK)[1]
    resource.setrlimit(resource.RLIMIT_STACK, (1100 * 1024, hard))


def run(interp: list[str], cwd: str, name: str, timeout: float) -> Result:
    start: float = time.monotonic()
    try:
        proc = subprocess.run(
            [*interp, "-W", name],
            cwd=cwd,
            stdin=subprocess.DEVNULL,
            capture_output=True,
            timeout=timeout,
            preexec_fn=limit_stack,
        )
    except subprocess.TimeoutExpired:
        return Result("timeout", time.monotonic() - start, "")
    seconds: float = time.monotonic() - start
    if proc.returncode == 0:
        return Result("ok", seconds, "")
    else:
        lines: list[str] = [l for l in proc.stderr.decode(errors="replace").splitlines() if l.strip()]
        chunk: list[str] = [l.strip() for l in lines if l.strip().endswith("in main chunk")]
        message: str
        if lines and chunk:
            message = f"{lines[0][:160]}  [{chunk[0].split(':')[1]}]"
        elif lines:
            message = lines[0][:160]
        else:
            message = ""
        return Result(f"exit {proc.returncode}", seconds, message)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--dir", default=os.path.join(root, "lua", "testes"))
    ap.add_argument("--vs", default=None)
    ap.add_argument("--timeout", type=float, default=60)
    ap.add_argument("files", nargs="*")
    args = ap.parse_args()
    cwd: str = os.path.abspath(args.dir)
    names: list[str]
    if args.files:
        names = [f if f.endswith(".lua") else f + ".lua" for f in args.files]
    else:
        names = sorted(f for f in os.listdir(cwd) if f.endswith(".lua") and f not in skipped)
    versus: list[str] | None = None
    if args.vs is not None:
        versus = shlex.split(args.vs)
        if os.path.sep in versus[0]:
            versus[0] = os.path.abspath(versus[0])
    ours_ok: int = 0
    theirs_ok: int = 0
    both_ok: int = 0
    for name in names:
        ours: Result = run(minivm, cwd, name, args.timeout)
        row: str = f"{name[:-4]:<12} {ours.status:<8} {ours.seconds:7.2f}s"
        if ours.status == "ok":
            ours_ok += 1
        if versus is not None:
            theirs: Result = run(versus, cwd, name, args.timeout)
            row += f"  ref {theirs.status:<8} {theirs.seconds:7.2f}s"
            if theirs.status == "ok":
                theirs_ok += 1
                if ours.status == "ok":
                    both_ok += 1
        print(row)
        if ours.message:
            print(f"    {ours.message}")
    summary: str = f"minivm ok {ours_ok}/{len(names)}"
    if versus is not None:
        summary += f", ref ok {theirs_ok}/{len(names)}, both ok {both_ok}"
    print(summary)
    if ours_ok != len(names):
        sys.exit(1)


if __name__ == "__main__":
    main()
