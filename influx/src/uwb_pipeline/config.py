from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path


@dataclass(slots=True)
class InfluxConfig:
    url: str
    org: str
    bucket: str
    token: str
    batch_size: int = 500
    flush_interval_ms: int = 1_000


@dataclass(slots=True)
class IngestConfig:
    input_path: Path
    source_file: str
    base_time_ns: int = 0
    limit: int | None = None
    dry_run: bool = False
