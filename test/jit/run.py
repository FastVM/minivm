import os
import re
import subprocess
import sys

base: str = os.path.dirname(os.path.abspath(__file__))
root: str = os.path.abspath(os.path.join(base, "..", ".."))
out_dir: str = os.path.join(base, "out")
minivm: str = os.path.join(root, "build", "bin", "minivm")
thresholds: list[str] = ["0", "1", "2", "5"]
reals: list[str] = ["etlua", "fennel", "inspect", "json", "lulpeg", "serpent", "tl"]
timeout_s: float = float(os.environ.get("JIT_TEST_TIMEOUT", "300"))


def normalize(text: str) -> str:
    return re.sub(r"\s+", " ", text.split("#")[0]).strip().replace(", ", ",")


def check_encoder() -> list[str]:
    os.makedirs(out_dir, exist_ok=True)
    host: str = os.path.join(out_dir, "encode")
    subprocess.run(
        ["cc", "-O1", "-o", host, os.path.join(base, "encode.c"), os.path.join(root, "src", "jit", "x64.c")],
        check=True,
    )
    table: str = subprocess.run([host], check=True, capture_output=True, text=True).stdout
    failures: list[str] = []
    forms: list[tuple[str, bytes]] = []
    for line in table.splitlines():
        text, code = line.split("\t")
        forms.append((text, bytes.fromhex(code)))
    blob: str = os.path.join(out_dir, "encode.bin")
    with open(blob, "wb") as f:
        for _, code in forms:
            f.write(code)
    listing: str = subprocess.run(
        ["objdump", "-D", "-b", "binary", "-mi386:x86-64", "-M", "intel", blob],
        check=True,
        capture_output=True,
        text=True,
    ).stdout
    decoded: dict[int, str] = {}
    for line in listing.splitlines():
        got = re.match(r"^\s*([0-9a-f]+):\t(?:[0-9a-f]{2} )+\s*\t?(.*)$", line)
        if got is not None and got.group(2).strip() != "":
            decoded[int(got.group(1), 16)] = got.group(2)
    at: int = 0
    for text, code in forms:
        want: str = normalize(text)
        have: str = normalize(decoded.get(at, "<nothing>"))
        relative = re.match(r"^(j\w+) \+0x([0-9a-f]+)$", want)
        if relative is not None:
            want = f"{relative.group(1)} {at + int(relative.group(2), 16):#x}"
        if want != have:
            failures.append(f"encode: {text!r} emitted {code.hex()} which disassembles as {have!r}")
        at += len(code)
    print(f"encoder: {len(forms)} forms, {len(failures)} wrong")
    return failures


def run_probe(cwd: str, name: str, env_extra: dict[str, str]) -> bytes:
    env: dict[str, str] = dict(os.environ)
    env.update(env_extra)
    proc = subprocess.run([minivm, name], cwd=cwd, env=env, capture_output=True, timeout=timeout_s)
    return proc.stdout + proc.stderr + f"\nexit {proc.returncode}".encode()


def show_diff(ours: bytes, theirs: bytes) -> str:
    a: list[str] = ours.decode(errors="replace").splitlines()
    b: list[str] = theirs.decode(errors="replace").splitlines()
    for i in range(max(len(a), len(b))):
        left: str = a[i] if i < len(a) else "<end>"
        right: str = b[i] if i < len(b) else "<end>"
        if left != right:
            return f"line {i + 1}: jit {left!r} interp {right!r}"
    return "identical lines"


def check_differential(cwd: str, name: str, expected: bytes | None) -> list[str]:
    failures: list[str] = []
    interp: bytes = run_probe(cwd, name, {"MX_JIT": "0"})
    if expected is not None and interp != expected:
        failures.append(f"{name}: interpreter differs from reference: {show_diff(interp, expected)}")
    for threshold in thresholds:
        got: bytes = run_probe(cwd, name, {"MX_JIT_THRESHOLD": threshold})
        if got != interp:
            failures.append(f"{name} threshold={threshold}: {show_diff(got, interp)}")
    return failures


def reference_output(lua: str, cwd: str, name: str) -> bytes:
    proc = subprocess.run([lua, name], cwd=cwd, capture_output=True, timeout=timeout_s)
    return proc.stdout + proc.stderr + f"\nexit {proc.returncode}".encode()


def main() -> None:
    argv: list[str] = sys.argv[1:]
    lua: str | None = None
    if "--lua" in argv:
        at: int = argv.index("--lua")
        lua = argv[at + 1]
        del argv[at : at + 2]
    is_real: bool = "--no-real" not in argv
    failures: list[str] = check_encoder()
    probes_dir: str = os.path.join(base, "probes")
    names: list[str] = sorted(n for n in os.listdir(probes_dir) if n.endswith(".lua"))
    for name in names:
        expected: bytes | None = None
        if lua is not None:
            expected = reference_output(lua, probes_dir, name)
        found: list[str] = check_differential(probes_dir, name, expected)
        print(f"probe {name}: {'ok' if not found else 'FAIL'}", flush=True)
        failures.extend(found)
    real_dir: str = os.path.join(root, "test", "bench", "real")
    for real in reals:
        if is_real:
            name = f"bench_{real}.lua"
            found = check_differential(real_dir, name, None)
            print(f"real {name}: {'ok' if not found else 'FAIL'}", flush=True)
            failures.extend(found)
    if failures:
        for failure in failures:
            print(failure)
        print(f"FAILED: {len(failures)}")
        sys.exit(1)
    print("all ok")


if __name__ == "__main__":
    main()
