# UWB RTLS WebSocket 数据转发示例

## 架构说明

```
+-------------+      WebSocket      +----------------+      WebSocket      +-------------+
|   UWB 硬件   |  --(串口/UDP)-->  |  QT PC 客户端   |  ------>  |  Web 后端服务器  |  -->  |  浏览器前端  |
| (Tag/Anchor)|                    | (hr_rtls_pc)   |   ws://ip:port/rtls  | (Node.js)  |      | (HTML/JS)  |
+-------------+                    +----------------+                      +------------+      +------------+
```

## 快速开始

### 1. 启动 Web 后端服务器

```bash
cd web-demo
npm install
npm start
```

服务器启动后会显示：
- HTTP API: http://localhost:8080
- WebSocket: ws://localhost:8080/rtls

### 2. 启动 QT 客户端（通过终端指定 WebSocket 地址）

#### 方式一：命令行参数（推荐）

```bash
# 指定 WebSocket 服务器地址
./HR-RTLS_PC --ws-url ws://192.168.1.100:8080/rtls

# 或使用短参数
./HR-RTLS_PC -w ws://192.168.1.100:8080/rtls

# 自动连接默认地址 (ws://localhost:8080/rtls)
./HR-RTLS_PC --ws-auto

# 组合使用
./HR-RTLS_PC --ws-url ws://192.168.1.100:8080/rtls --ws-auto
```

#### 方式二：环境变量

```bash
# Linux / macOS
export RTLS_WS_URL=ws://192.168.1.100:8080/rtls
./HR-RTLS_PC

# Windows CMD
set RTLS_WS_URL=ws://192.168.1.100:8080/rtls
HR-RTLS_PC.exe

# Windows PowerShell
$env:RTLS_WS_URL="ws://192.168.1.100:8080/rtls"
.\HR-RTLS_PC.exe
```

#### 方式三：不启用 WebSocket（默认行为）

直接启动程序即可，不添加任何参数则不会连接 WebSocket 服务器：
```bash
./HR-RTLS_PC
```

#### 查看帮助

```bash
./HR-RTLS_PC --help
```

输出示例：
```
Usage: HR-RTLS_PC [options]
HR-RTLS PC Application with WebSocket data forwarding

Options:
  -h, --help           Displays help on commandline options.
  --help-all           Displays help including Qt specific options.
  -v, --version        Displays version information.
  -w, --ws-url <url>   WebSocket server URL (e.g., ws://192.168.1.100:8080/rtls)
  -a, --ws-auto        Auto-connect to WebSocket on startup
```

### 3. 打开前端页面

浏览器访问 `http://localhost:8080` 即可看到实时定位数据。

---

## 数据协议

QT 客户端发送的 JSON 数据格式：

```json
{
    "type": "tag_position",
    "timestamp": "2026-04-20T14:30:00.000Z",
    "tagId": 0,
    "position": { "x": 1.234, "y": 2.567, "z": 0.890 },
    "filteredPosition": { "x": 1.230, "y": 2.560, "z": 0.895 },
    "ranges": { "range0": 1234, "range1": 1567, ... },
    "imu": {
        "accel": { "x": 0.01, "y": -0.02, "z": 9.80 },
        "gyro": { "x": 0.001, "y": 0.002, "z": -0.001 },
        "mag": { "x": 25.6, "y": 2.8, "z": 13.9 },
        "attitude": { "pitch": 1.02, "roll": 2.85, "yaw": -4.48 }
    },
    "alarm": 0,
    "rxPower": -85.5
}
```

---

## API 接口

| 接口 | 说明 |
|------|------|
| `GET /api/position/latest` | 获取每个 Tag 的最新数据 |
| `GET /api/position/history?tagId=0` | 获取指定 Tag 的历史数据 |
| `WS /rtls` | WebSocket 实时数据流 |

---

## 部署建议

### 局域网部署（同一 WiFi）

```bash
# Web 服务器运行在一台电脑上（IP: 192.168.1.100）
node server.js

# QT 客户端在另一台电脑上通过终端连接
./HR-RTLS_PC --ws-url ws://192.168.1.100:8080/rtls
```

### 跨网络部署（VPS/公网服务器）

将 Node.js 后端部署到具有公网 IP 的服务器：

```bash
# 服务器上
export PORT=8080
node server.js

# QT 客户端连接公网地址
./HR-RTLS_PC --ws-url ws://your-server-ip:8080/rtls
```

### 使用 Nginx 反向代理（HTTPS/WSS）

如需加密传输，配置 Nginx：

```nginx
server {
    listen 443 ssl;
    server_name your-domain.com;

    location /rtls {
        proxy_pass http://localhost:8080/rtls;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
    }
}
```

QT 客户端连接：
```bash
./HR-RTLS_PC --ws-url wss://your-domain.com/rtls
```

---

## 故障排查

| 问题 | 解决方案 |
|------|----------|
| WebSocket 连接失败 | 检查防火墙是否开放端口，确认服务器 IP 正确 |
| 数据未显示在前端 | 检查 QT 端是否已正常连接串口并接收到 UWB 数据 |
| 前端页面空白 | 检查浏览器控制台是否有跨域或 WebSocket 错误 |
| 中文乱码 | 确保终端使用 UTF-8 编码 |
