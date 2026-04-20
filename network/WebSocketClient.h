// -------------------------------------------------------------------------------------------------------------------
//
//  File: WebSocketClient.h
//
//  WebSocket client for sending RTLS data to remote web server
//
// -------------------------------------------------------------------------------------------------------------------

#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include <QObject>
#include <QWebSocket>
#include <QJsonObject>
#include <QTimer>
#include <QQueue>

class WebSocketClient : public QObject
{
    Q_OBJECT
public:
    explicit WebSocketClient(QObject *parent = nullptr);
    ~WebSocketClient();

    // 连接到 WebSocket 服务器，url 例如: "ws://192.168.1.100:8080/rtls"
    void connectToServer(const QString &url);
    void disconnectFromServer();

    bool isConnected() const;
    QString serverUrl() const;

    // 发送定位数据（会自动在连接成功后发送队列中的数据）
    void sendPositionData(const QJsonObject &data);

    // 快速发送，内部自动封装成统一格式
    void sendTagPosition(int tagId,
                         double x, double y, double z,
                         double fx, double fy, double fz,
                         const int *ranges,
                         double pitch, double roll, double yaw,
                         double accelX, double accelY, double accelZ,
                         double gyroX, double gyroY, double gyroZ,
                         double magX, double magY, double magZ,
                         int alarm, double rxPower);

signals:
    void connected();
    void disconnected();
    void connectionError(const QString &errorString);
    void dataSent(int bytesSent);

private slots:
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError error);
    void onTextMessageReceived(const QString &message);
    void flushSendQueue();

private:
    QWebSocket *_socket;
    QString _serverUrl;
    bool _connected;

    // 发送队列（断线时缓存数据）
    QQueue<QJsonObject> _sendQueue;
    static const int MAX_QUEUE_SIZE = 200;

    QTimer *_reconnectTimer;
    int _reconnectIntervalMs;
};

#endif // WEBSOCKETCLIENT_H
