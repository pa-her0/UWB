# UWB -> InfluxDB Pipeline

这个项目骨架面向当前目录下的 `log.csv`：读取 UWB 导出的 CSV 日志，映射为时序点，编码为 InfluxDB Line Protocol，并写入 InfluxDB。

## 设计目标

1. 解析层和写库层解耦，便于替换不同厂商的 UWB 日志格式。
2. CSV 脏数据可控，优先兼容当前 `log.csv` 这种“表头少于数据列”的非规范格式。
3. 数据模型显式区分 `measurement / tag / field / timestamp`。
4. InfluxDB 负责落盘，依赖其 WAL/TSM 持久化能力，不自行重复造存储层。

## 当前日志的时序建模

- `measurement`: `uwb_tag_telemetry`
- `tags`: `tag_id`, `source_file`
- `timestamp`: `rangetime(ms)` 转换为纳秒时间戳
- `fields`:
  - 位置: `position_x_m`, `position_y_m`, `position_z_m`
  - 测距: `range0_m` 到 `range7_m`
  - 信号: `rx_pwr_dbm`
  - 惯导: `accel_*`, `gyro_*`, `mag_*`
  - 姿态: `roll`, `pitch`, `yaw`
  - 尾部扩展列: `extra_col_01` 到 `extra_col_12`

`log.csv` 每行实际有 39 列，但表头只有 27 列。骨架会自动补齐尾部扩展列，避免 CSV 解析失败。

## 目录结构

```text
.
├── docker-compose.yml
├── pyproject.toml
├── README.md
├── src/uwb_pipeline
│   ├── cli.py
│   ├── config.py
│   ├── constants.py
│   ├── pipeline.py
│   ├── line_protocol.py
│   ├── model.py
│   ├── parser.py
│   └── sink.py
└── tests
    ├── test_line_protocol.py
    └── test_parser.py
```

## 快速开始

### 1. 启动 InfluxDB 2

```bash
docker compose up -d
```

默认会启动一个本地 InfluxDB，并在 `./.influxdb2` 下持久化数据。

### 2. 安装项目依赖

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -e .[dev]
```

### 3. 导入 `log.csv`

```bash
uwb-pipeline ingest \
  --input log.csv \
  --url http://localhost:8086 \
  --org uwb-lab \
  --bucket uwb \
  --token local-dev-token \
  --source-file log.csv
```

如果只想检查 Line Protocol 而不写库：

```bash
uwb-pipeline ingest --input log.csv --dry-run --limit 3
```

### 4. 生成某个 Tag 的轨迹图

先编辑 [`configs/trajectory_plot.json`](/Users/jiely/Documents/1 比赛/AI CUP2026/influx/configs/trajectory_plot.json)：

- `plot.tag_id`: 要查询的 tag
- `plot.anchors_path`: 基站坐标 JSON
- `plot.output_path`: 输出图片路径

基站坐标格式可参考 [`anchors.example.json`](/Users/jiely/Documents/1 比赛/AI CUP2026/influx/anchors.example.json)。

然后运行：

```bash
./run_trajectory_plot.sh
```

也可以临时覆盖配置里的 tag 或输出路径：

```bash
./run_trajectory_plot.sh configs/trajectory_plot.json \
  --tag-id "Tag 0" \
  --output outputs/tag0_trajectory.svg
```

## 数据库存储形式

当前使用的是 InfluxDB 2.x，数据存储在 bucket `uwb` 中，measurement 为 `uwb_tag_telemetry`。

### 逻辑模型

- `bucket`: `uwb`
- `measurement`: `uwb_tag_telemetry`
- `tags`: `tag_id`, `source_file`
- `fields`:
  - 位置: `position_x_m`, `position_y_m`, `position_z_m`
  - 测距: `range0_m` 到 `range7_m`
  - 信号: `rx_pwr_dbm`
  - 惯导: `accel_*`, `gyro_*`, `mag_*`
  - 姿态: `roll`, `pitch`, `yaw`
  - 尾部扩展列: `extra_col_01` 到 `extra_col_12`
- `timestamp`: `base_time_ns + rangetime_ms * 1_000_000`

其中时间戳计算逻辑在 [`src/uwb_pipeline/pipeline.py`](/Users/jiely/Documents/1 比赛/AI CUP2026/influx/src/uwb_pipeline/pipeline.py)：

```python
timestamp_ns = config.base_time_ns + (row.timestamp_ms * 1_000_000)
```

如果 `base_time_ns = 0`，那么时间就是把 `rangetime_ms` 当作从 Unix epoch 开始的相对毫秒数。
如果导入时显式传入真实起始时间，例如 `2026-03-25T00:00:00Z` 对应的纳秒值，那么库里的 `_time` 就会落在真实日期上。

### 在 InfluxDB 中的实际形态

InfluxDB 查询结果通常是“长表”结构，每个 field 会单独成为一行：

```text
_time                      _measurement        tag_id   source_file   _field         _value
2026-03-25T00:01:36.827Z   uwb_tag_telemetry   Tag 0    log.csv       position_x_m   2.995
2026-03-25T00:01:36.827Z   uwb_tag_telemetry   Tag 0    log.csv       position_y_m   2.449
2026-03-25T00:01:36.827Z   uwb_tag_telemetry   Tag 0    log.csv       yaw            49.303
```

对应的一条 Line Protocol 大致长这样：

```text
uwb_tag_telemetry,source_file=log.csv,tag_id=Tag\ 0 position_x_m=2.995,position_y_m=2.449,yaw=49.303 1774396896827000000
```

本地 Docker 持久化目录在：

- [`.influxdb2`](/Users/jiely/Documents/1 比赛/AI CUP2026/influx/.influxdb2)

## 查询方式与常用语法

当前主要使用 InfluxDB 2.x 的 Flux 查询语法。基本查询通常从下面这个骨架开始：

```flux
from(bucket: "uwb")
  |> range(start: time(v: "2026-03-25T00:00:00Z"))
  |> filter(fn: (r) => r._measurement == "uwb_tag_telemetry")
