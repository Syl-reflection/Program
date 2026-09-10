#pragma once

// 客户端网络层：连接房主、收发消息、UDP 发现局域网房间。

#include <QByteArray>
#include <QJsonObject>
#include <QObject>
#include <QString>

class QTcpSocket;
class QUdpSocket;

class NetworkClient : public QObject
{
    Q_OBJECT
public:
    explicit NetworkClient(QObject *parent = nullptr);
    ~NetworkClient() override;

    void connectToHost(const QString &host, quint16 port);
    void disconnect();

    bool isConnected() const;
    int localPlayerId() const { return playerId_; }
    void send(const QString &type, const QJsonObject &payload = {});

    // UDP 房间发现（发现用同一个端口号收公告）
    void startDiscovery(quint16 port);
    void stopDiscovery();

signals:
    void connected();
    void disconnected();
    void messageReceived(const QString &type, const QJsonObject &payload);
    void roomDiscovered(const QString &roomName, const QString &hostAddress,
                        int players, int max, quint16 port);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onSocketError();
    void onUdpReadyRead();

private:
    QTcpSocket *socket_ = nullptr;
    QUdpSocket *udp_ = nullptr;
    QByteArray buffer_;
    int playerId_ = -1;
    quint16 discoveryPort_ = 0;
};
