#include "LobbyDialog.h"

#include "NetworkClient.h"
#include "NetworkHost.h"
#include "Protocol.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>

#if defined(_MSC_VER)
#pragma execution_character_set("utf-8")
#endif

LobbyDialog::LobbyDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("虎牌 · 四人唬牌"));
    setMinimumSize(460, 360);

    stack_ = new QStackedWidget(this);

    QWidget *menuPage = buildMenuPage();
    QWidget *hostPage = buildHostPage();
    QWidget *clientPage = buildClientPage();
    stack_->addWidget(menuPage);   // 0
    stack_->addWidget(hostPage);   // 1
    stack_->addWidget(clientPage); // 2
    stack_->setCurrentIndex(0);

    auto *root = new QVBoxLayout(this);
    root->addWidget(stack_);
}

LobbyDialog::~LobbyDialog() = default;

NetworkHost *LobbyDialog::takeHost()
{
    NetworkHost *host = host_;
    host_ = nullptr;
    if (host)
        host->setParent(nullptr);
    return host;
}

NetworkClient *LobbyDialog::takeClient()
{
    NetworkClient *client = client_;
    client_ = nullptr;
    if (client)
        client->setParent(nullptr);
    return client;
}

QWidget *LobbyDialog::buildMenuPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(32, 32, 32, 32);
    layout->setSpacing(14);

    auto *title = new QLabel(QStringLiteral("🐯  虎 牌  ·  四 人 唬 牌"));
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size:26px; font-weight:800; color:#e8bd6a;");
    layout->addWidget(title);

    auto *subtitle = new QLabel(QStringLiteral("选择一种开局方式"));
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setStyleSheet("color:#b89052;");
    layout->addWidget(subtitle);

    layout->addStretch();

    auto *localBtn = new QPushButton(QStringLiteral("🤖  本地人机对战（1 真人 + 3 AI）"));
    auto *hostBtn = new QPushButton(QStringLiteral("🌐  创建房间（房主）"));
    auto *joinBtn = new QPushButton(QStringLiteral("🔍  加入房间（客户端）"));
    for (QPushButton *btn : {localBtn, hostBtn, joinBtn}) {
        btn->setMinimumHeight(48);
        btn->setStyleSheet("QPushButton{font-size:16px;text-align:left;padding:0 18px;}");
        layout->addWidget(btn);
    }
    connect(localBtn, &QPushButton::clicked, this, &LobbyDialog::chooseLocal);
    connect(hostBtn, &QPushButton::clicked, this, &LobbyDialog::chooseHost);
    connect(joinBtn, &QPushButton::clicked, this, &LobbyDialog::chooseClient);

    layout->addStretch();
    return page;
}

QWidget *LobbyDialog::buildHostPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(10);

    auto *title = new QLabel(QStringLiteral("创建房间（房主）"));
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size:20px; font-weight:bold; color:#e8bd6a;");
    layout->addWidget(title);

    auto *form = new QFormLayout;
    roomNameEdit_ = new QLineEdit(QStringLiteral("虎牌房间"));
    form->addRow(QStringLiteral("房间名："), roomNameEdit_);
    layout->addLayout(form);

    auto *hostBtn = new QPushButton(QStringLiteral("创建并开始监听"));
    hostBtn->setMinimumHeight(40);
    layout->addWidget(hostBtn);
    connect(hostBtn, &QPushButton::clicked, this, &LobbyDialog::startHosting);

    hostStatusLabel_ = new QLabel(QStringLiteral("尚未创建房间"));
    hostStatusLabel_->setWordWrap(true);
    hostStatusLabel_->setStyleSheet("color:#d8b36e;");
    layout->addWidget(hostStatusLabel_);

    hostPlayersLabel_ = new QLabel(QStringLiteral("已加入玩家：0 名"));
    hostPlayersLabel_->setWordWrap(true);
    hostPlayersLabel_->setStyleSheet("color:#f1dbad;");
    layout->addWidget(hostPlayersLabel_);

    layout->addStretch();

    startGameButton_ = new QPushButton(QStringLiteral("开始游戏（等待玩家加入后点击）"));
    startGameButton_->setMinimumHeight(44);
    startGameButton_->setEnabled(false);
    layout->addWidget(startGameButton_);
    connect(startGameButton_, &QPushButton::clicked, this, &LobbyDialog::startGame);

    auto *backBtn = new QPushButton(QStringLiteral("返回主菜单"));
    layout->addWidget(backBtn);
    connect(backBtn, &QPushButton::clicked, this, &LobbyDialog::backToMenu);

    return page;
}

