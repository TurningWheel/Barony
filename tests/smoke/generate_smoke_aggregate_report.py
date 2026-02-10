#!/usr/bin/env python3
import argparse
import csv
import html
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Tuple


MAPGEN_METRICS = ["rooms", "monsters", "gold", "items", "decorations"]


def parse_float(value: Optional[str]) -> Optional[float]:
    if value is None:
        return None
    value = value.strip()
    if not value:
        return None
    try:
        return float(value)
    except ValueError:
        return None


def parse_int(value: Optional[str]) -> Optional[int]:
    parsed = parse_float(value)
    if parsed is None:
        return None
    return int(parsed)


def read_csv_rows(path: Path) -> List[Dict[str, str]]:
    if not path.exists():
        return []
    with path.open("r", newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def mean(values: Iterable[float]) -> Optional[float]:
    values = list(values)
    if not values:
        return None
    return sum(values) / len(values)


def variance(values: Iterable[float]) -> Optional[float]:
    values = list(values)
    if len(values) < 2:
        return None
    m = mean(values)
    assert m is not None
    return sum((v - m) ** 2 for v in values) / (len(values) - 1)


def stddev(values: Iterable[float]) -> Optional[float]:
    var = variance(values)
    if var is None:
        return None
    return var ** 0.5


def covariance(xs: List[float], ys: List[float]) -> Optional[float]:
    if len(xs) != len(ys) or len(xs) < 2:
        return None
    mx = mean(xs)
    my = mean(ys)
    assert mx is not None and my is not None
    return sum((x - mx) * (y - my) for x, y in zip(xs, ys)) / (len(xs) - 1)


def linear_slope(xs: List[float], ys: List[float]) -> Optional[float]:
    cov = covariance(xs, ys)
    varx = variance(xs)
    if cov is None or varx is None or varx == 0:
        return None
    return cov / varx


def correlation(xs: List[float], ys: List[float]) -> Optional[float]:
    cov = covariance(xs, ys)
    sx = stddev(xs)
    sy = stddev(ys)
    if cov is None or sx is None or sy is None or sx == 0 or sy == 0:
        return None
    return cov / (sx * sy)


def fmt_number(value: Optional[float], digits: int = 2) -> str:
    if value is None:
        return "n/a"
    return f"{value:.{digits}f}"


def section_header(title: str) -> str:
    return f"<h2>{html.escape(title)}</h2>"


def bullet(lines: List[str]) -> str:
    if not lines:
        return "<p>n/a</p>"
    return "<ul>" + "".join(f"<li>{line}</li>" for line in lines) + "</ul>"


def render_table(headers: List[str], rows: List[List[str]]) -> str:
    out: List[str] = []
    out.append("<table>")
    out.append("<tr>" + "".join(f"<th>{html.escape(h)}</th>" for h in headers) + "</tr>")
    for row in rows:
        out.append("<tr>" + "".join(f"<td>{cell}</td>" for cell in row) + "</tr>")
    out.append("</table>")
    return "\n".join(out)


def summarize_mapgen(csv_paths: List[Path]) -> str:
    if not csv_paths:
        return ""
    rows: List[Dict[str, str]] = []
    for path in csv_paths:
        rows.extend(read_csv_rows(path))
    if not rows:
        return section_header("Mapgen Sweep") + "<p>No rows found.</p>"

    passes = [r for r in rows if r.get("status") == "pass"]
    fails = [r for r in rows if r.get("status") != "pass"]

    by_player: Dict[int, List[Dict[str, float]]] = {}
    for row in passes:
        p = parse_int(row.get("players"))
        if p is None:
            continue
        metrics: Dict[str, float] = {}
        valid = True
        for metric in MAPGEN_METRICS:
            v = parse_float(row.get(metric))
            if v is None:
                valid = False
                break
            metrics[metric] = v
        if not valid:
            continue
        by_player.setdefault(p, []).append(metrics)

    avg_by_player: Dict[int, Dict[str, float]] = {}
    for player, entries in by_player.items():
        avg_by_player[player] = {
            metric: sum(e[metric] for e in entries) / len(entries) for metric in MAPGEN_METRICS
        }

    xs: List[float] = sorted(float(p) for p in avg_by_player.keys())
    metric_stats: Dict[str, Tuple[Optional[float], Optional[float], Optional[float], Optional[float]]] = {}
    for metric in MAPGEN_METRICS:
        ys = [avg_by_player[int(x)][metric] for x in xs]
        slope = linear_slope(xs, ys) if ys else None
        corr = correlation(xs, ys) if ys else None
        avg = mean(ys) if ys else None
        player_norm = mean([y / x for x, y in zip(xs, ys) if x > 0]) if ys else None
        metric_stats[metric] = (slope, corr, avg, player_norm)

    headers = ["Players"] + [m.capitalize() for m in MAPGEN_METRICS] + ["Runs"]
    table_rows: List[List[str]] = []
    for player in sorted(avg_by_player.keys()):
        row = [str(player)]
        row.extend(fmt_number(avg_by_player[player][metric]) for metric in MAPGEN_METRICS)
        row.append(str(len(by_player.get(player, []))))
        table_rows.append(row)

    stat_headers = ["Metric", "Slope/Player", "Correlation", "Mean", "Mean/Player"]
    stat_rows: List[List[str]] = []
    for metric in MAPGEN_METRICS:
        slope, corr, avg, player_norm = metric_stats[metric]
        stat_rows.append([
            html.escape(metric),
            fmt_number(slope, 3),
            fmt_number(corr, 3),
            fmt_number(avg, 2),
            fmt_number(player_norm, 3),
        ])

    lines: List[str] = [section_header("Mapgen Sweep")]
    lines.append(
        bullet([
            f"Rows: {len(rows)} ({len(passes)} pass / {len(fails)} fail)",
            "Inputs: " + ", ".join(html.escape(str(p)) for p in csv_paths),
        ])
    )
    lines.append("<h3>Per-Player Averages</h3>")
    lines.append(render_table(headers, table_rows))
    lines.append("<h3>Scaling Diagnostics</h3>")
    lines.append(render_table(stat_headers, stat_rows))
    return "\n".join(lines)


def summarize_soak(csv_paths: List[Path]) -> str:
    if not csv_paths:
        return ""
    rows: List[Dict[str, str]] = []
    for path in csv_paths:
        rows.extend(read_csv_rows(path))
    if not rows:
        return section_header("Soak Runs") + "<p>No rows found.</p>"
    passes = [r for r in rows if r.get("status") == "pass"]
    fails = [r for r in rows if r.get("status") != "pass"]
    table_rows = [
        [
            html.escape(r.get("run", "")),
            html.escape(r.get("status", "")),
            html.escape(r.get("instances", "")),
            html.escape(r.get("host_chunk_lines", "")),
            html.escape(r.get("client_reassembled_lines", "")),
            html.escape(r.get("mapgen_found", "")),
            html.escape(r.get("tx_mode", "")),
            html.escape(r.get("run_dir", "")),
        ]
        for r in rows
    ]
    lines = [section_header("Soak Runs")]
    lines.append(
        bullet([
            f"Rows: {len(rows)} ({len(passes)} pass / {len(fails)} fail)",
            "Inputs: " + ", ".join(html.escape(str(p)) for p in csv_paths),
        ])
    )
    lines.append(
        render_table(
            ["Run", "Status", "Instances", "Host Chunks", "Client Reassembled", "Mapgen", "TX Mode", "Run Dir"],
            table_rows,
        )
    )
    return "\n".join(lines)


def summarize_adversarial(csv_paths: List[Path]) -> str:
    if not csv_paths:
        return ""
    rows: List[Dict[str, str]] = []
    for path in csv_paths:
        rows.extend(read_csv_rows(path))
    if not rows:
        return section_header("Adversarial HELO") + "<p>No rows found.</p>"
    mismatches = [r for r in rows if r.get("match") not in ("1", "true", "TRUE", "yes", "pass")]
    lines = [section_header("Adversarial HELO")]
    lines.append(
        bullet([
            f"Rows: {len(rows)} ({len(rows) - len(mismatches)} matched / {len(mismatches)} mismatched expectations)",
            "Inputs: " + ", ".join(html.escape(str(p)) for p in csv_paths),
        ])
    )
    table_rows = [
        [
            html.escape(r.get("case_name", "")),
            html.escape(r.get("tx_mode", "")),
            html.escape(r.get("expected_result", "")),
            html.escape(r.get("observed_result", "")),
            html.escape(r.get("match", "")),
            html.escape(r.get("network_backend", "")),
            html.escape(r.get("host_chunk_lines", "")),
            html.escape(r.get("client_reassembled_lines", "")),
            html.escape(r.get("per_client_reassembly_counts", "")),
            html.escape(r.get("chunk_reset_lines", "")),
            html.escape(r.get("chunk_reset_reason_counts", "")),
            html.escape(r.get("tx_mode_applied", "")),
            html.escape(r.get("tx_mode_log_lines", "")),
            html.escape(r.get("tx_mode_packet_plan_ok", "")),
            html.escape(r.get("run_dir", "")),
        ]
        for r in rows
    ]
    lines.append(
        render_table(
            [
                "Case",
                "TX Mode",
                "Expected",
                "Observed",
                "Match",
                "Backend",
                "Host Chunks",
                "Client Reassembled",
                "Per-Client Reassembly",
                "Chunk Reset Count",
                "Chunk Reset Reasons",
                "TX Applied",
                "TX Log Lines",
                "TX Plan OK",
                "Run Dir",
            ],
            table_rows,
        )
    )
    return "\n".join(lines)


def summarize_churn(csv_paths: List[Path]) -> str:
    if not csv_paths:
        return ""
    rows: List[Dict[str, str]] = []
    for path in csv_paths:
        path_rows = read_csv_rows(path)
        for row in path_rows:
            row = dict(row)
            row["__source"] = str(path)
            rows.append(row)
    if not rows:
        return section_header("Join/Leave Churn") + "<p>No rows found.</p>"
    fails = [r for r in rows if r.get("status") != "pass"]
    lines = [section_header("Join/Leave Churn")]
    lines.append(
        bullet([
            f"Cycle rows: {len(rows)} ({len(rows) - len(fails)} pass / {len(fails)} fail)",
            "Inputs: " + ", ".join(html.escape(str(p)) for p in csv_paths),
        ])
    )
    table_rows = [
        [
            html.escape(r.get("__source", "")),
            html.escape(r.get("cycle", "")),
            html.escape(r.get("required_host_chunk_lines", "")),
            html.escape(r.get("observed_host_chunk_lines", "")),
            html.escape(r.get("status", "")),
        ]
        for r in rows
    ]
    lines.append(
        render_table(
            ["Source", "Cycle", "Required Host Chunks", "Observed Host Chunks", "Status"],
            table_rows,
        )
    )
    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate an aggregate smoke-test HTML report.")
    parser.add_argument("--output", required=True, help="Output HTML path.")
    parser.add_argument("--mapgen-csv", action="append", default=[], help="Mapgen sweep CSV path.")
    parser.add_argument("--soak-csv", action="append", default=[], help="Soak CSV path.")
    parser.add_argument("--adversarial-csv", action="append", default=[], help="Adversarial HELO CSV path.")
    parser.add_argument("--churn-csv", action="append", default=[], help="Join/leave churn CSV path.")
    args = parser.parse_args()

    mapgen_paths = [Path(p) for p in args.mapgen_csv]
    soak_paths = [Path(p) for p in args.soak_csv]
    adversarial_paths = [Path(p) for p in args.adversarial_csv]
    churn_paths = [Path(p) for p in args.churn_csv]

    sections = [
        summarize_mapgen(mapgen_paths),
        summarize_soak(soak_paths),
        summarize_adversarial(adversarial_paths),
        summarize_churn(churn_paths),
    ]
    sections = [s for s in sections if s]
    if not sections:
        sections = ["<p>No input CSVs were provided.</p>"]

    lines: List[str] = []
    lines.append("<!doctype html>")
    lines.append("<html><head><meta charset='utf-8'><title>Barony Smoke Aggregate Report</title>")
    lines.append("<style>")
    lines.append("body{font-family:Menlo,Monaco,Consolas,monospace;background:#10151b;color:#e9eef5;padding:20px;}")
    lines.append("h1{font-size:24px;margin:0 0 12px 0;} h2{margin-top:28px;} h3{margin-top:18px;}")
    lines.append("table{border-collapse:collapse;margin:10px 0 18px 0;}")
    lines.append("th,td{border:1px solid #2b3440;padding:6px 8px;text-align:left;}")
    lines.append("th{background:#1b2330;} tr:nth-child(even) td{background:#141b24;}")
    lines.append("ul{margin-top:6px;} code{background:#1b2330;padding:2px 4px;border-radius:4px;}")
    lines.append("</style></head><body>")
    lines.append("<h1>Barony Smoke Aggregate Report</h1>")
    lines.extend(sections)
    lines.append("</body></html>")

    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text("\n".join(lines), encoding="utf-8")


if __name__ == "__main__":
    main()
