from __future__ import annotations

from dataclasses import dataclass, field


@dataclass(slots=True)
class TimeSeriesPoint:
    measurement: str
    tags: dict[str, str]
    fields: dict[str, int | float | bool | str]
    timestamp_ns: int


@dataclass(slots=True)
class ParsedRow:
    timestamp_ms: int
    tags: dict[str, str]
    fields: dict[str, int | float | bool | str] = field(default_factory=dict)
