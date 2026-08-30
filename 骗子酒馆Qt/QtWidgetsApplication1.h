#pragma once

#include <QMainWindow>
#include <QString>
#include <QVector>

class QComboBox;
class QHBoxLayout;
class QLabel;
class QPushButton;
class QTextEdit;

class QtWidgetsApplication1 : public QMainWindow
{
public:
    explicit QtWidgetsApplication1(QWidget *parent = nullptr);
    ~QtWidgetsApplication1() override = default;

private:
    enum class Phase { Waiting, Decide, Play, GameOver };
    enum class TavernEvent { None, BartenderRush, DrinkBeforeCatch, FinalTable, ClosingTime };

    struct Player {
        QString name;
        QVector<int> hand;
        bool finished = false;
        int rank = 0;
    };

    struct Claim {
        bool valid = false;
        int declarer = -1;
        QVector<int> cards;
        int declaredRank = 0;
    };

    QVector<Player> players_;
    QVector<int> finishOrder_;
    QVector<int> tablePile_;
    QVector<int> recentPlayCounts_;
    QVector<int> recentRoundPileSizes_;
    Claim claim_;
    Phase phase_ = Phase::Waiting;
    TavernEvent currentEvent_ = TavernEvent::None;
    int currentPlayer_ = 0;
    int gameId_ = 0;
    int roundNumber_ = 0;
    int playsSinceLastRank_ = 0;
    bool pendingBartenderRush_ = false;
    bool pendingDrinkBeforeCatch_ = false;
    QVector<bool> selected_;

    QLabel *titleLabel_ = nullptr;
    QLabel *phaseLabel_ = nullptr;
    QLabel *eventLabel_ = nullptr;
    QLabel *opponentLabels_[3] = { nullptr, nullptr, nullptr };
    QLabel *playerInfoLabel_ = nullptr;
    QLabel *claimLabel_ = nullptr;
    QLabel *rankingLabel_ = nullptr;
    QHBoxLayout *handLayout_ = nullptr;
    QComboBox *rankCombo_ = nullptr;
    QPushButton *playButton_ = nullptr;
    QPushButton *believeButton_ = nullptr;
    QPushButton *challengeButton_ = nullptr;
    QPushButton *restartButton_ = nullptr;
    QTextEdit *logEdit_ = nullptr;

    void buildUi();
    void startGame();
    void dealHands();
    void startNewRound(int starter);
    void beginTurn(int playerIndex);
    void beginHumanTurn();
    void runAiTurn();
    void aiPlay();
    void onPlayClicked();
    void onBelieveClicked();
    void onChallengeClicked();
    void resolveChallenge(int challenger);
    void confirmFinishedPlayer(int playerIndex);
    bool finishGameIfReady();
    void recordValidPlay(int cardCount);
    void detectNextRoundEvents(int settledPileSize);
    void chooseEventForNewRound();
    void activateEvent(TavernEvent event);
    void updateEventAfterRank();
    int minimumPlayCount(int playerIndex) const;
    int maximumPlayCount(int playerIndex) const;
    bool challengeAllowed() const;
    int activePlayerCount() const;

    void updateUi();
    void rebuildHandButtons();
    void setPhase(Phase phase, const QString &text);
    void addLog(const QString &text);
    int nextActive(int from) const;
    bool claimIsTrue() const;
    QString cardName(int rank) const;
    QString playerStatusText(const Player &player) const;
    QString rankingSummary() const;
    QString eventName(TavernEvent event) const;
    QString eventDescription(TavernEvent event) const;
};
