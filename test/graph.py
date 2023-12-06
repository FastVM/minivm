import matplotlib.pyplot as plt
from sys import argv
from dataclasses import dataclass
from typing import TypeAlias, Dict, List, Tuple, Literal

@dataclass
class Engine:
    name: str
    color: str

    def __lt__(self, other):
        return self.name < other.name

    def __hash__(self):
        return hash(self.name)

    @staticmethod
    def lua() -> 'Engine':
        return Engine(name="lua", color="#000080")

    @staticmethod
    def luajit() -> 'Engine':
        return Engine(name="luajit", color="#97a7d7")

    @staticmethod
    def minivm() -> 'Engine':
        return Engine(name="minivm", color="#75819F")


Benchmark: TypeAlias = Dict[Engine, float]
Benchmarks: TypeAlias = Dict[str, Benchmark]

min_len_pair = 3

def name_to_engine(name: str) -> Engine:
    match name:
        case "lua":
            return Engine.lua()
        case "luajit":
            return Engine.luajit()
        case "minivm":
            return Engine.minivm()
        case _:
            raise Exception('supported engines are lua, luajit, and minivm')


def get_data() -> str:
    with open("build/bench/all.txt") as all_file:
        return all_file.read()


def parse_data() -> Benchmarks:
    time_string: str = get_data()
    benchmarks: Benchmarks = {}

    for line in time_string.split("\n"):
        if line.strip() == "":
            continue
        parts: Tuple[str, ...] = tuple(ent.strip() for ent in line.split(":"))
        if parts[0] not in benchmarks:
            benchmarks[parts[0]] = {}
        benchmarks[parts[0]][name_to_engine(parts[1])] = float(parts[2])

    return benchmarks

def main(argv: List[str]) -> None:
    benchmarks: Benchmarks = parse_data()

    for key in benchmarks:
        results_by_engine: Benchmark = benchmarks[key]

        if len(results_by_engine) < min_len_pair:
            continue

        # if 'minivm' in pair and 'luajit' in pair and 'lua' in pair:
        name: str = key.replace("test/", "").replace(".lua", "")
        fig, ax = plt.subplots()

        fig.suptitle(f"MicroBench - {name}")

        engines: Tuple[Engine, ...] = tuple(sorted(results_by_engine.keys()))

        keys: List[str] = [i.name for i in engines]
        data: List[float] = [results_by_engine[i] for i in engines]
        colors: List[str] = [i.color for i in engines]

        ax.bar(keys, data, color=colors)

        fig.savefig(f'build/bench/png/{name.replace("/", "-")}.png')
        plt.close(fig)


if __name__ == "__main__":
    main(argv)
