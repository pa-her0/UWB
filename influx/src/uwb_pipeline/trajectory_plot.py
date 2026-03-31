from __future__ import annotations

import argparse
import json
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path

from influxdb_client import InfluxDBClient


@dataclass(slots=True)
class Anchor:
    name: str
    x: float
    y: float


@dataclass(slots=True)
class TrajectoryPoint:
    time: datetime
    x: float
    y: float


@dataclass(slots=True)
class InfluxQueryConfig:
    url: str
    org: str
    bucket: str
    token: str
    measurement: str
    start: str
    stop: str | None = None


@dataclass(slots=True)
class PlotConfig:
    tag_id: str
    anchors_path: Path
    output_path: Path
    influx: InfluxQueryConfig


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Query a tag trajectory from InfluxDB and render it with anchor positions to an SVG image."
    )
    parser.add_argument(
        "--config",
        type=Path,
        default=Path("configs/trajectory_plot.json"),
        help="Path to the visualization config JSON.",
    )
    parser.add_argument("--tag-id", help="Override tag id from config.")
    parser.add_argument("--output", type=Path, help="Override output SVG path from config.")
    parser.add_argument("--anchors", type=Path, help="Override anchor JSON path from config.")
    parser.add_argument("--start", help="Override Flux range start from config.")
    parser.add_argument("--stop", help="Override Flux range stop from config.")
    return parser


def load_plot_config(path: Path) -> PlotConfig:
    payload = json.loads(path.read_text(encoding="utf-8"))

    influx_raw = payload.get("influxdb")
    plot_raw = payload.get("plot")
    if not isinstance(influx_raw, dict) or not isinstance(plot_raw, dict):
        raise ValueError(f"{path} must contain 'influxdb' and 'plot' objects")

    influx = InfluxQueryConfig(
        url=str(influx_raw["url"]),
        org=str(influx_raw["org"]),
        bucket=str(influx_raw["bucket"]),
        token=str(influx_raw["token"]),
        measurement=str(influx_raw.get("measurement", "uwb_tag_telemetry")),
        start=str(influx_raw.get("start", "1970-01-01T00:00:00Z")),
        stop=None if influx_raw.get("stop") in (None, "") else str(influx_raw["stop"]),
    )
    plot = PlotConfig(
        tag_id=str(plot_raw["tag_id"]),
        anchors_path=(path.parent / str(plot_raw["anchors_path"])).resolve(),
        output_path=(path.parent / str(plot_raw["output_path"])).resolve(),
        influx=influx,
    )
    return plot


def apply_overrides(config: PlotConfig, args: argparse.Namespace) -> PlotConfig:
    influx = InfluxQueryConfig(
        url=config.influx.url,
        org=config.influx.org,
        bucket=config.influx.bucket,
        token=config.influx.token,
        measurement=config.influx.measurement,
        start=args.start or config.influx.start,
        stop=args.stop if args.stop is not None else config.influx.stop,
    )
    return PlotConfig(
        tag_id=args.tag_id or config.tag_id,
        anchors_path=(args.anchors or config.anchors_path).resolve(),
        output_path=(args.output or config.output_path).resolve(),
        influx=influx,
    )


def load_anchors(path: Path) -> list[Anchor]:
    payload = json.loads(path.read_text(encoding="utf-8"))
    anchors_raw = payload.get("anchors")
    if not isinstance(anchors_raw, list) or not anchors_raw:
        raise ValueError(f"{path} must contain a non-empty 'anchors' array")

    anchors: list[Anchor] = []
    for item in anchors_raw:
        if not isinstance(item, dict):
            raise ValueError("Each anchor entry must be an object")
        anchors.append(Anchor(name=str(item["name"]), x=float(item["x"]), y=float(item["y"])))
    return anchors


def query_trajectory(config: PlotConfig) -> list[TrajectoryPoint]:
    influx = config.influx
    start_expr = f'time(v: {json.dumps(influx.start)})'
    stop_clause = f', stop: time(v: {json.dumps(influx.stop)})' if influx.stop else ""
    query = f"""
from(bucket: {json.dumps(influx.bucket)})
  |> range(start: {start_expr}{stop_clause})
  |> filter(fn: (r) => r._measurement == {json.dumps(influx.measurement)})
  |> filter(fn: (r) => r.tag_id == {json.dumps(config.tag_id)})
  |> filter(fn: (r) => r._field == "position_x_m" or r._field == "position_y_m")
  |> sort(columns: ["_time"])
""".strip()

    client = InfluxDBClient(url=influx.url, token=influx.token, org=influx.org)
    try:
        grouped: dict[datetime, dict[str, float]] = {}
        for table in client.query_api().query(query):
            for record in table.records:
                point_time = record.get_time()
                value = record.get_value()
                if point_time is None or value is None:
                    continue
                grouped.setdefault(point_time, {})[str(record.get_field())] = float(value)

        points: list[TrajectoryPoint] = []
        for point_time in sorted(grouped):
            pair = grouped[point_time]
            if "position_x_m" not in pair or "position_y_m" not in pair:
                continue
            points.append(TrajectoryPoint(time=point_time, x=pair["position_x_m"], y=pair["position_y_m"]))
        return points
    finally:
        client.close()


def svg_escape(value: str) -> str:
    return (
        value.replace("&", "&amp;")
        .replace("<", "&lt;")
        .replace(">", "&gt;")
        .replace('"', "&quot;")
        .replace("'", "&apos;")
    )


