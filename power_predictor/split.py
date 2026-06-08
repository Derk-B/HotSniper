budgets = []
with open("tsp_budgets.txt") as f:
    line = f.readlines()[0].strip()
    budgets = line.split(', ')
with open("tsp_input.txt", 'w') as f:
    for b in budgets:
        f.write(f"{b[1:-1]}\n")