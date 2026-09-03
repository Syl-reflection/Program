#pragma once

#include <QMainWindow>
#include <QString>
#include <QVector>
#include "ui_QtWidgetsApplication1.h"

class QComboBox;
class QFrame;
class QGraphicsDropShadowEffect;
class QGridLayout;
class QLabel;
class QProgressBar;
class QPushButton;
class QTextEdit;
class QTimer;
class QVariantAnimation;

class QtWidgetsApplication1 : public QMainWindow
{
public:
    explicit QtWidgetsApplication1(QWidget *parent = nullptr);
    ~QtWidgetsApplication1() override = default;

private:
    Ui::QtWidgetsApplication1Class ui;

    enum class Phase { Waiting, Decide, Play, GameOver };
    enum class TavernEvent { None, BartenderRush, DrinkBeforeCatch, FinalTable, ClosingTime };
    enum class SecretTask { None, DeceiveAtRisk, BaitChallenge, BoldChallenge, DesperateBluff };
    enum class SecretReward { None, PeekCard, RankScout, PileScout, GamblerAllIn };

    struct Player {
        QString name;
        QVector<int> hand;
        bool finished = false;
        int rank = 0;
        SecretTask task = SecretTask::None;
        SecretReward reward = SecretReward::None;
        int completedTasks = 0;
        int rewardAwardRound = 0;
        int pendingRewards = 0;
        bool gamblingBan = false;
    };

    struct Claim {
        bool valid = false;
        int declarer = -1;
        QVector<int> cards;
        int declaredRank = 0;
        int pileSizeBeforePlay = 0;
        int declarerHandSizeBeforePlay = 0;
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
    int allInChallenger_ = -1;
    bool pendingBartenderRush_ = false;
    bool pendingDrinkBeforeCatch_ = false;
    bool tableActionImportant_ = false;
    QString tableActionText_;
    QString aiProgressStageText_;
    QVector<bool> selected_;
    int aiProgressDurationMs_ = 0;
    int aiProgressElapsedMs_ = 0;

    QLabel *titleLabel_ = nullptr;
    QLabel *phaseLabel_ = nullptr;
    QLabel *eventLabel_ = nullptr;
    QLabel *actionLabel_ = nullptr;
    QLabel *opponentLabels_[3] = { nullptr, nullptr, nullptr };
    QLabel *playerInfoLabel_ = nullptr;
    QLabel *taskLabel_ = nullptr;
    QLabel *rewardLabel_ = nullptr;
    QLabel *claimLabel_ = nullptr;
    QLabel *rankingLabel_ = nullptr;
    QLabel *tableCardLabels_[3] = { nullptr, nullptr, nullptr };
    QLabel *tableCardStatusLabel_ = nullptr;
    QFrame *playerSeatFrames_[4] = { nullptr, nullptr, nullptr, nullptr };
    QLabel *playerAvatarLabels_[4] = { nullptr, nullptr, nullptr, nullptr };
    QLabel *playerCountLabels_[4] = { nullptr, nullptr, nullptr, nullptr };
    QLabel *playerRankBadges_[4] = { nullptr, nullptr, nullptr, nullptr };
    QGraphicsDropShadowEffect *playerGlowEffects_[4] = { nullptr, nullptr, nullptr, nullptr };
    QFrame *decisionActionPanel_ = nullptr;
    QProgressBar *aiProgressBar_ = nullptr;
    QGridLayout *handLayout_ = nullptr;
    QComboBox *rankCombo_ = nullptr;
    QPushButton *playButton_ = nullptr;
    QPushButton *believeButton_ = nullptr;
    QPushButton *challengeButton_ = nullptr;
    QPushButton *rewardButton_ = nullptr;
    QPushButton *restartButton_ = nullptr;
    QTextEdit *logEdit_ = nullptr;
    QTimer *aiProgressTimer_ = nullptr;
    QVariantAnimation *turnGlowAnimation_ = nullptr;
    int glowingPlayer_ = -1;
    int presentedEventCode_ = -1;

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
    void onRewardClicked();
    void resolveChallenge(int challenger, bool allIn = false);
    void confirmFinishedPlayer(int playerIndex);
    bool finishGameIfReady();
    void recordValidPlay(int cardCount);
    void detectNextRoundEvents(int settledPileSize);
    void chooseEventForNewRound();
    void activateEvent(TavernEvent event);
    void updateEventAfterRank();
    int minimumPlayCount(int playerIndex) const;
    int maximumPlayCount(int playerIndex) const;
    int truthfulPlayCapacity(int playerIndex) const;
    bool challengeAllowed(int challenger) const;
    int activePlayerCount() const;

    void assignRandomTask(int playerIndex);
    void completeSecretTask(int playerIndex);
    void checkBeliefTask(int declarer);
    void checkChallengeTasks(int challenger, bool truthful, int settledPileSize);
    void grantRandomReward(int playerIndex);
    void grantPendingReward(int playerIndex);
    void consumeReward(int playerIndex);
    void expireRewardsForNewRound();
    void aiUseInformationReward(int playerIndex, int &suspicion);
    bool rewardUsable(int playerIndex) const;
    void handleAllInResult(int challenger, bool success);

    void updateUi();
    void rebuildHandButtons();
    void showTableCards(const QVector<int> &cards, bool revealed);
    void clearTableCards();
    void showTableAction(const QString &text, bool important = false);
    void startAiProgress(int durationMs, const QString &stageText);
    void stopAiProgress();
    void showRankPopup(int playerIndex);
    void setPhase(Phase phase, const QString &text);
    void addLog(const QString &text);
    int nextActive(int from) const;
    bool claimIsTrue() const;
    bool cardsMatchClaim(const QVector<int> &cards, int declaredRank) const;
    bool isLegalPlaySelection(int playerIndex, const QVector<int> &indices, int declaredRank) const;
    QString cardName(int rank) const;
    QString playerStatusText(const Player &player) const;
    QString rankingSummary() const;
    QString eventName(TavernEvent event) const;
    QString eventDescription(TavernEvent event) const;
    QString taskName(SecretTask task) const;
    QString taskDescription(SecretTask task) const;
    QString rewardName(SecretReward reward) const;
    QString rewardDescription(SecretReward reward) const;
};
