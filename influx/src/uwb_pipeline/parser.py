from __future__ import annotations

import csv
from pathlib import Path

from uwb_pipeline.constants import CSV_SCHEMA, TAG_COLUMNS
from uwb_pipeline.model import ParsedRow


def _to_number(raw: str) -> float | None:
    text = raw.strip()
    if text == "" or text.lower() == "null":
        return None
    return float(text)


def parse_csv_rows(input_path: Path) -> list[ParsedRow]:
    rows: list[ParsedRow] = []
    with input_path.open(newline="", encoding="utf-8") as handle:
        reader = csv.reader(handle)
        next(reader, None)
        for raw_row in reader:
            if not raw_row:
                continue
            normalized = list(raw_row[: len(CSV_SCHEMA)])
            if len(normalized) < len(CSV_SCHEMA):
                normalized.extend([""] * (len(CSV_SCHEMA) - len(normalized)))

            row_map = dict(zip(CSV_SCHEMA, normalized))
            timestamp_raw = row_map["rangetime_ms"].strip()
            tag_id = row_map["tag_id"].strip()
            if not timestamp_raw or not tag_id:
                continue

            tags = {name: row_map[name].strip() for name in TAG_COLUMNS}
            fields: dict[str, int | float | bool | str] = {}
            for column, raw_value in row_map.items():
                if column in ("rangetime_ms", *TAG_COLUMNS):
                    continue
                value = _to_number(raw_value)
                if value is not None:
                    fields[column] = value

            rows.append(ParsedRow(timestamp_ms=int(timestamp_raw), tags=tags, fields=fields))

    return rows
