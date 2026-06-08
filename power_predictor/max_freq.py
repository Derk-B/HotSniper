"""
Find the maximum frequency where no component exceeds its power budget.

Only specify budgets for the components you want to constrain.
Unspecified components are unconstrained.

Usage:
  python3 max_freq.py --file budgets.txt
  python3 max_freq.py component=budget [component=budget ...]

File format: one entry per line, either 'component budget' or 'component,budget'.
Run with --list-components to print all available component names.
"""

import argparse
import csv
import sys

LOOKUP = "/home/derk/Downloads/raw/power_lookup.csv"


def load_lookup():
    table = []
    with open(LOOKUP, newline='') as f:
        reader = csv.DictReader(f)
        components = [c for c in reader.fieldnames if c != "freq_GHz"]
        for row in reader:
            table.append({k: float(v) for k, v in row.items()})
    return sorted(table, key=lambda r: r["freq_GHz"]), components


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("budgets", nargs="*", metavar="component=budget",
                        help="Budget constraints as component=value pairs")
    parser.add_argument("--file", metavar="PATH",
                        help="Load budgets from a file (one 'component budget' per line)")
    parser.add_argument("--list-components", action="store_true",
                        help="Print all available component names and exit")
    args = parser.parse_args()

    table, components = load_lookup()

    if args.list_components:
        for c in components:
            print(c)
        return

    if args.file:
        budgets = {}
        with open(args.file) as f:
            for lineno, line in enumerate(f, 1):
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                parts = line.replace(',', ' ').split()
                if len(parts) != 2:
                    sys.exit(f"Line {lineno}: expected 'component budget', got: {line!r}")
                budgets[parts[0]] = float(parts[1])
    elif args.budgets:
        budgets = {}
        for token in args.budgets:
            if '=' not in token:
                sys.exit(f"Expected component=budget, got: {token!r}")
            comp, val = token.split('=', 1)
            budgets[comp] = float(val)
    else:
        parser.print_help()
        sys.exit(1)

    unknown = set(budgets) - set(components)
    if unknown:
        sys.exit(f"Unknown component(s): {', '.join(sorted(unknown))}\n"
                 f"Run with --list-components to see available names.")

    best = None
    for row in table:
        freq = row["freq_GHz"]
        violations = {
            comp: (row[comp], cap)
            for comp, cap in budgets.items()
            if row[comp] > cap
        }
        if not violations:
            best = freq
        else:
            print(f"  {freq:.1f} GHz — exceeds budget: "
                  + ", ".join(f"{c}={v:.4f} (budget {b:.4f})"
                               for c, (v, b) in violations.items()))

    if best is not None:
        print(f"\nMax frequency within all power budgets: {best:.1f} GHz")
    else:
        print("\nNo frequency satisfies all power budgets.")


if __name__ == "__main__":
    main()
