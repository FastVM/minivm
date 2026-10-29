import os
import subprocess
import sys

base: str = os.path.dirname(os.path.abspath(__file__))
root: str = os.path.abspath(os.path.join(base, "..", ".."))


def main() -> None:
    out_path: str = os.path.join(root, sys.argv[1])
    proc = subprocess.run(sys.argv[2:], cwd=root, capture_output=True)
    with open(out_path, "wb") as f:
        f.write(proc.stdout)
        f.write(proc.stderr)
        f.write(f"[exit {proc.returncode}]\n".encode())
    sys.exit(proc.returncode)


if __name__ == "__main__":
    main()
