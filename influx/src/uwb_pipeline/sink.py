from __future__ import annotations

from influxdb_client import InfluxDBClient
from influxdb_client.client.write_api import SYNCHRONOUS

from uwb_pipeline.config import InfluxConfig
from uwb_pipeline.line_protocol import to_line_protocol
from uwb_pipeline.model import TimeSeriesPoint


class InfluxSink:
    def __init__(self, config: InfluxConfig) -> None:
        self._config = config
        self._client = InfluxDBClient(url=config.url, token=config.token, org=config.org)
        self._writer = self._client.write_api(write_options=SYNCHRONOUS)

    def write_points(self, points: list[TimeSeriesPoint]) -> None:
        lines = [to_line_protocol(point) for point in points]
        self._writer.write(bucket=self._config.bucket, org=self._config.org, record=lines)

    def close(self) -> None:
        self._writer.close()
        self._client.close()