QWidget *LobbyDialog::buildClientPage()
{
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(10);

    auto *title = new QLabel(QStringLiteral("加入房间（客户端）"));
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size:20px; font-weight:bold; color:#e8bd6a;");
    layout->addWidget(title);

    auto *roomGroup = new QGroupBox(QStringLiteral("局域网房间"));
    auto *roomLayout = new QVBoxLayout(roomGroup);
    roomList_ = new QListWidget;
    roomList_->setMinimumHeight(120);
    roomLayout->addWidget(roomList_);
    auto *refreshBtn = new QPushButton(QStringLiteral("刷新房间列表"));
    roomLayout->addWidget(refreshBtn);
    connect(refreshBtn, &QPushButton::clicked, this, &LobbyDialog::refreshRooms);
    layout->addWidget(roomGroup);

    auto *joinBtn = new QPushButton(QStringLiteral("加入选中房间"));
    joinBtn->setMinimumHeight(40);
    layout->addWidget(joinBtn);
    connect(joinBtn, &QPushButton::clicked, this, &LobbyDialog::connectToSelectedRoom);
    connect(roomList_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *) { connectToSelectedRoom(); });

    auto *manualGroup = new QGroupBox(QStringLiteral("手动直连"));
    auto *manualLayout = new QFormLayout(manualGroup);
    ipEdit_ = new QLineEdit(QStringLiteral("127.0.0.1"));
    portSpin_ = new QSpinBox;
    portSpin_->setRange(1024, 65535);
    portSpin_->setValue(static_cast<int>(Protocol::DefaultPort));
    manualLayout->addRow(QStringLiteral("房主 IP："), ipEdit_);
    manualLayout->addRow(QStringLiteral("端口："), portSpin_);
    auto *manualConnectBtn = new QPushButton(QStringLiteral("连接"));
    manualLayout->addRow(manualConnectBtn);
    connect(manualConnectBtn, &QPushButton::clicked, this, &LobbyDialog::connectToManualIp);
    layout->addWidget(manualGroup);

    clientStatusLabel_ = new QLabel(QStringLiteral("正在搜索局域网房间……"));
    clientStatusLabel_->setWordWrap(true);
    clientStatusLabel_->setStyleSheet("color:#d8b36e;");
    layout->addWidget(clientStatusLabel_);

    auto *backBtn = new QPushButton(QStringLiteral("返回主菜单"));
    layout->addWidget(backBtn);
    connect(backBtn, &QPushButton::clicked, this, &LobbyDialog::backToMenu);

    return page;
}

void LobbyDialog::chooseLocal()
{
    mode_ = Mode::Local;
    accept();
}

void LobbyDialog::chooseHost()
{
    mode_ = Mode::Host;
    stack_->setCurrentIndex(1);
}

void LobbyDialog::chooseClient()
{
    mode_ = Mode::Client;
    if (!client_) {
        client_ = new NetworkClient(this);
        connect(client_, &NetworkClient::connected, this, &LobbyDialog::onClientConnectedToHost);
        connect(client_, &NetworkClient::disconnected, this, &LobbyDialog::onClientDisconnectedFromHost);
        connect(client_, &NetworkClient::messageReceived, this, &LobbyDialog::onClientMessage);
        connect(client_, &NetworkClient::roomDiscovered, this, &LobbyDialog::onRoomDiscovered);
        client_->startDiscovery(Protocol::DefaultPort);
    }
    stack_->setCurrentIndex(2);
}

void LobbyDialog::backToMenu()
{
    stack_->setCurrentIndex(0);
}

