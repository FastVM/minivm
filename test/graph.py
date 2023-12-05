
import matplotlib.pyplot as plt 

with open('build/bench/all.txt') as f:
    src = f.read()

maps = {}

for line in src.split('\n'):
    if line.strip() == '':
        continue
    (bench, engine, time) = (ent.strip() for ent in line.split(':'))
    if bench not in maps:
        maps[bench] = {}
    maps[bench][engine] = float(time)

for key in maps:
    pair = maps[key]
    # if 'minivm' in pair and 'luajit' in pair and 'lua' in pair:
    name = key.replace('test/', '').replace('.lua', '')
    fig, ax = plt.subplots()

    fig.suptitle(f'MicroBench - {name}')

    keys = sorted(pair.keys())

    data = [pair[i] for i in keys]

    ax.bar(keys, data, color=('#000080', '#97a7d7', '#75819F'))

    fig.savefig(f'build/bench/png/v{len(pair)}-{name.replace("/", "-")}.png')
    plt.close(fig)
