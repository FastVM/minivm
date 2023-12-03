
import matplotlib.pyplot as plt 

with open('build/bench/all.txt') as f:
    src = f.read()

maps = {}

m1 = (0.10,0.12,0.10,0.11,0.14,0.10)

for line in src.split('\n'):
    if line.strip() == '':
        break
    (bench, engine, time) = (ent.strip() for ent in line.split(':'))
    if bench not in maps:
        maps[bench] = {}
    maps[bench][engine] = float(time)

for key in maps:
    pair = maps[key]
    if 'minivm' in pair and 'luajit' in pair and 'lua' in pair:
        name = key.replace('test/', '').replace('.lua', '')
        fig, ax = plt.subplots()

        fig.suptitle(f'MicroBench - {name}')

        data = (pair['luajit'], pair['minivm'], pair['lua'])

        ax.bar(('luajit', 'minivm', 'lua'), data, color=('#97a7d7', '#75819F', '#000080'))

        fig.savefig(f'build/bench/png/{name.replace("/", "-")}.png')

