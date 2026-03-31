from __future__ import annotations

from uwb_pipeline.config import IngestConfig
from uwb_pipeline.constants import MEASUREMENT
from uwb_pipeline.line_protocol import to_line_protocol
from uwb_pipeline.model import TimeSeriesPoint
from uwb_pipeline.parser import parse_csv_rows

from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from uwb_pipeline.sink import InfluxSink


def build_points(config: IngestConfig) -> list[TimeSeriesPoint]:
    parsed_rows = parse_csv_rows(config.input_path)
    if config.limit is not None:
        parsed_rows = parsed_rows[: config.limit]

    points: list[TimeSeriesPoint] = []
    for row in parsed_rows:
        tags = dict(row.tags)
        tags["source_file"] = config.source_file
        points.append(
            TimeSeriesPoint(
                measurement=MEASUREMENT,
                tags=tags,
                fields=row.fields,
                timestamp_ns=config.base_time_ns + (row.timestamp_ms * 1_000_000),
            )
        )
    return points


def ingest(config: IngestConfig, sink: InfluxSink | None = None) -> list[str]:
    points = build_points(config)
    lines = [to_line_protocol(point) for point in points]
    if config.dry_run or sink is None:
        return lines
    sink.write_points(points)
    return lines
