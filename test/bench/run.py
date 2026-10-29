import math
import os
import shlex
import signal
import subprocess
import sys
import time

base: str = os.path.dirname(os.path.abspath(__file__))
root: str = os.path.abspath(os.path.join(base, "..", ".."))
minivm: list[str] = [os.path.join(root, "build", "bin", "minivm")]

reps: int = int(os.environ.get("BENCH_REPS", "3"))
timeout_s: float = float(os.environ.get("BENCH_TIMEOUT", "300"))

rss_divisor: int
if sys.platform == "darwin":
    rss_divisor = 1024
else:
    rss_divisor = 1


time_wrapper: str = "/usr/bin/time"
use_time_wrapper: bool = sys.platform == "linux" and os.access(time_wrapper, os.X_OK)
rss_marker: str = "MXBENCHRSS"


valgrind: str = "/usr/bin/valgrind"
count_marker: str = "I refs:"
is_counting: bool = False


def count_once(cwd: str, name: str, binary: list[str]) -> tuple[float | None, str | None]:
    devnull: int = os.open(os.devnull, os.O_RDWR)
    report_r, report_w = os.pipe()
    argv: list[str] = [
        valgrind,
        "--tool=cachegrind",
        "--cache-sim=no",
        "--cachegrind-out-file=/dev/null",
        *binary,
        name,
    ]
    prev: str = os.getcwd()
    os.chdir(cwd)
    try:
        child: int = os.posix_spawnp(
            argv[0],
            argv,
            dict(os.environ),
            file_actions=[
                (os.POSIX_SPAWN_DUP2, devnull, 0),
                (os.POSIX_SPAWN_DUP2, devnull, 1),
                (os.POSIX_SPAWN_DUP2, report_w, 2),
            ],
        )
    finally:
        os.chdir(prev)
        os.close(devnull)
        os.close(report_w)
    chunks: list[bytes] = []
    while True:
        data: bytes = os.read(report_r, 65536)
        if len(data) == 0:
            break
        chunks.append(data)
    os.close(report_r)
    _, wait_status = os.waitpid(child, 0)
    exit_code: int = os.waitstatus_to_exitcode(wait_status)
    if exit_code != 0:
        return None, f"exit {exit_code}"
    for line in b"".join(chunks).decode(errors="replace").splitlines():
        if count_marker in line:
            digits: str = line.split(count_marker)[1].strip().replace(",", "")
            return float(digits) / 1e9, None
    return None, "no instruction count"


def capture(cwd: str, name: str, binary: list[str]) -> bytes:
    proc = subprocess.run([*binary, name], cwd=cwd, capture_output=True, timeout=timeout_s)
    return proc.stdout + proc.stderr + f"exit {proc.returncode}".encode()


def check(jobs: list[tuple[str, str]], versus: list[str]) -> None:
    failed: list[str] = []
    for cwd, name in jobs:
        short: str = name[:-4]
        try:
            ours: bytes = capture(cwd, name, minivm)
            theirs: bytes = capture(cwd, name, versus)
        except subprocess.TimeoutExpired:
            print(short, "TIMEOUT")
            failed.append(short)
            continue
        if ours == theirs:
            print(short, "same")
        else:
            print(short, "DIFFERENT")
            print("  ours:  ", ours[:200])
            print("  theirs:", theirs[:200])
            failed.append(short)
    if failed:
        print(f"DIFFERENT: {' '.join(failed)}")
        sys.exit(1)


def run_once(cwd: str, name: str, binary: list[str]) -> tuple[float | None, int, str | None]:
    if is_counting:
        _, rss, fail = run_native(cwd, name, binary)
        if fail is not None:
            return None, rss, fail
        counted, count_fail = count_once(cwd, name, binary)
        return counted, rss, count_fail
    else:
        return run_native(cwd, name, binary)


def run_native(cwd: str, name: str, binary: list[str]) -> tuple[float | None, int, str | None]:
    devnull: int = os.open(os.devnull, os.O_RDWR)
    argv: list[str]
    report_r: int = -1
    file_actions: list[tuple[int, int, int]]
    if use_time_wrapper:
        report_r, report_w = os.pipe()
        argv = [time_wrapper, "-f", f"{rss_marker} %M", *binary, name]
        file_actions = [
            (os.POSIX_SPAWN_DUP2, devnull, 0),
            (os.POSIX_SPAWN_DUP2, devnull, 1),
            (os.POSIX_SPAWN_DUP2, report_w, 2),
        ]
    else:
        report_w = -1
        argv = [*binary, name]
        file_actions = [
            (os.POSIX_SPAWN_DUP2, devnull, 0),
            (os.POSIX_SPAWN_DUP2, devnull, 1),
            (os.POSIX_SPAWN_DUP2, devnull, 2),
        ]
    prev: str = os.getcwd()
    os.chdir(cwd)
    try:
        child: int = os.posix_spawnp(
            argv[0],
            argv,
            dict(os.environ),
            file_actions=file_actions,
            setpgroup=0,
        )
    finally:
        os.chdir(prev)
        os.close(devnull)
        if report_w >= 0:
            os.close(report_w)
    start: float = time.monotonic()
    deadline: float = start + timeout_s
    pid, wait_status, ru = os.wait4(child, os.WNOHANG)
    while pid == 0 and time.monotonic() <= deadline:
        time.sleep(0.005)
        pid, wait_status, ru = os.wait4(child, os.WNOHANG)
    if pid == 0:
        os.killpg(child, signal.SIGKILL)
        os.wait4(child, 0)
        if report_r >= 0:
            os.close(report_r)
        return None, 0, "timeout"
    elapsed: float = time.monotonic() - start
    exit_code: int = os.waitstatus_to_exitcode(wait_status)
    maxrss: int = ru.ru_maxrss // rss_divisor
    if report_r >= 0:
        chunks: list[bytes] = []
        while True:
            data: bytes = os.read(report_r, 65536)
            if len(data) == 0:
                break
            chunks.append(data)
        os.close(report_r)
        for line in b"".join(chunks).decode(errors="replace").splitlines():
            if line.startswith(rss_marker):
                maxrss = int(line.split()[1])
    if exit_code != 0:
        return None, maxrss, f"exit {exit_code}"
    else:
        return elapsed, maxrss, None