```

含义：

- `from(bucket: ...)`: 从哪个 bucket 查询
- `range(...)`: 指定时间范围，InfluxDB 查询必须带时间范围
- `filter(...)`: 过滤 measurement、tag 或 field

### 常见字段说明

- `_measurement`: 类似关系型数据库里的表名
- `tag_id`: 轨迹所属标签
- `_field`: 当前行是哪一个字段，如 `position_x_m`
- `_value`: 当前字段的值
- `_time`: 时间戳

### 常用查询示例

查询最近时间范围内的所有数据：

```flux
from(bucket: "uwb")
  |> range(start: time(v: "2026-03-25T00:00:00Z"))
  |> filter(fn: (r) => r._measurement == "uwb_tag_telemetry")
  |> limit(n: 10)
```

查询某个 tag 的某个字段：

```flux
from(bucket: "uwb")
  |> range(start: time(v: "2026-03-25T00:00:00Z"))
  |> filter(fn: (r) => r._measurement == "uwb_tag_telemetry")
  |> filter(fn: (r) => r.tag_id == "Tag 0")
  |> filter(fn: (r) => r._field == "position_x_m")
  |> sort(columns: ["_time"])
```

查询某个 tag 的二维轨迹：

```flux
from(bucket: "uwb")
  |> range(start: time(v: "2026-03-25T00:00:00Z"))
  |> filter(fn: (r) => r._measurement == "uwb_tag_telemetry")
  |> filter(fn: (r) => r.tag_id == "Tag 0")
  |> filter(fn: (r) => r._field == "position_x_m" or r._field == "position_y_m")
  |> pivot(rowKey: ["_time"], columnKey: ["_field"], valueColumn: "_value")
  |> keep(columns: ["_time", "tag_id", "position_x_m", "position_y_m"])
  |> sort(columns: ["_time"])
```

查询某个 tag 的姿态：

```flux
from(bucket: "uwb")
  |> range(start: time(v: "2026-03-25T00:00:00Z"))
  |> filter(fn: (r) => r._measurement == "uwb_tag_telemetry")
  |> filter(fn: (r) => r.tag_id == "Tag 0")
  |> filter(fn: (r) => r._field == "roll" or r._field == "pitch" or r._field == "yaw")
  |> sort(columns: ["_time"])
```

统计某个字段的数据点数量：

```flux
from(bucket: "uwb")
  |> range(start: time(v: "2026-03-25T00:00:00Z"))
  |> filter(fn: (r) => r._measurement == "uwb_tag_telemetry")
  |> filter(fn: (r) => r._field == "accel_x")
  |> count()
```

按时间窗口做聚合：

```flux
from(bucket: "uwb")
  |> range(start: time(v: "2026-03-25T00:00:00Z"))
  |> filter(fn: (r) => r._measurement == "uwb_tag_telemetry")
  |> filter(fn: (r) => r._field == "yaw")
  |> aggregateWindow(every: 1s, fn: mean)
```

列出当前 measurement 里的 tag 值：

```flux
import "influxdata/influxdb/schema"

schema.tagValues(
  bucket: "uwb",
  tag: "tag_id",
  predicate: (r) => r._measurement == "uwb_tag_telemetry",
  start: time(v: "2026-03-25T00:00:00Z")
)
```

### 如何执行查询

1. 在 InfluxDB Web UI 中查询

- 打开 `http://localhost:8086`
- 登录后进入 Data Explorer
- 直接编写 Flux

2. 使用容器内 CLI 查询

```bash
docker exec uwb-influxdb sh -lc 'influx query --org uwb-lab --token local-dev-token "
from(bucket: \"uwb\")
  |> range(start: time(v: \"2026-03-25T00:00:00Z\"))
  |> filter(fn: (r) => r._measurement == \"uwb_tag_telemetry\")
  |> limit(n: 10)
"'
```

3. 使用 Python 客户端查询

项目中的轨迹图脚本 [`src/uwb_pipeline/trajectory_plot.py`](/Users/jiely/Documents/1 比赛/AI CUP2026/influx/src/uwb_pipeline/trajectory_plot.py) 就是通过 Python 客户端执行 Flux 查询并生成可视化图片的。

## 代码分层建议

### 1. 输入层

- 当前先读离线 CSV 文件。
- 后续可追加串口/Socket/Kafka 等 source，统一产出原始记录。

### 2. 解析层

- 负责把原始 CSV 行清洗成结构化记录。
- 对脏值如 `null`、空串、缺损列做兼容。
- 不关心 InfluxDB 细节。

### 3. 映射层

- 决定哪些字段作为 tags，哪些字段作为 fields。
- 负责 measurement 命名、单位统一、时间戳换算。
- 如果后续要做 anchor/tag/device 多 measurement 拆分，这层扩展最自然。

### 4. Sink 层

- 只负责把 `TimeSeriesPoint` 写入 InfluxDB。
- 同时保留 `to_line_protocol()`，便于调试、压测和后续接 Kafka/HTTP。
