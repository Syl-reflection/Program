#include "NetworkHost.h"

#include "Protocol.h"

#include <QAbstractSocket>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QUdpSocket>

#if defined(_MSC_VER)
#pragma execution_character_set("utf-8")
#endif

NetworkHost::NetworkHost(QObject *parent)
    : QObject(parent)
{
}

NetworkHost::~NetworkHost() = default;

bool NetworkHost::startListening(quint16 port)
{
    port_ = port;
    clients_.fill(nullptr, MaxClients);
    buffers_.fill(QByteArray(), MaxClients);

    server_ = new QTcpServer(this);
    connect(server_, &QTcpServer::newConnection, this, &NetworkHost::onNewConnection);
    if (!server_->listen(QHostAddress::Any, port_))
        return false;

    // UDP 广播：与 TCP 端口同号（协议不同，互不冲突），ShareAddress 便于本机多实例测试。
    udp_ = new QUdpSocket(this);
    udp_->bind(QHostAddress::Any, port_,
               QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);

    announceTimer_ = new QTimer(this);
    connect(announceTimer_, &QTimer::timeout, this, &NetworkHost::announce);
    announceTimer_->start(1000);
    return true;
}

void NetworkHost::stop()
{
    if (announceTimer_)
        announceTimer_->stop();
    if (udp_)
        udp_->close();
    if (server_)
        server_->close();
    for (QTcpSocket *socket : clients_) {
        if (socket) {
            socket->disconnectFromHost();
            socket->deleteLater();
        }
    }
    clients_.clear();
    buffers_.clear();
}

bool NetworkHost::isListening() const
{
    return server_ && server_->isListening();
}

bool NetworkHost::isConnected(int playerId) const
{
    const int idx = playerId - 1;
    if (idx < 0 || idx >= clients_.size())
        return false;
    const QTcpSocket *socket = clients_[idx];
    return socket && socket->state() == QAbstractSocket::ConnectedState;
}

int NetworkHost::clientCount() const
{
    int count = 0;
    for (QTcpSocket *socket : clients_)
        if (socket && socket->state() == QAbstractSocket::ConnectedState)
            ++count;
    return count;
}

void NetworkHost::broadcast(const QString &type, const QJsonObject &payload)
{
    const QByteArray json = Protocol::encode(type, payload);
    for (QTcpSocket *socket : clients_) {
        if (socket && socket->state() == QAbstractSocket::ConnectedState)
            writeMessage(socket, json);
    }
}

void NetworkHost::sendTo(int playerId, const QString &type, const QJsonObject &payload)
{
    const int idx = playerId - 1;
    if (idx < 0 || idx >= clients_.size())
        return;
    if (QTcpSocket *socket = clients_[idx])
        writeMessage(socket, Protocol::encode(type, payload));
}

void NetworkHost::disconnectClient(int playerId)
{
    const int idx = playerId - 1;
    if (idx < 0 || idx >= clients_.size())
        return;
    if (QTcpSocket *socket = clients_[idx]) {
        socket->disconnectFromHost();
        if (socket->state() != QAbstractSocket::UnconnectedState)
            socket->waitForDisconnected(500);
    }
}

void NetworkHost::writeMessage(QTcpSocket *socket, const QByteArray &json)
{
    if (!socket || socket->state() != QAbstractSocket::ConnectedState)
        return;
    socket->write(Protocol::frame(json));
}

void NetworkHost::onNewConnection()
{
    while (server_ && server_->hasPendingConnections()) {
        QTcpSocket *socket = server_->nextPendingConnection();
        int slot = -1;
        for (int i = 0; i < clients_.size(); ++i) {
            if (!clients_[i]) {
                slot = i;
                break;
            }
        }
        if (slot < 0) {
            socket->disconnectFromHost();
            socket->deleteLater();
            continue;
        }
        clients_[slot] = socket;
        buffers_[slot].clear();
        const int playerId = slot + 1;
        connect(socket, &QTcpSocket::readyRead, this, &NetworkHost::onReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &NetworkHost::onDisconnected);
        emit clientConnected(playerId);

        // 分配席位：告知客户端其 playerId。
        QJsonObject welcome;
        welcome.insert(QStringLiteral("playerId"), playerId);
        welcome.insert(QStringLiteral("seat"), Protocol::seatKindToString(Protocol::SeatKind::Human));
        writeMessage(socket, Protocol::encode(Protocol::Msg::Welcome, welcome));
    }
}

void NetworkHost::onReadyRead()
{
    auto *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket)
        return;
    const int idx = clients_.indexOf(socket);
    if (idx < 0)
        return;

    buffers_[idx].append(socket->readAll());
    QByteArray payload;
    int consumed = 0;
    while (Protocol::takeFrame(buffers_[idx], &payload, &consumed)) {
        buffers_[idx].remove(0, consumed);
        QString type;
        QJsonObject obj;
        if (Protocol::decode(payload, &type, &obj))
            emit messageReceived(idx + 1, type, obj);
    }
}

void NetworkHost::onDisconnected()
{
    auto *socket = qobject_cast<QTcpSocket *>(sender());
    if (!socket)
        return;
    const int idx = clients_.indexOf(socket);
    if (idx < 0)
        return;
    clients_[idx] = nullptr;
    buffers_[idx].clear();
    socket->deleteLater();
    emit clientDisconnected(idx + 1);
}

void NetworkHost::announce()
{
    if (!udp_)
        return;
    QJsonObject payload;
    payload.insert(QStringLiteral("room"), roomName_);
    payload.insert(QStringLiteral("players"), clientCount() + 1);
    payload.insert(QStringLiteral("max"), 4);
    payload.insert(QStringLiteral("port"), static_cast<int>(port_));
    const QByteArray datagram = Protocol::encode(Protocol::Msg::Announce, payload);
    udp_->writeDatagram(datagram, QHostAddress::Broadcast, port_);
}