def render_svg(tag_id: str, anchors: list[Anchor], points: list[TrajectoryPoint], output_path: Path) -> None:
    if not points:
        raise ValueError(f"No trajectory points found for tag_id={tag_id!r}")

    width = 1200
    height = 900
    margin = 80
    all_x = [point.x for point in points] + [anchor.x for anchor in anchors]
    all_y = [point.y for point in points] + [anchor.y for anchor in anchors]
    min_x = min(all_x)
    max_x = max(all_x)
    min_y = min(all_y)
    max_y = max(all_y)
    span_x = max(max_x - min_x, 1.0)
    span_y = max(max_y - min_y, 1.0)
    pad_x = span_x * 0.1
    pad_y = span_y * 0.1
    min_x -= pad_x
    max_x += pad_x
    min_y -= pad_y
    max_y += pad_y
    plot_width = width - (margin * 2)
    plot_height = height - (margin * 2)

    def project(x: float, y: float) -> tuple[float, float]:
        px = margin + ((x - min_x) / (max_x - min_x)) * plot_width
        py = height - margin - ((y - min_y) / (max_y - min_y)) * plot_height
        return px, py

    polyline = " ".join(f"{project(point.x, point.y)[0]:.2f},{project(point.x, point.y)[1]:.2f}" for point in points)
    start_px, start_py = project(points[0].x, points[0].y)
    end_px, end_py = project(points[-1].x, points[-1].y)

    axis_labels: list[str] = []
    tick_count = 6
    for idx in range(tick_count + 1):
        ratio = idx / tick_count
        x_value = min_x + (max_x - min_x) * ratio
        y_value = min_y + (max_y - min_y) * ratio
        x_px = margin + plot_width * ratio
        y_px = height - margin - plot_height * ratio
        axis_labels.append(
            f'<line x1="{x_px:.2f}" y1="{margin}" x2="{x_px:.2f}" y2="{height - margin}" stroke="#e4e7eb" stroke-width="1" />'
        )
        axis_labels.append(
            f'<line x1="{margin}" y1="{y_px:.2f}" x2="{width - margin}" y2="{y_px:.2f}" stroke="#e4e7eb" stroke-width="1" />'
        )
        axis_labels.append(
            f'<text x="{x_px:.2f}" y="{height - margin + 28}" text-anchor="middle" font-size="14" fill="#334155">{x_value:.2f}</text>'
        )
        axis_labels.append(
            f'<text x="{margin - 16}" y="{y_px + 5:.2f}" text-anchor="end" font-size="14" fill="#334155">{y_value:.2f}</text>'
        )

    anchor_nodes: list[str] = []
    for anchor in anchors:
        px, py = project(anchor.x, anchor.y)
        anchor_nodes.append(
            f'<circle cx="{px:.2f}" cy="{py:.2f}" r="8" fill="#d9480f" stroke="#7c2d12" stroke-width="2" />'
        )
        anchor_nodes.append(
            f'<text x="{px + 12:.2f}" y="{py - 12:.2f}" font-size="16" font-weight="700" fill="#7c2d12">{svg_escape(anchor.name)} ({anchor.x:.2f}, {anchor.y:.2f})</text>'
        )

    start_label = points[0].time.astimezone(timezone.utc).isoformat()
    end_label = points[-1].time.astimezone(timezone.utc).isoformat()
    rendered = f"""<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">
  <rect width="100%" height="100%" fill="#f8fafc" />
  <rect x="{margin}" y="{margin}" width="{plot_width}" height="{plot_height}" fill="#ffffff" stroke="#cbd5e1" stroke-width="2" rx="18" />
  {''.join(axis_labels)}
  <line x1="{margin}" y1="{height - margin}" x2="{width - margin}" y2="{height - margin}" stroke="#0f172a" stroke-width="2" />
  <line x1="{margin}" y1="{margin}" x2="{margin}" y2="{height - margin}" stroke="#0f172a" stroke-width="2" />
  <polyline points="{polyline}" fill="none" stroke="#2563eb" stroke-width="4" stroke-linecap="round" stroke-linejoin="round" />
  <circle cx="{start_px:.2f}" cy="{start_py:.2f}" r="9" fill="#16a34a" />
  <circle cx="{end_px:.2f}" cy="{end_py:.2f}" r="9" fill="#dc2626" />
  {''.join(anchor_nodes)}
  <text x="{margin}" y="38" font-size="28" font-weight="700" fill="#0f172a">Trajectory for {svg_escape(tag_id)}</text>
  <text x="{margin}" y="64" font-size="16" fill="#475569">Blue line: tag trajectory. Green: start. Red: end. Orange: anchors.</text>
  <text x="{width - margin}" y="38" text-anchor="end" font-size="15" fill="#475569">Points: {len(points)}</text>
  <text x="{width - margin}" y="60" text-anchor="end" font-size="15" fill="#475569">UTC start: {svg_escape(start_label)}</text>
  <text x="{width - margin}" y="82" text-anchor="end" font-size="15" fill="#475569">UTC end: {svg_escape(end_label)}</text>
  <text x="{width / 2:.2f}" y="{height - 20}" text-anchor="middle" font-size="16" fill="#0f172a">position_x_m</text>
  <text x="28" y="{height / 2:.2f}" text-anchor="middle" font-size="16" fill="#0f172a" transform="rotate(-90 28 {height / 2:.2f})">position_y_m</text>
</svg>
"""
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(rendered, encoding="utf-8")


def run_plot(config: PlotConfig) -> Path:
    anchors = load_anchors(config.anchors_path)
    points = query_trajectory(config)
    render_svg(config.tag_id, anchors, points, config.output_path)
    return config.output_path


def main() -> None:
    args = build_parser().parse_args()
    config = apply_overrides(load_plot_config(args.config.resolve()), args)
    output_path = run_plot(config)
    print(f"Saved trajectory image to {output_path}")


if __name__ == "__main__":
    main()
