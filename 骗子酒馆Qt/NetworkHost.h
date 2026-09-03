#pragma once

// 房主网络层：QTcpServer 监听 + 客户端连接管理 + UDP 房间公告。
// 房主自身为 playerId = 0；客户端按连接顺序分配 playerId = 1..3。

#include <QByteArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVector>

class QTcpServer;
class QTcpSocket;
class QUdpSocket;
class QTimer;

class NetworkHost : public QObject
{
    Q_OBJECT
public:
    explicit NetworkHost(QObject *parent = nullptr);
    ~NetworkHost() override;

    bool startListening(quint16 port);
    void stop();

    QString roomName() const { return roomName_; }
    void setRoomName(const QString &name) { roomName_ = name; }

    bool isListening() const;
    quint16 port() const { return port_; }
    int clientCount() const;
    bool isConnected(int playerId) const;

    // 发送：broadcast 发给所有客户端；sendTo 发给指定 playerId（1..3）
    void broadcast(const QString &type, const QJsonObject &payload = {});
    void sendTo(int playerId, const QString &type, const QJsonObject &payload = {});
    void disconnectClient(int playerId);

signals:
    void clientConnected(int playerId);
    void clientDisconnected(int playerId);
    void messageReceived(int playerId, const QString &type, const QJsonObject &payload);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();
    void announce();

private:
    static constexpr int MaxClients = 3; // 房主占 1 席，最多再进 3 个客户端

    void writeMessage(QTcpSocket *socket, const QByteArray &json);

    QTcpServer *server_ = nullptr;
    QVector<QTcpSocket *> clients_;   // 下标 = playerId - 1
    QVector<QByteArray> buffers_;     // 与 clients_ 对应的接收缓冲
    QUdpSocket *udp_ = nullptr;
    QTimer *announceTimer_ = nullptr;
    QString roomName_;
    quint16 port_ = 0;
};
