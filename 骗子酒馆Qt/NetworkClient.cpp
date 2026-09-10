#include "NetworkClient.h"

#include "Protocol.h"

#include <QAbstractSocket>
#include <QHostAddress>
#include <QTcpSocket>
#include <QUdpSocket>

#if defined(_MSC_VER)
#pragma execution_character_set("utf-8")
#endif

NetworkClient::NetworkClient(QObject *parent)
    : QObject(parent)
{
}

NetworkClient::~NetworkClient() = default;

void NetworkClient::connectToHost(const QString &host, quint16 port)
{
    if (!socket_) {
        socket_ = new QTcpSocket(this);
        connect(socket_, &QTcpSocket::connected, this, &NetworkClient::onConnected);
        connect(socket_, &QTcpSocket::disconnected, this, &NetworkClient::onDisconnected);
        connect(socket_, &QTcpSocket::readyRead, this, &NetworkClient::onReadyRead);
        connect(socket_, &QTcpSocket::errorOccurred, this, &NetworkClient::onSocketError);
    }
    buffer_.clear();
    playerId_ = -1;
    socket_->abort();
    socket_->connectToHost(host, port);
}

void NetworkClient::disconnect()
{
    if (socket_)
        socket_->disconnectFromHost();
}

bool NetworkClient::isConnected() const
{
    return socket_ && socket_->state() == QAbstractSocket::ConnectedState;
}

void NetworkClient::send(const QString &type, const QJsonObject &payload)
{
    if (!isConnected())
        return;
    socket_->write(Protocol::frame(Protocol::encode(type, payload)));
}

void NetworkClient::startDiscovery(quint16 port)
{
    discoveryPort_ = port;
    if (!udp_) {
        udp_ = new QUdpSocket(this);
        connect(udp_, &QUdpSocket::readyRead, this, &NetworkClient::onUdpReadyRead);
    }
    udp_->bind(QHostAddress::Any, port,
               QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
}

void NetworkClient::stopDiscovery()
{
    if (udp_)
        udp_->close();
}

void NetworkClient::onConnected()
{
    emit connected();
}

void NetworkClient::onDisconnected()
{
    playerId_ = -1;
    buffer_.clear();
    emit disconnected();
}

void NetworkClient::onSocketError()
{
    // 连接失败/中断统一对外表现为断开
    if (socket_ && socket_->state() != QAbstractSocket::ConnectedState)
        emit disconnected();
}

void NetworkClient::onReadyRead()
{
    if (!socket_)
        return;
    buffer_.append(socket_->readAll());
    QByteArray payload;
    int consumed = 0;
    while (Protocol::takeFrame(buffer_, &payload, &consumed)) {
        buffer_.remove(0, consumed);
        QString type;
        QJsonObject obj;
        if (!Protocol::decode(payload, &type, &obj))
            continue;
        if (type == Protocol::Msg::Welcome)
            playerId_ = obj.value(QStringLiteral("playerId")).toInt(-1);
        emit messageReceived(type, obj);
    }
}

void NetworkClient::onUdpReadyRead()
{
    if (!udp_)
        return;
    while (udp_->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(static_cast<int>(udp_->pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort = 0;
        udp_->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        QString type;
        QJsonObject obj;
        if (!Protocol::decode(datagram, &type, &obj))
            continue;
        if (type != Protocol::Msg::Announce)
            continue;
        emit roomDiscovered(
            obj.value(QStringLiteral("room")).toString(),
            sender.toString(),
            obj.value(QStringLiteral("players")).toInt(1),
            obj.value(QStringLiteral("max")).toInt(4),
            static_cast<quint16>(obj.value(QStringLiteral("port")).toInt(Protocol::DefaultPort)));
    }
}
