from __future__ import annotations

import argparse
from pathlib import Path

from uwb_pipeline.config import InfluxConfig, IngestConfig
from uwb_pipeline.pipeline import ingest


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Parse UWB CSV logs and write them to InfluxDB.")
    subparsers = parser.add_subparsers(dest="command", required=True)

    ingest_parser = subparsers.add_parser("ingest", help="Ingest a UWB CSV file.")
    ingest_parser.add_argument("--input", required=True, type=Path, help="Path to the CSV log file.")
    ingest_parser.add_argument("--source-file", default="", help="Source file tag value. Defaults to input file name.")
    ingest_parser.add_argument("--base-time-ns", type=int, default=None, help="Base unix time in nanoseconds.")
    ingest_parser.add_argument("--limit", type=int, default=None, help="Limit parsed rows for debugging.")
    ingest_parser.add_argument("--dry-run", action="store_true", help="Print line protocol instead of writing.")
    ingest_parser.add_argument("--url", default="http://localhost:8086", help="InfluxDB URL.")
    ingest_parser.add_argument("--org", default="uwb-lab", help="InfluxDB org.")
    ingest_parser.add_argument("--bucket", default="uwb", help="InfluxDB bucket.")
    ingest_parser.add_argument("--token", default="local-dev-token", help="InfluxDB token.")
    ingest_parser.add_argument("--batch-size", type=int, default=500, help="InfluxDB write batch size.")
    ingest_parser.add_argument("--flush-interval-ms", type=int, default=1000, help="InfluxDB flush interval in ms.")
    return parser


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()

    if args.command != "ingest":
        parser.error(f"Unsupported command: {args.command}")

    source_file = args.source_file or args.input.name
    base_time_ns = args.base_time_ns if args.base_time_ns is not None else 0
    ingest_config = IngestConfig(
        input_path=args.input,
        source_file=source_file,
        base_time_ns=base_time_ns,
        limit=args.limit,
        dry_run=args.dry_run,
    )

    if args.dry_run:
        for line in ingest(ingest_config):
            print(line)
        return

    influx_config = InfluxConfig(
        url=args.url,
        org=args.org,
        bucket=args.bucket,
        token=args.token,
        batch_size=args.batch_size,
        flush_interval_ms=args.flush_interval_ms,
    )
    from uwb_pipeline.sink import InfluxSink

    sink = InfluxSink(influx_config)
    try:
        ingest(ingest_config, sink=sink)
    finally:
        sink.close()


if __name__ == "__main__":
    main()
