from __future__ import annotations

from uwb_pipeline.model import TimeSeriesPoint


def _escape(value: str) -> str:
    return value.replace("\\", "\\\\").replace(" ", "\\ ").replace(",", "\\,").replace("=", "\\=")


def _encode_field_value(value: int | float | bool | str) -> str:
    if isinstance(value, bool):
        return "true" if value else "false"
    if isinstance(value, int) and not isinstance(value, bool):
        return f"{value}i"
    if isinstance(value, float):
        return format(value, "g")
    return f"\"{str(value).replace(chr(34), r'\\\"')}\""


def to_line_protocol(point: TimeSeriesPoint) -> str:
    tag_part = ",".join(f"{_escape(key)}={_escape(value)}" for key, value in sorted(point.tags.items()))
    field_part = ",".join(
        f"{_escape(key)}={_encode_field_value(value)}" for key, value in sorted(point.fields.items())
    )
    head = _escape(point.measurement)
    if tag_part:
        head = f"{head},{tag_part}"
    return f"{head} {field_part} {point.timestamp_ns}"
