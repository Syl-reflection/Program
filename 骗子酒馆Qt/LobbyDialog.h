#pragma once

// 主菜单 + 房主大厅 + 加入房间对话框。
// 阶段 1 用于打通网络：本地人机 / 开房监听 / 发现并连接房间。

#include <QDialog>
#include <QJsonObject>

class QStackedWidget;
class QListWidget;
class QLineEdit;
class QLabel;
class QPushButton;
class QSpinBox;
class NetworkHost;
class NetworkClient;

class LobbyDialog : public QDialog
{
    Q_OBJECT
public:
    enum class Mode { Local, Host, Client };

    explicit LobbyDialog(QWidget *parent = nullptr);
    ~LobbyDialog() override;

    Mode mode() const { return mode_; }
    NetworkHost *takeHost();
    NetworkClient *takeClient();

private slots:
    void chooseLocal();
    void chooseHost();
    void chooseClient();
    void backToMenu();
    void startHosting();
    void refreshRooms();
    void connectToSelectedRoom();
    void connectToManualIp();
    void startGame();
    void onClientConnected(int playerId);
    void onClientDisconnected(int playerId);
    void onRoomDiscovered(const QString &room, const QString &host, int players, int max, quint16 port);
    void onClientConnectedToHost();
    void onClientDisconnectedFromHost();
    void onClientMessage(const QString &type, const QJsonObject &payload);

private:
    QWidget *buildMenuPage();
    QWidget *buildHostPage();
    QWidget *buildClientPage();
    void updateHostStatus();

    QStackedWidget *stack_ = nullptr;

    // 主菜单页
    QLabel *menuTitle_ = nullptr;

    // 房主页
    QLineEdit *roomNameEdit_ = nullptr;
    QLabel *hostStatusLabel_ = nullptr;
    QLabel *hostPlayersLabel_ = nullptr;
    QPushButton *startGameButton_ = nullptr;

    // 客户端页
    QListWidget *roomList_ = nullptr;
    QLineEdit *ipEdit_ = nullptr;
    QSpinBox *portSpin_ = nullptr;
    QLabel *clientStatusLabel_ = nullptr;

    NetworkHost *host_ = nullptr;
    NetworkClient *client_ = nullptr;
    Mode mode_ = Mode::Local;
};
