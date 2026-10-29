import os
import re
import subprocess
import sys

base: str = os.path.dirname(os.path.abspath(__file__))
root: str = os.path.abspath(os.path.join(base, "..", ".."))
minivm: str = os.path.join(root, "build", "bin", "minivm")

usage: str = "usage: asm.py FUNCTION FILE FIRST [LAST]  (needs a -g build)"


def main() -> None:
    argv: list[str] = sys.argv[1:]
    if len(argv) < 3:
        print(usage)
        sys.exit(2)
    fn: str = argv[0]
    file: str = os.path.basename(argv[1])
    first: int = int(argv[2])
    last: int = first
    if len(argv) > 3:
        last = int(argv[3])
    proc = subprocess.run(
        ["objdump", "-d", "-l", "--no-show-raw-insn", f"--disassemble={fn}", minivm],
        capture_output=True,
        text=True,
    )
    marker = re.compile(r"^(/.*?|[^\s:]+):(\d+)( \(discriminator \d+\))?$")
    is_inside: bool = False
    for line in proc.stdout.splitlines():
        got = marker.match(line)
        if got is not None:
            at: int = int(got.group(2))
            is_inside = os.path.basename(got.group(1)) == file and first <= at <= last
            if is_inside:
                print(f"-- {file}:{at}")
        elif is_inside and line.strip() != "":
            print(line)


if __name__ == "__main__":
    main()