void LobbyDialog::startHosting()
{
    if (host_ && host_->isListening())
        return;
    if (!host_) {
        host_ = new NetworkHost(this);
        connect(host_, &NetworkHost::clientConnected, this, &LobbyDialog::onClientConnected);
        connect(host_, &NetworkHost::clientDisconnected, this, &LobbyDialog::onClientDisconnected);
    }
    QString name = roomNameEdit_->text().trimmed();
    if (name.isEmpty())
        name = QStringLiteral("虎牌房间");
    host_->setRoomName(name);
    if (host_->startListening(Protocol::DefaultPort)) {
        hostStatusLabel_->setText(QStringLiteral("房间已创建，正在监听端口 %1，等待玩家加入……")
                                      .arg(host_->port()));
        startGameButton_->setEnabled(true);
    } else {
        hostStatusLabel_->setText(QStringLiteral("监听失败：端口可能被占用。"));
    }
    updateHostStatus();
}

void LobbyDialog::updateHostStatus()
{
    if (!host_)
        return;
    const int clients = host_->clientCount();
    hostPlayersLabel_->setText(QStringLiteral("已加入玩家：%1 名（含房主共 %2 人，最多 4 人）")
                                   .arg(clients)
                                   .arg(clients + 1));
}

void LobbyDialog::onClientConnected(int playerId)
{
    Q_UNUSED(playerId);
    updateHostStatus();
}

void LobbyDialog::onClientDisconnected(int playerId)
{
    Q_UNUSED(playerId);
    updateHostStatus();
}

void LobbyDialog::refreshRooms()
{
    roomList_->clear();
    clientStatusLabel_->setText(QStringLiteral("正在搜索局域网房间……"));
}

void LobbyDialog::onRoomDiscovered(const QString &room, const QString &host, int players, int max, quint16 port)
{
    const QString key = room + QStringLiteral("@") + host + QStringLiteral(":") + QString::number(port);
    for (int i = 0; i < roomList_->count(); ++i) {
        QListWidgetItem *item = roomList_->item(i);
        if (item->data(Qt::UserRole + 2).toString() == key) {
            item->setText(QStringLiteral("%1　（%2/%3 人）　%4:%5").arg(room).arg(players).arg(max).arg(host).arg(port));
            return;
        }
    }
    auto *item = new QListWidgetItem(QStringLiteral("%1　（%2/%3 人）　%4:%5").arg(room).arg(players).arg(max).arg(host).arg(port));
    item->setData(Qt::UserRole, host);
    item->setData(Qt::UserRole + 1, static_cast<int>(port));
    item->setData(Qt::UserRole + 2, key);
    roomList_->addItem(item);
}

void LobbyDialog::connectToSelectedRoom()
{
    QListWidgetItem *item = roomList_->currentItem();
    if (!item) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请先在列表中选择一个房间。"));
        return;
    }
    const QString host = item->data(Qt::UserRole).toString();
    const quint16 port = static_cast<quint16>(item->data(Qt::UserRole + 1).toInt());
    if (!client_) {
        chooseClient();
    }
    client_->connectToHost(host, port);
    clientStatusLabel_->setText(QStringLiteral("正在连接 %1:%2 ……").arg(host).arg(port));
}

void LobbyDialog::connectToManualIp()
{
    const QString host = ipEdit_->text().trimmed();
    if (host.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("提示"), QStringLiteral("请输入房主 IP。"));
        return;
    }
    if (!client_) {
        chooseClient();
    }
    client_->connectToHost(host, static_cast<quint16>(portSpin_->value()));
    clientStatusLabel_->setText(QStringLiteral("正在连接 %1:%2 ……").arg(host).arg(portSpin_->value()));
}

void LobbyDialog::onClientConnectedToHost()
{
    clientStatusLabel_->setText(QStringLiteral("已连接，等待房主分配席位……"));
}

void LobbyDialog::onClientDisconnectedFromHost()
{
    clientStatusLabel_->setText(QStringLiteral("已断开连接。"));
}

void LobbyDialog::onClientMessage(const QString &type, const QJsonObject &payload)
{
    if (type == Protocol::Msg::Welcome) {
        const int pid = payload.value(QStringLiteral("playerId")).toInt(-1);
        clientStatusLabel_->setText(QStringLiteral("已加入房间，你是玩家%1。等待房主开始游戏……").arg(pid + 1));
    } else if (type == Protocol::Msg::Start) {
        accept(); // 房主已开局，进入对局界面
    } else if (type == Protocol::Msg::Error) {
        clientStatusLabel_->setText(QStringLiteral("错误：%1").arg(payload.value(QStringLiteral("message")).toString()));
    }
}

void LobbyDialog::startGame()
{
    accept();
}
