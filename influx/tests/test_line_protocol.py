from __future__ import annotations

from uwb_pipeline.line_protocol import to_line_protocol
from uwb_pipeline.model import TimeSeriesPoint


def test_to_line_protocol_formats_tags_fields_and_timestamp() -> None:
    point = TimeSeriesPoint(
        measurement="uwb_tag_telemetry",
        tags={"source_file": "log.csv", "tag_id": "Tag 0"},
        fields={"position_x_m": 2.995, "range0_m": 4.29, "sequence": 1},
        timestamp_ns=123456789,
    )
    encoded = to_line_protocol(point)
    assert encoded == (
        "uwb_tag_telemetry,source_file=log.csv,tag_id=Tag\\ 0 "
        "position_x_m=2.995,range0_m=4.29,sequence=1i 123456789"
    )
