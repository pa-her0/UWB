from __future__ import annotations

from pathlib import Path

from uwb_pipeline.parser import parse_csv_rows


def test_parse_csv_rows_handles_short_header_and_long_rows() -> None:
    rows = parse_csv_rows(Path("log.csv"))
    assert rows
    first = rows[0]
    assert first.timestamp_ms == 96827
    assert first.tags["tag_id"] == "Tag 0"
    assert first.fields["position_x_m"] == 2.995
    assert first.fields["range0_m"] == 4.29
    assert first.fields["yaw"] == 49.303
    assert first.fields["extra_col_12"] == 49.303