class Tally:
    def __init__(self) -> None:
        self.best: float | None = None
        self.rss: int = 0
        self.fail: str | None = None

    def add(self, elapsed: float | None, maxrss: int, fail: str | None) -> None:
        if fail is not None:
            self.fail = fail
        else:
            if elapsed is not None and (self.best is None or elapsed < self.best):
                self.best = elapsed
            if maxrss > self.rss:
                self.rss = maxrss


def bench_interleaved(cwd: str, name: str, binaries: list[list[str]]) -> list[Tally]:
    tallies: list[Tally] = [Tally() for _ in binaries]
    for _ in range(reps):
        for binary, tally in zip(binaries, tallies):
            if tally.fail is None:
                tally.add(*run_once(cwd, name, binary))
    return tallies


def geomean(ratios: list[float]) -> float | None:
    if not ratios:
        return None
    else:
        return math.exp(sum(math.log(r) for r in ratios) / len(ratios))


def ratio_text(ratio: float | None) -> str:
    if ratio is None:
        return "-"
    else:
        return f"{ratio:.2f}x"


def main() -> None:
    global is_counting, reps
    argv: list[str] = sys.argv[1:]
    unit: str = "s"
    if "--count" in argv:
        argv.remove("--count")
        is_counting = True
        reps = 1
        unit = "G"
    is_checking: bool = "--check" in argv
    if is_checking:
        argv.remove("--check")
    versus: list[str] | None = None
    if "--vs" in argv:
        i: int = argv.index("--vs")
        if i + 1 >= len(argv):
            print("--vs requires an interpreter argument")
            sys.exit(2)
        versus = shlex.split(argv[i + 1])
        if os.path.sep in versus[0]:
            versus[0] = os.path.abspath(versus[0])
        del argv[i : i + 2]
    picks: set[str] = set()
    for a in argv:
        stem: str = os.path.basename(a)
        if stem.endswith(".lua"):
            stem = stem[:-4]
        picks.add(stem)

    jobs: list[tuple[str, str]] = []
    for name in sorted(os.listdir(base)):
        if name.endswith(".lua"):
            jobs.append((base, name))
    real: str = os.path.join(base, "real")
    if os.path.isdir(real):
        for name in sorted(os.listdir(real)):
            if name.startswith("bench_") and name.endswith(".lua"):
                jobs.append((real, name))
    if picks:
        jobs = [(d, n) for (d, n) in jobs if n[:-4] in picks]

    if is_checking:
        if versus is None:
            print("--check requires --vs")
            sys.exit(2)
        check(jobs, versus)
        return

    binaries: list[list[str]] = [minivm]
    header: list[str] = ["bench", "time", "rss"]
    if versus is not None:
        binaries.append(versus)
        ref_name: str = "".join([os.path.basename(versus[0]), *versus[1:]])
        header.append(f"{ref_name}-time")
        header.append(f"{ref_name}-rss")
        header.append("time-ratio")
        header.append("rss-ratio")
    print(" ".join(header))

    failed: list[str] = []
    time_ratios: dict[str, list[float]] = {"all": [], "micro": [], "real": []}
    rss_ratios: dict[str, list[float]] = {"all": [], "micro": [], "real": []}
    for cwd, name in jobs:
        group: str
        if cwd == real:
            group = "real"
        else:
            group = "micro"
        short: str = name[:-4]
        tallies: list[Tally] = bench_interleaved(cwd, name, binaries)
        us: Tally = tallies[0]
        if us.fail is not None or us.best is None:
            print(short, "FAIL", us.fail)
            failed.append(short)
        else:
            row: list[str] = [short, f"{us.best:.3f}{unit}", f"{us.rss / 1024:.1f}MB"]
            if versus is not None:
                ref: Tally = tallies[1]
                if ref.fail is not None or ref.best is None:
                    row.append("FAIL")
                    row.append(str(ref.fail))
                    row.append("-")
                    row.append("-")
                else:
                    time_ratio: float | None = None
                    rss_ratio: float | None = None
                    if ref.best > 0:
                        time_ratio = us.best / ref.best
                        time_ratios["all"].append(time_ratio)
                        time_ratios[group].append(time_ratio)
                    if ref.rss > 0 and us.rss > 0:
                        rss_ratio = us.rss / ref.rss
                        rss_ratios["all"].append(rss_ratio)
                        rss_ratios[group].append(rss_ratio)
                    row.append(f"{ref.best:.3f}{unit}")
                    row.append(f"{ref.rss / 1024:.1f}MB")
                    row.append(ratio_text(time_ratio))
                    row.append(ratio_text(rss_ratio))
            print(" ".join(row))

    if versus is not None:
        print("geomean", ratio_text(geomean(time_ratios["all"])), ratio_text(geomean(rss_ratios["all"])))
        for group in ("micro", "real"):
            print(
                f"geomean-{group}",
                ratio_text(geomean(time_ratios[group])),
                ratio_text(geomean(rss_ratios[group])),
                f"({len(time_ratios[group])} benchmarks)",
            )

    if failed:
        print(f"FAILED: {' '.join(failed)}")
        sys.exit(1)


if __name__ == "__main__":
    main()
