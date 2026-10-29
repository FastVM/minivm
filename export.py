
import glob

srcs = []

srcs.extend(glob.glob('src/**/*.c', recursive=True))
srcs.extend(glob.glob('src/**/*.h', recursive=True))

srcs.sort()

lines = []
for i, src in enumerate(srcs):
    if i != 0:
        lines.append('\n')
    name = src.removeprefix('src/')
    lines.append(f'/// file: {name}\n')
    with open(src) as f:
        lines.append(f.read().strip())
    lines.append('\n')

with open('export.txt', 'w') as f:
    f.writelines(lines)
