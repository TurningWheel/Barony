#!/usr/bin/env python3
import argparse
import csv
import html
from collections import defaultdict
from pathlib import Path

METRICS = ["rooms", "monsters", "gold", "items", "decorations"]


def parse_float(value: str):
    if value is None:
        return None
    value = value.strip()
    if value == "":
        return None
    try:
        return float(value)
    except ValueError:
        return None


def lerp(a: int, b: int, t: float) -> int:
    return int(round(a + (b - a) * t))


def color_for(metric: str, value, ranges):
    if value is None:
        return "#2a2f36"
    lo, hi = ranges[metric]
    if hi <= lo:
        t = 1.0
    else:
        t = (value - lo) / (hi - lo)
        t = min(1.0, max(0.0, t))
    # light blue -> deep blue
    r = lerp(228, 32, t)
    g = lerp(241, 98, t)
    b = lerp(252, 172, t)
    return f"#{r:02x}{g:02x}{b:02x}"


def main():
    parser = argparse.ArgumentParser(description="Generate a simple HTML heatmap for mapgen sweep results.")
    parser.add_argument("--input", required=True, help="Input CSV path.")
    parser.add_argument("--output", required=True, help="Output HTML path.")
    args = parser.parse_args()

    rows = []
    with open(args.input, "r", newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        for row in reader:
            rows.append(row)

    by_player = defaultdict(list)
    for row in rows:
        if row.get("status") != "pass":
            continue
        player_raw = row.get("players", "")
        if not player_raw.isdigit():
            continue
        player = int(player_raw)
        values = {}
        valid = True
        for metric in METRICS:
            parsed = parse_float(row.get(metric, ""))
            if parsed is None:
                valid = False
                break
            values[metric] = parsed
        if valid:
            by_player[player].append(values)

    avg_by_player = {}
    for player, entries in by_player.items():
        avg_by_player[player] = {
            metric: sum(e[metric] for e in entries) / len(entries)
            for metric in METRICS
        }

    ranges = {}
    for metric in METRICS:
        values = [avg_by_player[p][metric] for p in sorted(avg_by_player.keys()) if metric in avg_by_player[p]]
        if values:
            ranges[metric] = (min(values), max(values))
        else:
            ranges[metric] = (0.0, 1.0)

    min_player = 1
    max_player = 16

    lines = []
    lines.append("<!doctype html>")
    lines.append("<html><head><meta charset='utf-8'><title>Barony Mapgen Heatmap</title>")
    lines.append("<style>")
    lines.append("body{font-family:Menlo,Monaco,Consolas,monospace;background:#11161c;color:#e8edf3;padding:20px;}")
    lines.append("h1{font-size:20px;margin:0 0 8px 0;}")
    lines.append("p{margin:0 0 14px 0;color:#b6c2cf;}")
    lines.append("table{border-collapse:collapse;margin-top:12px;}")
    lines.append("th,td{border:1px solid #29323b;padding:6px 10px;text-align:right;}")
    lines.append("th:first-child,td:first-child{text-align:center;}")
    lines.append("th{background:#1d2630;}")
    lines.append("td.na{background:#2a2f36;color:#808a94;}")
    lines.append("code{background:#1d2630;padding:2px 4px;border-radius:4px;}")
    lines.append("</style></head><body>")
    lines.append("<h1>Barony Mapgen Heatmap (Players 1-16)</h1>")
    lines.append(f"<p>Source CSV: <code>{html.escape(str(Path(args.input)))}</code></p>")
    lines.append("<table>")
    header = "<tr><th>Players</th>" + "".join(f"<th>{html.escape(m)}</th>" for m in METRICS) + "<th>Runs</th></tr>"
    lines.append(header)

    for player in range(min_player, max_player + 1):
        lines.append("<tr>")
        lines.append(f"<td>{player}</td>")
        entry = avg_by_player.get(player)
        for metric in METRICS:
            if not entry:
                lines.append("<td class='na'>n/a</td>")
                continue
            value = entry[metric]
            bg = color_for(metric, value, ranges)
            lines.append(f"<td style='background:{bg}'>{value:.2f}</td>")
        lines.append(f"<td>{len(by_player.get(player, []))}</td>")
        lines.append("</tr>")

    lines.append("</table>")
    lines.append("</body></html>")

    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text("\n".join(lines), encoding="utf-8")


if __name__ == "__main__":
    main()
