#include "QtWidgetsApplication1.h"
#include "LobbyDialog.h"
#include "NetworkClient.h"
#include "NetworkHost.h"
#include <QApplication>

#if defined(_MSC_VER)
#pragma execution_character_set("utf-8")
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("虎牌：四人唬牌 · 秘密任务与动态事件排名赛"));

    LobbyDialog lobby;
    lobby.exec();

    switch (lobby.mode()) {
    case LobbyDialog::Mode::Local: {
        // 本地人机对战：保留原有 1 真人 + 3 AI 流程。
        QtWidgetsApplication1 window;
        window.show();
        return app.exec();
    }
    case LobbyDialog::Mode::Host: {
        NetworkHost *host = lobby.takeHost();
        QtWidgetsApplication1 window(QtWidgetsApplication1::GameMode::Host, 0, host, nullptr);
        window.show();
        return app.exec();
    }
    case LobbyDialog::Mode::Client: {
        NetworkClient *client = lobby.takeClient();
        const int pid = client ? client->localPlayerId() : 0;
        QtWidgetsApplication1 window(QtWidgetsApplication1::GameMode::Client, pid, nullptr, client);
        window.show();
        return app.exec();
    }
    }
    return 0;
}
