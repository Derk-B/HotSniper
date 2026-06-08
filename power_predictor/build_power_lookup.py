import gzip
import os
import re
import csv

base_dir = "/home/derk/Downloads/raw/raw_blackscholes"
base_dir = "/home/derk/Documents/HotSniper/power_predictor/raw_blackscholes"

rows = []

for folder in sorted(os.listdir(base_dir)):
    match = re.search(r'_(\d+\.\d+)GHz\+', folder)
    if not match:
        print(folder)
        continue
    freq_ghz = float(match.group(1))

    log_path = os.path.join(base_dir, folder, "PeriodicPower.log.gz")
    if not os.path.exists(log_path):
        print(f"Missing: {log_path}")
        continue

    with gzip.open(log_path, 'rt') as f:
        lines = f.readlines()

    headers = lines[0].strip().split('\t')
    data = []
    for line in lines[1:]:
        line = line.strip()
        if not line:
            continue
        values = list(map(float, line.split('\t')))
        data.append(values)

    n = len(data)
    maxs = [max(data[r][c] for r in range(n)) for c in range(len(headers))]

    row = {"freq_GHz": freq_ghz}
    for h, v in zip(headers, maxs):
        row[h] = v
    rows.append(row)

rows.sort(key=lambda r: r["freq_GHz"])

if not rows:
    print("No data found.")
else:
    out_path = "/home/derk/Downloads/raw/power_lookup.csv"
    out_path = "/home/derk/Documents/HotSniper/power_predictor/power_lookup.csv"
    fieldnames = list(rows[0].keys())
    with open(out_path, 'w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)
    print(f"Written {len(rows)} rows to {out_path}")
    print(f"Components: {fieldnames[1:]}")
