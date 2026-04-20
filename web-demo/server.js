const express = require('express');
const { WebSocketServer } = require('ws');
const http = require('http');
const path = require('path');

const app = express();
const server = http.createServer(app);

// 提供静态前端页面
app.use(express.static(path.join(__dirname, 'public')));

// REST API: 获取最新的定位数据
let latestData = {};
let dataHistory = [];
const MAX_HISTORY = 1000;

app.get('/api/position/latest', (req, res) => {
    res.json(latestData);
});

app.get('/api/position/history', (req, res) => {
    const tagId = req.query.tagId;
    if (tagId !== undefined) {
        res.json(dataHistory.filter(d => d.tagId == tagId));
    } else {
        res.json(dataHistory);
    }
});

// WebSocket 服务器，路径 /rtls
const wss = new WebSocketServer({ server, path: '/rtls' });

wss.on('connection', (ws, req) => {
    const clientIp = req.socket.remoteAddress;
    console.log(`[WebSocket] Client connected from ${clientIp}`);

    ws.on('message', (message) => {
        try {
            const data = JSON.parse(message.toString());
            console.log(`[WebSocket] Received: ${JSON.stringify(data).substring(0, 200)}...`);

            // 保存最新数据
            if (data.tagId !== undefined) {
                latestData[data.tagId] = data;
            }

            // 保存历史（限制大小）
            dataHistory.push(data);
            if (dataHistory.length > MAX_HISTORY) {
                dataHistory.shift();
            }

            // 广播给所有前端客户端（排除发送者自身，可选）
            wss.clients.forEach((client) => {
                if (client !== ws && client.readyState === 1) {
                    client.send(JSON.stringify(data));
                }
            });
        } catch (err) {
            console.error('[WebSocket] Failed to parse message:', err.message);
        }
    });

    ws.on('close', () => {
        console.log('[WebSocket] Client disconnected');
    });

    ws.on('error', (err) => {
        console.error('[WebSocket] Error:', err.message);
    });
});

const PORT = process.env.PORT || 8080;
server.listen(PORT, () => {
    console.log(`=====================================`);
    console.log(`RTLS WebSocket Server running`);
    console.log(`HTTP  API:  http://localhost:${PORT}`);
    console.log(`WS    URL:  ws://localhost:${PORT}/rtls`);
    console.log(`=====================================`);
});
