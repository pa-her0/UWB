// -------------------------------------------------------------------------------------------------------------------
//
//  File: WebSocketClient.cpp
//
// -------------------------------------------------------------------------------------------------------------------

#include "WebSocketClient.h"
#include <QJsonDocument>
#include <QDebug>

WebSocketClient::WebSocketClient(QObject *parent)
    : QObject(parent)
    , _socket(nullptr)
    , _connected(false)
    , _reconnectIntervalMs(3000)
{
    _reconnectTimer = new QTimer(this);
    _reconnectTimer->setSingleShot(true);
    connect(_reconnectTimer, &QTimer::timeout, this, [this]() {
        if (!_connected && !_serverUrl.isEmpty()) {
            connectToServer(_serverUrl);
        }
    });
}

WebSocketClient::~WebSocketClient()
{
    disconnectFromServer();
}

void WebSocketClient::connectToServer(const QString &url)
{
    if (_socket) {
        disconnectFromServer();
    }

    _serverUrl = url;
    _socket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);

    connect(_socket, &QWebSocket::connected, this, &WebSocketClient::onConnected);
    connect(_socket, &QWebSocket::disconnected, this, &WebSocketClient::onDisconnected);
    connect(_socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, &WebSocketClient::onError);
    connect(_socket, &QWebSocket::textMessageReceived, this, &WebSocketClient::onTextMessageReceived);

    qDebug() << "[WebSocket] Connecting to" << url;
    _socket->open(QUrl(url));
}

void WebSocketClient::disconnectFromServer()
{
    if (_socket) {
        _socket->close();
        _socket->deleteLater();
        _socket = nullptr;
    }
    _connected = false;
    emit disconnected();
}

bool WebSocketClient::isConnected() const
{
    return _connected;
}

QString WebSocketClient::serverUrl() const
{
    return _serverUrl;
}

void WebSocketClient::sendPositionData(const QJsonObject &data)
{
    if (_connected && _socket && _socket->state() == QAbstractSocket::ConnectedState) {
        QJsonDocument doc(data);
        QByteArray bytes = doc.toJson(QJsonDocument::Compact);
        qint64 sent = _socket->sendTextMessage(QString(bytes));
        emit dataSent(sent);
    } else {
        // 断线时缓存数据，避免丢失
        if (_sendQueue.size() < MAX_QUEUE_SIZE) {
            _sendQueue.enqueue(data);
        } else {
            // 队列满了，丢弃最旧的数据
            _sendQueue.dequeue();
            _sendQueue.enqueue(data);
        }
    }
}

void WebSocketClient::sendTagPosition(int tagId,
                                       double x, double y, double z,
                                       double fx, double fy, double fz,
                                       const int *ranges,
                                       double pitch, double roll, double yaw,
                                       double accelX, double accelY, double accelZ,
                                       double gyroX, double gyroY, double gyroZ,
                                       double magX, double magY, double magZ,
                                       int alarm, double rxPower)
{
    QJsonObject data;
    data["type"] = "tag_position";
    data["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    data["tagId"] = tagId;

    QJsonObject pos;
    pos["x"] = QString::number(x, 'f', 3).toDouble();
    pos["y"] = QString::number(y, 'f', 3).toDouble();
    pos["z"] = QString::number(z, 'f', 3).toDouble();
    data["position"] = pos;

    QJsonObject filtered;
    filtered["x"] = QString::number(fx, 'f', 3).toDouble();
    filtered["y"] = QString::number(fy, 'f', 3).toDouble();
    filtered["z"] = QString::number(fz, 'f', 3).toDouble();
    data["filteredPosition"] = filtered;

    QJsonObject rangeObj;
    for (int i = 0; i < 8; ++i) {
        rangeObj[QString("range%1").arg(i)] = ranges[i];
    }
    data["ranges"] = rangeObj;

    QJsonObject imu;
    QJsonObject acc;  acc["x"] = accelX;  acc["y"] = accelY;  acc["z"] = accelZ;
    QJsonObject gyr;  gyr["x"] = gyroX;   gyr["y"] = gyroY;   gyr["z"] = gyroZ;
    QJsonObject mag;  mag["x"] = magX;    mag["y"] = magY;    mag["z"] = magZ;
    QJsonObject att;  att["pitch"] = pitch; att["roll"] = roll; att["yaw"] = yaw;
    imu["accel"] = acc;
    imu["gyro"] = gyr;
    imu["mag"] = mag;
    imu["attitude"] = att;
    data["imu"] = imu;

    data["alarm"] = alarm;
    data["rxPower"] = rxPower;

    sendPositionData(data);
}

void WebSocketClient::onConnected()
{
    qDebug() << "[WebSocket] Connected to" << _serverUrl;
    _connected = true;
    emit connected();

    // 发送缓存的数据
    flushSendQueue();
}

void WebSocketClient::onDisconnected()
{
    qDebug() << "[WebSocket] Disconnected from" << _serverUrl;
    _connected = false;
    emit disconnected();

    // 启动重连定时器
    if (!_reconnectTimer->isActive()) {
        _reconnectTimer->start(_reconnectIntervalMs);
    }
}

void WebSocketClient::onError(QAbstractSocket::SocketError error)
{
    QString errStr = _socket ? _socket->errorString() : QString("Unknown error");
    qDebug() << "[WebSocket] Error:" << error << errStr;
    emit connectionError(errStr);

    _connected = false;

    // 启动重连定时器
    if (!_reconnectTimer->isActive()) {
        _reconnectTimer->start(_reconnectIntervalMs);
    }
}

void WebSocketClient::onTextMessageReceived(const QString &message)
{
    // 可以处理服务器下发的控制指令，例如配置更新等
    qDebug() << "[WebSocket] Received from server:" << message;
}

void WebSocketClient::flushSendQueue()
{
    while (!_sendQueue.isEmpty() && _connected) {
        QJsonObject data = _sendQueue.dequeue();
        QJsonDocument doc(data);
        QByteArray bytes = doc.toJson(QJsonDocument::Compact);
        _socket->sendTextMessage(QString(bytes));
    }
}
