import os
import re
import csv

script_dir = os.path.dirname(os.path.abspath(__file__))
data_path = os.path.join(script_dir, 'new_powers')
csv_path = os.path.join(script_dir, "power_mappings.csv")

binary_pattern = re.compile(r'^[01]{4}$')
subcore_pattern = re.compile(r"SUB-CORE TSP VALUES:\s*(\[.*?\])", re.DOTALL)
subcore_pattern2 = re.compile(r"subcore-tsp:\s*(\[.*?\])", re.DOTALL)
def parse_float_list(raw):
    raw = raw.replace('\n', ' ')
    return [float(x.replace('\n', '').replace(' ', '')) for x in re.findall(r"'([^']+)'", raw)]

data = {}
for fname in os.listdir(data_path):
    if not binary_pattern.match(fname):
        continue
    fpath = os.path.join(data_path, fname)
    with open(fpath, 'r') as f:
        content = f.read()
    m = subcore_pattern2.search(content)
    if not m:
        print(f"Warning: no sub-core TSP values in {fname}")
        continue
    values = parse_float_list(m.group(1))
    data[fname] = values

with open(csv_path, 'r', newline='') as f:
    reader = csv.reader(f)
    header = next(reader)
    existing_rows = {row[0]: row for row in reader if row}

value_columns = header[1:]
num_values = len(value_columns)

all_mappings = sorted(set(list(existing_rows.keys()) + list(data.keys())))

rows_out = []
for mapping in all_mappings:
    if mapping in data:
        values = data[mapping]
        if len(values) != num_values:
            print(f"Warning: {mapping} has {len(values)} values, expected {num_values}")
        rows_out.append([mapping] + [str(v) for v in values])
    else:
        rows_out.append(existing_rows.get(mapping, [mapping]))

with open(csv_path, 'w', newline='') as f:
    writer = csv.writer(f)
    writer.writerow(header)
    writer.writerows(rows_out)

print(f"Written {len(rows_out)} rows to {csv_path}")
for m, vals in sorted(data.items()):
    print(f"  {m}: {len(vals)} values")
