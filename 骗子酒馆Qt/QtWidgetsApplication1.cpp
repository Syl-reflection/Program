#include "QtWidgetsApplication1.h"

#include "NetworkClient.h"
#include "NetworkHost.h"
#include "Protocol.h"

#include <QApplication>
#include <QColor>
#include <QComboBox>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGridLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QRandomGenerator>
#include <QScrollBar>
#include <QStringList>
#include <QTextEdit>
#include <QTimer>
#include <QVariantAnimation>
#include <algorithm>
#include <functional>

#if defined(_MSC_VER)
#pragma execution_character_set("utf-8")
#endif

namespace {
constexpr int PlayerCount = 4;
constexpr int RankCount = 7;
constexpr int CardsPerRank = 6;
constexpr int JokerCount = 2;
constexpr int JokerRank = RankCount;
constexpr int DeckSize = RankCount * CardsPerRank + JokerCount;
constexpr int InitialHandSize = DeckSize / PlayerCount;
constexpr int MaxPlayCards = 3;
constexpr int HandColumns = 14;

constexpr int DeceivePileRequirement = 10;
constexpr int BaitPileRequirement = 10;
constexpr int BoldPileRequirement = 14;
constexpr int DesperateHandMin = 7;
constexpr int DesperateHandMax = 10;

constexpr int ConservativePlayWindow = 8;
constexpr int ConservativePlayNeed = 6;
constexpr int ConservativePlayMaxCards = 2;
constexpr int SmallPileRounds = 3;
constexpr int SmallPileMax = 7;
constexpr int DrinkChallengePile = 8;
constexpr int ClosingTimePlays = 30;
constexpr int AiThinkDelayMs = 3200;
constexpr int AiThinkSpectatorDelayMs = 4200;
constexpr int AiOpeningPlayDelayMs = 1400;
constexpr int AiOpeningPlaySpectatorDelayMs = 1900;
constexpr int AiDecisionDisplayMs = 3200;
constexpr int AiDecisionSpectatorDisplayMs = 4200;
constexpr int AiChallengeDisplayMs = 3200;
constexpr int AiChallengeSpectatorDisplayMs = 4200;
constexpr int AiSettlementDisplayMs = 3800;
constexpr int AiSettlementSpectatorDisplayMs = 4800;

QStringList rankChoices()
{
    return { QStringLiteral("8"), QStringLiteral("9"), QStringLiteral("10"),
             QStringLiteral("J"), QStringLiteral("Q"), QStringLiteral("K"),
             QStringLiteral("A"), QStringLiteral("Joker") };
}
}

QtWidgetsApplication1::QtWidgetsApplication1(QWidget *parent)
    : QtWidgetsApplication1(GameMode::LocalVsAi, 0, nullptr, nullptr, parent)
{
}

QtWidgetsApplication1::QtWidgetsApplication1(GameMode mode, int localPlayerId,
                                             NetworkHost *host, NetworkClient *client,
                                             QWidget *parent)
    : QMainWindow(parent)
    , mode_(mode)
    , localPlayerId_(localPlayerId)
    , host_(host)
    , client_(client)
{
    aiSeat_.fill(false, PlayerCount);
    buildUi();

    if (mode_ == GameMode::Client && client_) {
        connect(client_, &NetworkClient::messageReceived, this, &QtWidgetsApplication1::onClientMessage);
        connect(client_, &NetworkClient::disconnected, this, [this] {
            setPhase(Phase::Waiting, QStringLiteral("与房主的连接已断开。"));
        });
        client_->send(Protocol::Msg::Sync);
        setPhase(Phase::Waiting, QStringLiteral("已加入房间，等待房主开始游戏……"));
        updateUi();
        return;
    }

    if (mode_ == GameMode::Host && host_) {
        connect(host_, &NetworkHost::messageReceived, this, &QtWidgetsApplication1::onHostMessage);
    }
    QTimer::singleShot(50, this, [this] { startGame(); });
}

void QtWidgetsApplication1::buildUi()
{
    ui.setupUi(this);

    titleLabel_ = ui.titleLabel;
    phaseLabel_ = ui.phaseLabel;
    eventLabel_ = ui.eventLabel;
    actionLabel_ = ui.actionLabel;
    opponentLabels_[0] = ui.opponentLabel1;
    opponentLabels_[1] = ui.opponentLabel2;
    opponentLabels_[2] = ui.opponentLabel3;
    playerInfoLabel_ = ui.playerInfoLabel;
    taskLabel_ = ui.taskLabel;
    rewardLabel_ = ui.rewardLabel;
    claimLabel_ = ui.claimLabel;
    rankingLabel_ = ui.rankingLabel;
    tableCardLabels_[0] = ui.tableCard1;
    tableCardLabels_[1] = ui.tableCard2;
    tableCardLabels_[2] = ui.tableCard3;
    tableCardStatusLabel_ = ui.tableCardStatusLabel;
    playerSeatFrames_[0] = ui.humanPanel;
    playerSeatFrames_[1] = ui.opponentSeat1;
    playerSeatFrames_[2] = ui.opponentSeat2;
    playerSeatFrames_[3] = ui.opponentSeat3;
    playerAvatarLabels_[0] = ui.humanAvatarLabel;
    playerAvatarLabels_[1] = ui.opponentAvatar1;
    playerAvatarLabels_[2] = ui.opponentAvatar2;
    playerAvatarLabels_[3] = ui.opponentAvatar3;
    playerCountLabels_[0] = ui.humanCountLabel;
    playerCountLabels_[1] = ui.opponentCount1;
    playerCountLabels_[2] = ui.opponentCount2;
    playerCountLabels_[3] = ui.opponentCount3;
    playerRankBadges_[0] = ui.humanRankBadge;
    playerRankBadges_[1] = ui.opponentRankBadge1;
    playerRankBadges_[2] = ui.opponentRankBadge2;
    playerRankBadges_[3] = ui.opponentRankBadge3;
    playerNameLabels_[0] = playerInfoLabel_;
    playerNameLabels_[1] = opponentLabels_[0];
    playerNameLabels_[2] = opponentLabels_[1];
    playerNameLabels_[3] = opponentLabels_[2];
    decisionActionPanel_ = ui.decisionActionPanel;
    aiProgressBar_ = ui.aiProgressBar;
    handLayout_ = ui.handGridLayout;
    rankCombo_ = ui.rankCombo;
    playButton_ = ui.playButton;
    believeButton_ = ui.believeButton;
    challengeButton_ = ui.challengeButton;
    rewardButton_ = ui.rewardButton;
    restartButton_ = ui.restartButton;
    logEdit_ = ui.logEdit;

    handLayout_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    aiProgressBar_->hide();
    clearTableCards();

    for (int i = 0; i < PlayerCount; ++i) {
        playerGlowEffects_[i] = new QGraphicsDropShadowEffect(this);
        playerGlowEffects_[i]->setOffset(0, 0);
        playerGlowEffects_[i]->setBlurRadius(0);
        playerGlowEffects_[i]->setColor(QColor(240, 178, 62, 0));
        playerAvatarLabels_[i]->setGraphicsEffect(playerGlowEffects_[i]);
    }
    turnGlowAnimation_ = new QVariantAnimation(this);
    turnGlowAnimation_->setDuration(800);
    turnGlowAnimation_->setStartValue(0.0);
    turnGlowAnimation_->setKeyValueAt(0.5, 1.0);
    turnGlowAnimation_->setEndValue(0.0);
    turnGlowAnimation_->setLoopCount(-1);
    connect(turnGlowAnimation_, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        const qreal pulse = value.toReal();
        for (int i = 0; i < PlayerCount; ++i) {
            if (!playerGlowEffects_[i])
                continue;
            const bool glowing = i == glowingPlayer_;
            playerGlowEffects_[i]->setBlurRadius(glowing ? 25.0 + pulse * 13.0 : 0.0);
            playerGlowEffects_[i]->setColor(glowing
                ? QColor(246, 187, 67, 175 + static_cast<int>(pulse * 55.0))
                : QColor(0, 0, 0, 0));
        }
    });
    turnGlowAnimation_->start();

    aiProgressTimer_ = new QTimer(this);
    aiProgressTimer_->setInterval(100);
    connect(aiProgressTimer_, &QTimer::timeout, this, [this] {
        if (!aiProgressBar_ || aiProgressDurationMs_ <= 0) {
            stopAiProgress();
            return;
        }
        aiProgressElapsedMs_ = std::min(aiProgressElapsedMs_ + 100, aiProgressDurationMs_);
        aiProgressBar_->setValue(aiProgressElapsedMs_);
        const int remainingMs = std::max(0, aiProgressDurationMs_ - aiProgressElapsedMs_);
        aiProgressBar_->setFormat(QStringLiteral("⏳ %1　剩余 %2 秒")
            .arg(aiProgressStageText_)
            .arg(remainingMs / 1000.0, 0, 'f', 1));
        if (aiProgressElapsedMs_ >= aiProgressDurationMs_)
            aiProgressTimer_->stop();
    });

    connect(playButton_, &QPushButton::clicked, this, [this] { onPlayClicked(); });
    connect(believeButton_, &QPushButton::clicked, this, [this] { onBelieveClicked(); });
    connect(challengeButton_, &QPushButton::clicked, this, [this] { onChallengeClicked(); });
    connect(rewardButton_, &QPushButton::clicked, this, [this] { onRewardClicked(); });
    connect(restartButton_, &QPushButton::clicked, this, [this] { startGame(); });
    connect(ui.exitButton, &QPushButton::clicked, this, &QWidget::close);
    connect(ui.rulesButton, &QPushButton::clicked, this, [this] {
        QMessageBox::information(this, QStringLiteral("规则说明"),
            QStringLiteral("目标：尽快出完手牌，按出完顺序获得第 1–4 名。\n\n"
                           "出牌：选择 1–3 张牌，再声明它们是同一种点数。\n"
                           "判断：下家可以相信或质疑；质疑后根据实际牌面判定谁收走中央牌堆。\n"
                           "机制：每轮可能触发动态酒馆事件，完成秘密任务可获得一次性奖励。"));
    });
}

void QtWidgetsApplication1::startGame()
{
    ++gameId_;
    stopAiProgress();
    players_.clear();
    players_.resize(PlayerCount);
    // 名字保持中性（不含“你”），由 updateUi 按 localPlayerId_ 追加“（你）”。
    players_[0].name = QStringLiteral("玩家1");
    players_[1].name = QStringLiteral("玩家2");
    players_[2].name = QStringLiteral("玩家3");
    players_[3].name = QStringLiteral("玩家4");
    for (Player &player : players_) {
        player.hand.clear();
        player.finished = false;
        player.rank = 0;
        player.task = SecretTask::None;
        player.reward = SecretReward::None;
        player.completedTasks = 0;
        player.rewardAwardRound = 0;
        player.pendingRewards = 0;
        player.gamblingBan = false;
    }

    // Host 模式：没有客户端连接的席位由 AI 补位。
    aiSeat_.fill(false, PlayerCount);
    if (mode_ == GameMode::Host && host_) {
        for (int i = 1; i < PlayerCount; ++i)
            aiSeat_[i] = !host_->isConnected(i);
    }

    finishOrder_.clear();
    tablePile_.clear();
    recentPlayCounts_.clear();
    recentRoundPileSizes_.clear();
    claim_ = Claim{};
    claimRevealed_ = false;
    phase_ = Phase::Waiting;
    currentEvent_ = TavernEvent::None;
    currentPlayer_ = localPlayerId_;
    roundNumber_ = 0;
    playsSinceLastRank_ = 0;
    allInChallenger_ = -1;
    pendingBartenderRush_ = false;
    pendingDrinkBeforeCatch_ = false;
    tableActionImportant_ = false;
    tableActionText_.clear();
    presentedEventCode_ = -1;
    clearTableCards();
    dealHands();
    for (int i = 0; i < players_.size(); ++i)
        assignRandomTask(i);
    selected_.fill(false, players_[localPlayerId_].hand.size());

    logEdit_->clear();
    addLog(QStringLiteral("四人对局开始：所有玩家的开局条件与操作规则完全相同。"));
    addLog(QStringLiteral("牌库包含 8、9、10、J、Q、K、A 各 %1 张及 %2 张 Joker，共 %3 张；Joker 可充当任意声明牌面。")
               .arg(CardsPerRank).arg(JokerCount).arg(DeckSize));
    addLog(QStringLiteral("每人获得 %1 张牌。率先出完手牌者获得第 1 名，其余玩家继续争夺后续名次。")
               .arg(InitialHandSize));
    addLog(QStringLiteral("选择相信后，盖牌会留在中央牌池；质疑判断失败者收走整个中央牌池。"));
    addLog(QStringLiteral("系统会在牌局节奏变慢时自动触发公开的酒馆事件。"));
    addLog(QStringLiteral("【你的秘密任务】%1：%2").arg(taskName(players_[localPlayerId_].task), taskDescription(players_[localPlayerId_].task)));
    showTableAction(QStringLiteral("四人对局已经开始，等待第一位玩家行动。"));
    startNewRound(localPlayerId_);

    if (mode_ == GameMode::Host && host_) {
        host_->broadcast(Protocol::Msg::Start, {});
        broadcastState();
    }
}

void QtWidgetsApplication1::dealHands()
{
    QVector<int> deck;
    deck.reserve(DeckSize);
    for (int rank = 0; rank < RankCount; ++rank)
        for (int i = 0; i < CardsPerRank; ++i)
            deck.append(rank);
    for (int i = 0; i < JokerCount; ++i)
        deck.append(JokerRank);
    std::shuffle(deck.begin(), deck.end(), *QRandomGenerator::global());

    int position = 0;
    for (Player &player : players_) {
        const int take = std::min(InitialHandSize, static_cast<int>(deck.size()) - position);
        for (int i = 0; i < take; ++i)
            player.hand.append(deck[position++]);
    }
}

void QtWidgetsApplication1::startNewRound(int starter)
{
    if (phase_ == Phase::GameOver)
        return;

    ++roundNumber_;
    expireRewardsForNewRound();
    claim_ = Claim{};
    tablePile_.clear();
    clearTableCards();
    chooseEventForNewRound();
    addLog(QStringLiteral("—— 第 %1 轮开始 ——").arg(roundNumber_));
    const int actualStarter = players_[starter].finished ? nextActive(starter) : starter;
    showTableAction(QStringLiteral("第 %1 轮开始，由 %2 先行动。")
                        .arg(roundNumber_).arg(players_[actualStarter].name));
    beginTurn(actualStarter);
}

void QtWidgetsApplication1::beginTurn(int playerIndex)
{
    if (phase_ == Phase::GameOver || players_.isEmpty())
        return;
    if (playerIndex < 0 || playerIndex >= players_.size())
        playerIndex = localPlayerId_;
    if (claim_.valid && (claim_.cards.isEmpty()
        || claim_.declarer < 0 || claim_.declarer >= players_.size())) {
        addLog(QStringLiteral("检测到无效声明状态，系统已自动清除该声明。"));
        claim_ = Claim{};
    }
    if (players_[playerIndex].finished)
        playerIndex = nextActive(playerIndex);

    currentPlayer_ = playerIndex;
    selected_.fill(false, players_[localPlayerId_].hand.size());
    if (playerIndex == localPlayerId_) {
        beginHumanTurn();
    } else if (mode_ == GameMode::Host && !isAiSeat(playerIndex)) {
        waitForRemote(playerIndex);
    } else if (mode_ == GameMode::Client) {
        // 客户端不推进本地状态机，只等待房主的状态快照。
        setPhase(Phase::Waiting, QStringLiteral("等待 %1 操作……").arg(players_[playerIndex].name));
        updateUi();
    } else {
        scheduleAiTurn(playerIndex);
    }
}

void QtWidgetsApplication1::scheduleAiTurn(int playerIndex)
{
    const QString analysisTarget = claim_.valid
        ? QStringLiteral("上一份声明和当前手牌")
        : QStringLiteral("当前手牌并规划出牌");
    const QString phaseText = players_[localPlayerId_].finished
        ? QStringLiteral("【观战模式 · 电脑决策阶段】\n你已获得第 %1 名，%2 正在认真分析%3……")
              .arg(players_[localPlayerId_].rank).arg(players_[playerIndex].name).arg(analysisTarget)
        : QStringLiteral("【电脑决策阶段】\n%1 正在分析%2，请稍候……")
              .arg(players_[playerIndex].name, analysisTarget);
    setPhase(Phase::Waiting, phaseText);
    updateUi();
    const int expectedGame = gameId_;
    const int thinkDelay = players_[localPlayerId_].finished
        ? AiThinkSpectatorDelayMs : AiThinkDelayMs;
    startAiProgress(thinkDelay, QStringLiteral("%1 正在思考")
        .arg(players_[playerIndex].name));
    QTimer::singleShot(thinkDelay, this, [this, expectedGame, playerIndex] {
        if (expectedGame == gameId_ && currentPlayer_ == playerIndex
            && phase_ == Phase::Waiting && !players_[playerIndex].finished)
            runAiTurn();
    });
}

void QtWidgetsApplication1::waitForRemote(int playerIndex)
{
    // 关键：phase_ 必须反映远端玩家当前需要的动作（出牌/判断），否则广播给客户端的
    // phase 会让其按钮失效，房主端 doPlay/doBelieve/doChallenge 也会拒绝远端操作。
    if (claim_.valid) {
        setPhase(Phase::Decide, QStringLiteral("等待 %1 判断……").arg(players_[playerIndex].name));
    } else {
        setPhase(Phase::Play, QStringLiteral("等待 %1 出牌……").arg(players_[playerIndex].name));
    }
    updateUi();
    startWaitProgress(QStringLiteral("等待 %1 操作").arg(players_[playerIndex].name));
    if (mode_ == GameMode::Host && host_)
        broadcastState();
}

void QtWidgetsApplication1::beginHumanTurn()
{
    if (claim_.valid) {
        if (players_[localPlayerId_].gamblingBan)
            setPhase(Phase::Decide, QStringLiteral("【禁赌状态】你在完成下一次真实出牌前不能质疑，本次只能相信。"));
        else if (challengeAllowed(localPlayerId_))
            setPhase(Phase::Decide, QStringLiteral("轮到你判断：相信 %1，还是质疑并揭牌？").arg(players_[claim_.declarer].name));
        else
            setPhase(Phase::Decide, QStringLiteral("【先喝再抓】中央牌池不足 %1 张，本次只能相信。")
                .arg(DrinkChallengePile));
    } else {
        const int minimum = minimumPlayCount(localPlayerId_);
        const int maximum = maximumPlayCount(localPlayerId_);
        const QString restriction = players_[localPlayerId_].gamblingBan
            ? QStringLiteral("【禁赌状态】下一次出牌必须作出完全真实的声明。")
            : QString();
        setPhase(Phase::Play, (minimum == maximum
            ? QStringLiteral("轮到你出牌：本次必须打出 %1 张牌。").arg(minimum)
            : QStringLiteral("轮到你出牌：请选择 %1–%2 张牌。").arg(minimum).arg(maximum))
            + restriction);
    }
    updateUi();
}

void QtWidgetsApplication1::runAiTurn()
{
    if (currentPlayer_ == localPlayerId_ || phase_ == Phase::GameOver || players_[currentPlayer_].finished)
        return;

    const int ai = currentPlayer_;
    const bool decidedOnClaim = claim_.valid;
    if (claim_.valid) {
        int suspicion = 14 + static_cast<int>(claim_.cards.size()) * 17;
        int knownDeclared = 0;
        for (int card : players_[ai].hand)
            if (card == claim_.declaredRank || card == JokerRank)
                ++knownDeclared;
        suspicion += knownDeclared * 5;
        if (players_[claim_.declarer].hand.isEmpty())
            suspicion += 10;
        aiUseInformationReward(ai, suspicion);
        suspicion = std::clamp(suspicion, 5, 92);

        const bool useAllIn = players_[ai].reward == SecretReward::GamblerAllIn
            && challengeAllowed(ai) && suspicion >= 70
            && QRandomGenerator::global()->bounded(100) < 65;
        if (challengeAllowed(ai)
            && (useAllIn || QRandomGenerator::global()->bounded(100) < suspicion)) {
            if (useAllIn) {
                consumeReward(ai);
                addLog(QStringLiteral("%1 发动【赌徒·梭哈】，并质疑了 %2！")
                           .arg(players_[ai].name, players_[claim_.declarer].name));
                showTableAction(QStringLiteral("%1 发动【赌徒·梭哈】！\n决定质疑 %2 的声明，等待揭牌……")
                                    .arg(players_[ai].name, players_[claim_.declarer].name), true);
            } else {
                addLog(QStringLiteral("%1 质疑了 %2！").arg(players_[ai].name, players_[claim_.declarer].name));
                showTableAction(QStringLiteral("%1 选择【质疑】！\n正在揭开 %2 刚才打出的牌……")
                                    .arg(players_[ai].name, players_[claim_.declarer].name), true);
            }
            setPhase(Phase::Waiting, players_[localPlayerId_].finished
                ? QStringLiteral("【观战模式 · 电脑决策结果】\n%1 选择了质疑，稍后将揭开 %2 的牌。")
                      .arg(players_[ai].name, players_[claim_.declarer].name)
                : QStringLiteral("【电脑决策结果】\n%1 选择了质疑，正在等待揭牌判定。")
                      .arg(players_[ai].name));
            updateUi();
            const int expectedGame = gameId_;
            const int challengeDelay = players_[localPlayerId_].finished
                ? AiChallengeSpectatorDisplayMs : AiChallengeDisplayMs;
            startAiProgress(challengeDelay, QStringLiteral("%1 已决定质疑，等待揭牌")
                .arg(players_[ai].name));
            QTimer::singleShot(challengeDelay, this, [this, ai, useAllIn, expectedGame] {
                if (expectedGame == gameId_ && currentPlayer_ == ai
                    && phase_ == Phase::Waiting && claim_.valid)
                    resolveChallenge(ai, useAllIn);
            });
            return;
        }

        const bool couldChallenge = challengeAllowed(ai);
        addLog(couldChallenge
            ? QStringLiteral("%1 选择相信上一份声明，中央牌池继续累积。").arg(players_[ai].name)
            : QStringLiteral("%1 受规则限制，只能相信，中央牌池继续累积。").arg(players_[ai].name));
        showTableAction(couldChallenge
            ? QStringLiteral("%1 选择【相信】%2 的声明。\n中央牌池继续保留，现在轮到 %1 出牌。")
                  .arg(players_[ai].name, players_[claim_.declarer].name)
            : QStringLiteral("%1 当前不能质疑，只能【相信】%2。\n中央牌池继续保留，现在轮到 %1 出牌。")
                  .arg(players_[ai].name, players_[claim_.declarer].name), true);
        setPhase(Phase::Waiting, players_[localPlayerId_].finished
            ? QStringLiteral("【观战模式 · 电脑决策结果】\n%1 选择了相信；该结果会停留显示，然后由它出牌。")
                  .arg(players_[ai].name)
            : QStringLiteral("【电脑决策结果】\n%1 选择了相信；稍后轮到它继续出牌。")
                  .arg(players_[ai].name));
        updateUi();
        checkBeliefTask(claim_.declarer);
        if (players_[claim_.declarer].hand.isEmpty()) {
            confirmFinishedPlayer(claim_.declarer);
            if (finishGameIfReady())
                return;
        }
        claim_ = Claim{};
    }

    const int expectedGame = gameId_;
    const int actionDelay = players_[localPlayerId_].finished
        ? (decidedOnClaim ? AiDecisionSpectatorDisplayMs : AiOpeningPlaySpectatorDelayMs)
        : (decidedOnClaim ? AiDecisionDisplayMs : AiOpeningPlayDelayMs);
    if (!decidedOnClaim) {
        setPhase(Phase::Waiting, players_[localPlayerId_].finished
            ? QStringLiteral("【观战模式 · 电脑决策结果】\n%1 已完成分析，正在选择要打出的手牌。")
                  .arg(players_[ai].name)
            : QStringLiteral("【电脑决策结果】\n%1 已完成分析，正在准备出牌。")
                  .arg(players_[ai].name));
        updateUi();
    }
    startAiProgress(actionDelay, decidedOnClaim
        ? QStringLiteral("决策结果展示中，稍后出牌")
        : QStringLiteral("%1 正在准备出牌").arg(players_[ai].name));
    QTimer::singleShot(actionDelay, this, [this, ai, expectedGame] {
        if (expectedGame == gameId_ && currentPlayer_ == ai
            && phase_ == Phase::Waiting && !players_[ai].finished)
            aiPlay();
    });
}

void QtWidgetsApplication1::aiPlay()
{
    const int ai = currentPlayer_;
    if (ai == localPlayerId_ || phase_ == Phase::GameOver || players_[ai].finished || players_[ai].hand.isEmpty())
        return;

    Player &player = players_[ai];
    const bool mustTellTruth = player.gamblingBan;
    int declared = QRandomGenerator::global()->bounded(RankCount);

    auto matchingIndices = [&player](int rank) {
        QVector<int> result;
        for (int i = 0; i < player.hand.size(); ++i)
            if (player.hand[i] == rank || player.hand[i] == JokerRank)
                result.append(i);
        std::shuffle(result.begin(), result.end(), *QRandomGenerator::global());
        return result;
    };

    QVector<int> matching = matchingIndices(declared);
    if (mustTellTruth) {
        for (int rank = 0; rank < RankCount; ++rank) {
            const QVector<int> candidate = matchingIndices(rank);
            if (candidate.size() > matching.size()) {
                declared = rank;
                matching = candidate;
            }
        }
    }

    const int minimumCards = minimumPlayCount(ai);
    const int maximumCards = maximumPlayCount(ai);
    const bool canTellTruth = matching.size() >= minimumCards;
    bool intendsTruth = mustTellTruth || (canTellTruth && QRandomGenerator::global()->bounded(100) < 62);
    int count = minimumCards;
    if (maximumCards > minimumCards)
        count += QRandomGenerator::global()->bounded(maximumCards - minimumCards + 1);
    QVector<int> indices;
    if (intendsTruth) {
        count = std::min(count, static_cast<int>(matching.size()));
        indices = matching.mid(0, count);
    } else {
        QVector<int> shuffled;
        for (int i = 0; i < player.hand.size(); ++i)
            shuffled.append(i);
        std::shuffle(shuffled.begin(), shuffled.end(), *QRandomGenerator::global());
        for (int index : shuffled) {
            if (indices.size() >= count)
                break;
            if (player.hand[index] != declared && player.hand[index] != JokerRank)
                indices.append(index);
        }
        for (int index : shuffled) {
            if (indices.size() >= count)
                break;
            if (!indices.contains(index))
                indices.append(index);
        }
    }

    if (!isLegalPlaySelection(ai, indices, declared)) {
        indices.clear();
        const int fallbackCount = minimumPlayCount(ai);
        if (mustTellTruth) {
            declared = 0;
            matching = matchingIndices(declared);
            for (int rank = 1; rank < RankCount; ++rank) {
                const QVector<int> candidate = matchingIndices(rank);
                if (candidate.size() > matching.size()) {
                    declared = rank;
                    matching = candidate;
                }
            }
            indices = matching.mid(0, fallbackCount);
        } else {
            declared = QRandomGenerator::global()->bounded(RankCount);
            for (int i = 0; i < player.hand.size(); ++i)
                indices.append(i);
            std::shuffle(indices.begin(), indices.end(), *QRandomGenerator::global());
            indices = indices.mid(0, fallbackCount);
        }
    }
    if (!isLegalPlaySelection(ai, indices, declared)) {
        setPhase(Phase::GameOver, QStringLiteral("检测到异常的电脑出牌状态，本局已安全停止。"));
        addLog(QStringLiteral("电脑出牌合法性校验未通过，系统已阻止该操作。"));
        updateUi();
        return;
    }

    const int pileSizeBeforePlay = tablePile_.size();
    const int handSizeBeforePlay = player.hand.size();
    std::sort(indices.begin(), indices.end(), std::greater<int>());
    QVector<int> cards;
    for (int index : indices) {
        cards.prepend(player.hand[index]);
        player.hand.removeAt(index);
    }

    claim_ = Claim{};
    claim_.valid = true;
    claim_.declarer = ai;
    claim_.cards = cards;
    claim_.declaredRank = declared;
    claim_.pileSizeBeforePlay = pileSizeBeforePlay;
    claim_.declarerHandSizeBeforePlay = handSizeBeforePlay;
    for (int card : cards)
        tablePile_.append(card);
    recordValidPlay(cards.size());
    claimRevealed_ = false;
    addLog(QStringLiteral("%1 盖下 %2 张牌，声明：%2 张 %3。")
               .arg(player.name).arg(cards.size()).arg(cardName(declared)));
    showTableAction(QStringLiteral("【电脑出牌】%1 完成出牌：\n盖下 %2 张牌，并声明“%2 张 %3”。")
                        .arg(player.name).arg(cards.size()).arg(cardName(declared)));
    showTableCards(claim_.cards, false);
    if (mustTellTruth) {
        player.gamblingBan = false;
        addLog(QStringLiteral("%1 已完成下一次真实出牌，禁赌状态解除。").arg(player.name));
    }
    beginTurn(nextActive(ai));
}

void QtWidgetsApplication1::onPlayClicked()
{
    if (phase_ != Phase::Play || currentPlayer_ != localPlayerId_ || players_[localPlayerId_].finished)
        return;

    QVector<int> indices;
    for (int i = 0; i < selected_.size(); ++i)
        if (selected_[i])
            indices.append(i);
    const int minimum = minimumPlayCount(localPlayerId_);
    const int maximum = maximumPlayCount(localPlayerId_);
    if (indices.size() < minimum || indices.size() > maximum) {
        const QString requirement = minimum == maximum
            ? QStringLiteral("本次必须选择 %1 张牌。").arg(minimum)
            : QStringLiteral("本次必须选择 %1–%2 张牌。").arg(minimum).arg(maximum);
        QMessageBox::information(this, QStringLiteral("不能出牌"), requirement);
        return;
    }

    const int declaredRank = rankCombo_->currentIndex();
    QVector<int> previewCards;
    for (int index : indices)
        previewCards.append(players_[localPlayerId_].hand[index]);
    if (players_[localPlayerId_].gamblingBan && !cardsMatchClaim(previewCards, declaredRank)) {
        QMessageBox::information(this, QStringLiteral("禁赌状态"),
            QStringLiteral("梭哈失败后的下一次出牌必须完全真实。\n"
                           "所选牌必须都是声明牌型或 Joker。"));
        return;
    }
    if (!isLegalPlaySelection(localPlayerId_, indices, declaredRank)) {
        QMessageBox::warning(this, QStringLiteral("不能出牌"),
            QStringLiteral("本次出牌未通过规则校验，请重新选择手牌和声明牌型。"));
        return;
    }

    if (mode_ == GameMode::Client) {
        QJsonObject payload;
        QJsonArray idx;
        for (int i : indices)
            idx.append(i);
        payload.insert(QStringLiteral("indices"), idx);
        payload.insert(QStringLiteral("declaredRank"), declaredRank);
        sendAction(QStringLiteral("play"), payload);
        return;
    }
    doPlay(localPlayerId_, indices, declaredRank);
}

void QtWidgetsApplication1::doPlay(int playerIndex, const QVector<int> &indices, int declaredRank)
{
    if (phase_ != Phase::Play || currentPlayer_ != playerIndex
        || playerIndex < 0 || playerIndex >= players_.size()
        || players_[playerIndex].finished
        || !isLegalPlaySelection(playerIndex, indices, declaredRank))
        return;

    Player &player = players_[playerIndex];
    claim_ = Claim{};
    claim_.valid = true;
    claim_.declarer = playerIndex;
    claim_.declaredRank = declaredRank;
    claim_.pileSizeBeforePlay = tablePile_.size();
    claim_.declarerHandSizeBeforePlay = player.hand.size();
    QVector<int> sortedIndices = indices;
    std::sort(sortedIndices.begin(), sortedIndices.end(), std::greater<int>());
    for (int index : sortedIndices) {
        claim_.cards.prepend(player.hand[index]);
        player.hand.removeAt(index);
    }
    for (int card : claim_.cards)
        tablePile_.append(card);
    recordValidPlay(claim_.cards.size());
    claimRevealed_ = false;

    addLog(QStringLiteral("%1 盖下 %2 张牌，声明：%2 张 %3。")
               .arg(player.name).arg(claim_.cards.size()).arg(cardName(claim_.declaredRank)));
    showTableAction(QStringLiteral("%1 完成出牌：\n盖下 %2 张牌，并声明“%2 张 %3”。")
                        .arg(player.name).arg(claim_.cards.size()).arg(cardName(claim_.declaredRank)));
    showTableCards(claim_.cards, false);
    if (player.gamblingBan) {
        player.gamblingBan = false;
        addLog(QStringLiteral("%1 已完成下一次真实出牌，禁赌状态解除。").arg(player.name));
    }
    beginTurn(nextActive(playerIndex));
}

void QtWidgetsApplication1::onBelieveClicked()
{
    if (phase_ != Phase::Decide || currentPlayer_ != localPlayerId_ || !claim_.valid
        || claim_.cards.isEmpty()
        || claim_.declarer < 0 || claim_.declarer >= players_.size())
        return;

    if (mode_ == GameMode::Client) {
        sendAction(QStringLiteral("believe"));
        return;
    }
    doBelieve(localPlayerId_);
}

void QtWidgetsApplication1::doBelieve(int playerIndex)
{
    if (phase_ != Phase::Decide || currentPlayer_ != playerIndex || !claim_.valid
        || claim_.cards.isEmpty()
        || claim_.declarer < 0 || claim_.declarer >= players_.size())
        return;

    addLog(QStringLiteral("%1 选择相信 %2，中央牌池继续累积。")
               .arg(players_[playerIndex].name, players_[claim_.declarer].name));
    showTableAction(QStringLiteral("%1 选择【相信】%2 的声明。\n中央牌池继续保留，现在轮到 %1 出牌。")
                        .arg(players_[playerIndex].name, players_[claim_.declarer].name));
    checkBeliefTask(claim_.declarer);
    if (players_[claim_.declarer].hand.isEmpty()) {
        confirmFinishedPlayer(claim_.declarer);
        if (finishGameIfReady())
            return;
    }
    claim_ = Claim{};
    claimRevealed_ = false;
    beginTurn(playerIndex);
}

void QtWidgetsApplication1::onChallengeClicked()
{
    if (phase_ == Phase::Decide && currentPlayer_ == localPlayerId_ && claim_.valid && challengeAllowed(localPlayerId_)) {
        if (mode_ == GameMode::Client) {
            sendAction(QStringLiteral("challenge"));
            return;
        }
        doChallenge(localPlayerId_);
    }
}

void QtWidgetsApplication1::doChallenge(int playerIndex, bool allIn)
{
    if (phase_ != Phase::Decide || currentPlayer_ != playerIndex || !claim_.valid
        || !challengeAllowed(playerIndex))
        return;

    addLog(QStringLiteral("%1 质疑了 %2！").arg(players_[playerIndex].name, players_[claim_.declarer].name));
    showTableAction(QStringLiteral("%1 选择【质疑】！\n正在揭开 %2 刚才打出的牌……")
                        .arg(players_[playerIndex].name, players_[claim_.declarer].name), true);
    resolveChallenge(playerIndex, allIn);
}

void QtWidgetsApplication1::onRewardClicked()
{
    if (!rewardUsable(localPlayerId_))
        return;

    const SecretReward reward = players_[localPlayerId_].reward;

    // 客户端：本地收集参数 → 发送 action 给房主，由房主权威结算并回传私密结果。
    if (mode_ == GameMode::Client) {
        if (reward == SecretReward::PeekCard) {
            QJsonObject payload;
            payload.insert(QStringLiteral("reward"), QStringLiteral("peek"));
            sendAction(QStringLiteral("reward"), payload);
        } else if (reward == SecretReward::RankScout) {
            QStringList targets;
            QVector<int> targetIndices;
            for (int i = 0; i < players_.size(); ++i) {
                if (i != localPlayerId_ && !players_[i].finished) {
                    targets << players_[i].name;
                    targetIndices.append(i);
                }
            }
            if (targets.isEmpty())
                return;
            bool accepted = false;
            const QString targetText = QInputDialog::getItem(this, QStringLiteral("点数侦查"),
                QStringLiteral("选择一名仍在游戏中的玩家："), targets, 0, false, &accepted);
            if (!accepted)
                return;
            const QStringList ranks = rankChoices();
            const QString rankText = QInputDialog::getItem(this, QStringLiteral("点数侦查"),
                QStringLiteral("选择要侦查的牌型："), ranks, 0, false, &accepted);
            if (!accepted)
                return;
            const int targetPosition = targets.indexOf(targetText);
            const int rank = ranks.indexOf(rankText);
            if (targetPosition < 0 || targetPosition >= targetIndices.size()
                || rank < 0 || rank > JokerRank)
                return;
            QJsonObject payload;
            payload.insert(QStringLiteral("reward"), QStringLiteral("rankScout"));
            payload.insert(QStringLiteral("target"), targetIndices[targetPosition]);
            payload.insert(QStringLiteral("rank"), rank);
            sendAction(QStringLiteral("reward"), payload);
        } else if (reward == SecretReward::PileScout) {
            bool accepted = false;
            const QStringList ranks = rankChoices();
            const QString rankText = QInputDialog::getItem(this, QStringLiteral("牌堆侦察"),
                QStringLiteral("选择要侦察的牌型："), ranks, 0, false, &accepted);
            if (!accepted)
                return;
            const int rank = ranks.indexOf(rankText);
            if (rank < 0 || rank > JokerRank)
                return;
            QJsonObject payload;
            payload.insert(QStringLiteral("reward"), QStringLiteral("pileScout"));
            payload.insert(QStringLiteral("rank"), rank);
            sendAction(QStringLiteral("reward"), payload);
        } else if (reward == SecretReward::GamblerAllIn) {
            if (QMessageBox::question(this, QStringLiteral("发动赌徒·梭哈"),
                    QStringLiteral("发动后必须立即质疑，不能改为相信。\n\n"
                                   "质疑成功：秘密查看一名其他玩家的全部手牌。\n"
                                   "质疑失败：收牌，并在下一次真实出牌前不能质疑或撒谎。\n\n确定发动吗？"))
                != QMessageBox::Yes) {
                return;
            }
            QJsonObject payload;
            payload.insert(QStringLiteral("reward"), QStringLiteral("allIn"));
            sendAction(QStringLiteral("reward"), payload);
        }
        return;
    }

    // 本地人机 / 房主：沿用原有本地权威结算逻辑。
    if (reward == SecretReward::PeekCard) {
        const int index = QRandomGenerator::global()->bounded(static_cast<int>(claim_.cards.size()));
        const QString seenCard = cardName(claim_.cards[index]);
        consumeReward(localPlayerId_);
        QMessageBox::information(this, QStringLiteral("窥牌结果"),
            QStringLiteral("你随机看到上一家刚打出的 1 张牌：%1\n\n其余牌仍然隐藏，请继续选择相信或质疑。")
                .arg(seenCard));
        return;
    }

    if (reward == SecretReward::RankScout) {
        QStringList targets;
        QVector<int> targetIndices;
        for (int i = 0; i < players_.size(); ++i) {
            if (i != localPlayerId_ && !players_[i].finished) {
                targets << players_[i].name;
                targetIndices.append(i);
            }
        }
        if (targets.isEmpty())
            return;
        bool accepted = false;
        const QString targetText = QInputDialog::getItem(this, QStringLiteral("点数侦查"),
            QStringLiteral("选择一名仍在游戏中的玩家："), targets, 0, false, &accepted);
        if (!accepted)
            return;
        const QStringList ranks = rankChoices();
        const QString rankText = QInputDialog::getItem(this, QStringLiteral("点数侦查"),
            QStringLiteral("选择要侦查的牌型："), ranks, 0, false, &accepted);
        if (!accepted)
            return;
        const int targetPosition = targets.indexOf(targetText);
        const int rank = ranks.indexOf(rankText);
        if (targetPosition < 0 || targetPosition >= targetIndices.size()
            || rank < 0 || rank > JokerRank)
            return;
        const int target = targetIndices[targetPosition];
        int count = 0;
        for (int card : players_[target].hand)
            if (card == rank)
                ++count;
        consumeReward(localPlayerId_);
        QMessageBox::information(this, QStringLiteral("点数侦查结果"),
            QStringLiteral("%1 当前手中有 %2 张 %3。\n\n这条情报只代表当前时刻。")
                .arg(players_[target].name).arg(count).arg(cardName(rank)));
        return;
    }

    if (reward == SecretReward::PileScout) {
        bool accepted = false;
        const QStringList ranks = rankChoices();
        const QString rankText = QInputDialog::getItem(this, QStringLiteral("牌堆侦察"),
            QStringLiteral("选择要侦察的牌型："), ranks, 0, false, &accepted);
        if (!accepted)
            return;
        const int rank = ranks.indexOf(rankText);
        if (rank < 0 || rank > JokerRank)
            return;
        int count = 0;
        for (int card : tablePile_)
            if (card == rank)
                ++count;
        consumeReward(localPlayerId_);
        QMessageBox::information(this, QStringLiteral("牌堆侦察结果"),
            QStringLiteral("当前中央牌堆中共有 %1 张 %2。\n\n系统不会显示这些牌由谁打出。")
                .arg(count).arg(cardName(rank)));
        return;
    }

    if (reward == SecretReward::GamblerAllIn) {
        if (QMessageBox::question(this, QStringLiteral("发动赌徒·梭哈"),
                QStringLiteral("发动后必须立即质疑，不能改为相信。\n\n"
                               "质疑成功：秘密查看一名其他玩家的全部手牌。\n"
                               "质疑失败：收牌，并在下一次真实出牌前不能质疑或撒谎。\n\n确定发动吗？"))
            != QMessageBox::Yes) {
            return;
        }
        consumeReward(localPlayerId_);
        addLog(QStringLiteral("你发动【赌徒·梭哈】，并质疑了 %1！").arg(players_[claim_.declarer].name));
        showTableAction(QStringLiteral("你发动【赌徒·梭哈】！\n正在揭开 %1 刚才打出的牌……")
                            .arg(players_[claim_.declarer].name), true);
        resolveChallenge(localPlayerId_, true);
    }
}

void QtWidgetsApplication1::resolveChallenge(int challenger, bool allIn)
{
    if (!claim_.valid || claim_.cards.isEmpty()
        || challenger < 0 || challenger >= players_.size()
        || players_[challenger].finished
        || claim_.declarer < 0 || claim_.declarer >= players_.size()
        || challenger == claim_.declarer) {
        addLog(QStringLiteral("质疑状态校验未通过，系统已阻止本次无效结算。"));
        return;
    }

    allInChallenger_ = allIn ? challenger : -1;
    showTableCards(claim_.cards, true);
    claimRevealed_ = true;   // 揭牌后，本份声明进入公开状态（同步给客户端）
    setPhase(Phase::Waiting, QStringLiteral("揭牌判定中……"));
    updateUi();

    const int declarer = claim_.declarer;
    QStringList revealed;
    for (int card : claim_.cards)
        revealed << cardName(card);
    const bool truthful = claimIsTrue();
    addLog(QStringLiteral("揭开的牌是：[ %1 ]。声明%2。")
               .arg(revealed.join(QStringLiteral("、")), truthful ? QStringLiteral("真实") : QStringLiteral("是谎言")));

    const int settledPileSize = tablePile_.size();
    const int loser = truthful ? challenger : declarer;
    const QString reason = truthful ? QStringLiteral("质疑错误") : QStringLiteral("谎言被识破");
    for (int card : tablePile_)
        players_[loser].hand.append(card);
    addLog(QStringLiteral("%1 因“%2”收走中央牌池的 %3 张牌，现在有 %4 张手牌。")
               .arg(players_[loser].name, reason)
               .arg(settledPileSize)
               .arg(players_[loser].hand.size()));
    showTableAction(QStringLiteral("【揭牌】%1\n【判定】%2 的声明%3，%4。\n【结算】%5 收走中央牌池的 %6 张牌。")
                        .arg(revealed.join(QStringLiteral("、")))
                        .arg(players_[declarer].name)
                        .arg(truthful ? QStringLiteral("真实") : QStringLiteral("是谎言"))
                        .arg(truthful
                            ? QStringLiteral("质疑者判断错误")
                            : QStringLiteral("质疑者成功识破谎言"))
                        .arg(players_[loser].name)
                        .arg(settledPileSize), true);
    setPhase(Phase::Waiting, players_[localPlayerId_].finished
        ? QStringLiteral("【观战模式 · 揭牌结算结果】\n本次判定已完成，请查看中央提示；结果停留后再开始下一轮。")
        : QStringLiteral("【揭牌结算结果】\n本次判定已完成，请查看中央提示；结果停留后再开始下一轮。"));
    updateUi();

    checkChallengeTasks(challenger, truthful, settledPileSize);
    if (allIn)
        handleAllInResult(challenger, !truthful);

    if (currentEvent_ == TavernEvent::BartenderRush || currentEvent_ == TavernEvent::DrinkBeforeCatch) {
        addLog(QStringLiteral("【酒馆事件结束】%1结束，下轮恢复基础规则。").arg(eventName(currentEvent_)));
        currentEvent_ = TavernEvent::None;
    }
    if (truthful && players_[declarer].hand.isEmpty())
        confirmFinishedPlayer(declarer);
    detectNextRoundEvents(settledPileSize);
    claim_ = Claim{};
    allInChallenger_ = -1;
    tablePile_.clear();
    updateUi();
    if (finishGameIfReady())
        return;

    const int expectedGame = gameId_;
    const int settlementDelay = players_[localPlayerId_].finished
        ? AiSettlementSpectatorDisplayMs : AiSettlementDisplayMs;
    startAiProgress(settlementDelay, QStringLiteral("揭牌与收牌结果展示中"));
    QTimer::singleShot(settlementDelay, this, [this, loser, expectedGame] {
        if (expectedGame == gameId_ && phase_ == Phase::Waiting
            && loser >= 0 && loser < players_.size())
            startNewRound(loser);
    });
}

void QtWidgetsApplication1::confirmFinishedPlayer(int playerIndex)
{
    Player &player = players_[playerIndex];
    if (player.finished || !player.hand.isEmpty())
        return;

    player.finished = true;
    player.rank = finishOrder_.size() + 1;
    finishOrder_.append(playerIndex);
    playsSinceLastRank_ = 0;
    addLog(QStringLiteral("%1 已出完全部手牌，获得第 %2 名！").arg(player.name).arg(player.rank));
    QString finishNotice = QStringLiteral("%1 已安全出完全部手牌！\n获得本局第 %2 名，接下来退出出牌顺序。")
        .arg(player.name).arg(player.rank);
    if (tableActionText_.startsWith(QStringLiteral("【揭牌】")))
        finishNotice = tableActionText_ + QStringLiteral("\n\n【排名】") + finishNotice;
    showTableAction(finishNotice, true);
    updateEventAfterRank();
    updateUi();
    showRankPopup(playerIndex);
}

bool QtWidgetsApplication1::finishGameIfReady()
{
    if (finishOrder_.size() < players_.size() - 1)
        return false;

    QString fourthPlaceName;
    if (finishOrder_.size() == players_.size() - 1) {
        for (int i = 0; i < players_.size(); ++i) {
            if (!players_[i].finished) {
                players_[i].finished = true;
                players_[i].rank = players_.size();
                finishOrder_.append(i);
                addLog(QStringLiteral("%1 为最后一位未出完手牌的玩家，获得第 %2 名。")
                           .arg(players_[i].name).arg(players_[i].rank));
                fourthPlaceName = players_[i].name;
                break;
            }
        }
    }

    const QString result = rankingSummary();
    currentEvent_ = TavernEvent::None;
    setPhase(Phase::GameOver, QStringLiteral("对局结束！%1").arg(result));
    addLog(QStringLiteral("最终排名：%1").arg(result));
    showTableAction(QStringLiteral("对局结束！\n最终排名：%1").arg(result), true);
    updateUi();
    if (mode_ == GameMode::Host && host_) {
        QJsonObject payload;
        payload.insert(QStringLiteral("result"), result);
        host_->broadcast(Protocol::Msg::GameOver, payload);
    }
    const QString gameOverText = fourthPlaceName.isEmpty()
        ? QStringLiteral("最终排名\n%1").arg(result)
        : QStringLiteral("【新名次产生】\n%1 获得本局第 4 名。\n\n所有名次已经产生：\n%2")
              .arg(fourthPlaceName, result);
    QMessageBox::information(this, QStringLiteral("游戏结束"), gameOverText);
    return true;
}

void QtWidgetsApplication1::recordValidPlay(int cardCount)
{
    recentPlayCounts_.append(cardCount);
    while (recentPlayCounts_.size() > ConservativePlayWindow)
        recentPlayCounts_.removeAt(0);

    ++playsSinceLastRank_;
    if (activePlayerCount() >= 3 && playsSinceLastRank_ >= ClosingTimePlays
        && currentEvent_ != TavernEvent::FinalTable
        && currentEvent_ != TavernEvent::ClosingTime) {
        activateEvent(TavernEvent::ClosingTime);
    }
}

void QtWidgetsApplication1::detectNextRoundEvents(int settledPileSize)
{
    recentRoundPileSizes_.append(settledPileSize);
    while (recentRoundPileSizes_.size() > SmallPileRounds)
        recentRoundPileSizes_.removeAt(0);

    pendingBartenderRush_ = false;
    pendingDrinkBeforeCatch_ = false;
    if (activePlayerCount() < 3 || currentEvent_ == TavernEvent::FinalTable
        || currentEvent_ == TavernEvent::ClosingTime) {
        return;
    }

    if (recentPlayCounts_.size() == ConservativePlayWindow) {
        int conservativePlays = 0;
        for (int count : recentPlayCounts_)
            if (count <= ConservativePlayMaxCards)
                ++conservativePlays;
        pendingBartenderRush_ = conservativePlays >= ConservativePlayNeed;
    }

    if (recentRoundPileSizes_.size() == SmallPileRounds) {
        pendingDrinkBeforeCatch_ = true;
        for (int pileSize : recentRoundPileSizes_)
            if (pileSize > SmallPileMax)
                pendingDrinkBeforeCatch_ = false;
    }
}

void QtWidgetsApplication1::chooseEventForNewRound()
{
    if (activePlayerCount() == 2) {
        if (currentEvent_ != TavernEvent::FinalTable)
            activateEvent(TavernEvent::FinalTable);
        pendingBartenderRush_ = false;
        pendingDrinkBeforeCatch_ = false;
        return;
    }

    if (currentEvent_ == TavernEvent::ClosingTime) {
        pendingBartenderRush_ = false;
        pendingDrinkBeforeCatch_ = false;
        return;
    }

    TavernEvent nextEvent = TavernEvent::None;
    if (playsSinceLastRank_ >= ClosingTimePlays)
        nextEvent = TavernEvent::ClosingTime;
    else if (pendingDrinkBeforeCatch_)
        nextEvent = TavernEvent::DrinkBeforeCatch;
    else if (pendingBartenderRush_)
        nextEvent = TavernEvent::BartenderRush;

    if (nextEvent == TavernEvent::DrinkBeforeCatch || nextEvent == TavernEvent::BartenderRush) {
        recentRoundPileSizes_.clear();
        recentPlayCounts_.clear();
    }
    pendingBartenderRush_ = false;
    pendingDrinkBeforeCatch_ = false;
    activateEvent(nextEvent);
}

void QtWidgetsApplication1::activateEvent(TavernEvent event)
{
    if (currentEvent_ == event)
        return;

    currentEvent_ = event;
    if (event != TavernEvent::None) {
        addLog(QStringLiteral("【酒馆事件：%1】%2").arg(eventName(event), eventDescription(event)));
    }
    updateUi();
}

void QtWidgetsApplication1::updateEventAfterRank()
{
    if (activePlayerCount() == 2) {
        if (currentEvent_ != TavernEvent::FinalTable) {
            if (currentEvent_ != TavernEvent::None)
                addLog(QStringLiteral("原事件因进入双人残局而结束。"));
            activateEvent(TavernEvent::FinalTable);
        }
        pendingBartenderRush_ = false;
        pendingDrinkBeforeCatch_ = false;
        return;
    }

    if (currentEvent_ == TavernEvent::ClosingTime) {
        addLog(QStringLiteral("【酒馆事件结束】新的名次已经产生，酒馆打烊解除。"));
        activateEvent(TavernEvent::None);
    }
}

int QtWidgetsApplication1::minimumPlayCount(int playerIndex) const
{
    const int handSize = static_cast<int>(players_[playerIndex].hand.size());
    if (handSize <= 0)
        return 0;
    int minimum = 1;
    if (currentEvent_ == TavernEvent::BartenderRush)
        minimum = std::min(MaxPlayCards, handSize);
    else if (currentEvent_ == TavernEvent::FinalTable || currentEvent_ == TavernEvent::ClosingTime)
        minimum = std::min(2, handSize);

    if (players_[playerIndex].gamblingBan)
        minimum = std::min(minimum, truthfulPlayCapacity(playerIndex));
    return minimum;
}

int QtWidgetsApplication1::maximumPlayCount(int playerIndex) const
{
    int maximum = std::min(MaxPlayCards, static_cast<int>(players_[playerIndex].hand.size()));
    if (players_[playerIndex].gamblingBan)
        maximum = std::min(maximum, truthfulPlayCapacity(playerIndex));
    return maximum;
}

int QtWidgetsApplication1::truthfulPlayCapacity(int playerIndex) const
{
    int jokers = 0;
    int counts[RankCount] = {};
    for (int card : players_[playerIndex].hand) {
        if (card == JokerRank)
            ++jokers;
        else if (card >= 0 && card < RankCount)
            ++counts[card];
    }
    int best = 0;
    for (int count : counts)
        best = std::max(best, count + jokers);
    return std::min(MaxPlayCards, best);
}

bool QtWidgetsApplication1::challengeAllowed(int challenger) const
{
    if (!claim_.valid || claim_.cards.isEmpty()
        || claim_.declarer < 0 || claim_.declarer >= players_.size())
        return false;
    if (challenger < 0 || challenger >= players_.size()
        || challenger == claim_.declarer || players_[challenger].finished
        || players_[challenger].gamblingBan)
        return false;
    if (currentEvent_ != TavernEvent::DrinkBeforeCatch)
        return true;
    return tablePile_.size() >= DrinkChallengePile || players_[claim_.declarer].hand.isEmpty();
}

int QtWidgetsApplication1::activePlayerCount() const
{
    int count = 0;
    for (const Player &player : players_)
        if (!player.finished)
            ++count;
    return count;
}

void QtWidgetsApplication1::assignRandomTask(int playerIndex)
{
    Player &player = players_[playerIndex];
    if (player.completedTasks >= 2 || player.finished) {
        player.task = SecretTask::None;
        return;
    }
    player.task = static_cast<SecretTask>(1 + QRandomGenerator::global()->bounded(4));
}

void QtWidgetsApplication1::completeSecretTask(int playerIndex)
{
    Player &player = players_[playerIndex];
    if (player.task == SecretTask::None || player.completedTasks >= 2)
        return;

    const SecretTask completedTask = player.task;
    player.task = SecretTask::None;
    ++player.completedTasks;
    if (playerIndex == localPlayerId_)
        addLog(QStringLiteral("【秘密任务完成】%1（本局已完成 %2/2）。")
                   .arg(taskName(completedTask)).arg(player.completedTasks));

    if (player.reward == SecretReward::None) {
        grantRandomReward(playerIndex);
    } else {
        ++player.pendingRewards;
        if (playerIndex == localPlayerId_)
            addLog(QStringLiteral("你当前已有奖励，新奖励已进入待领取状态；旧奖励使用或失效后自动发放。"));
    }

    if (player.completedTasks < 2) {
        assignRandomTask(playerIndex);
        if (playerIndex == localPlayerId_)
            addLog(QStringLiteral("【新的秘密任务】%1：%2")
                       .arg(taskName(player.task), taskDescription(player.task)));
    } else if (playerIndex == localPlayerId_) {
        addLog(QStringLiteral("你已完成本局允许的 2 个秘密任务，不再获得新任务。"));
    }
    updateUi();
}

void QtWidgetsApplication1::checkBeliefTask(int declarer)
{
    if (declarer < 0 || declarer >= players_.size())
        return;
    const bool truthful = cardsMatchClaim(claim_.cards, claim_.declaredRank);
    const SecretTask task = players_[declarer].task;
    if (task == SecretTask::DeceiveAtRisk
        && claim_.pileSizeBeforePlay >= DeceivePileRequirement
        && claim_.cards.size() >= 2 && !truthful) {
        completeSecretTask(declarer);
    } else if (task == SecretTask::DesperateBluff
        && claim_.declarerHandSizeBeforePlay >= DesperateHandMin
        && claim_.declarerHandSizeBeforePlay <= DesperateHandMax
        && claim_.cards.size() == MaxPlayCards && !truthful) {
        completeSecretTask(declarer);
    }
}

void QtWidgetsApplication1::checkChallengeTasks(int challenger, bool truthful, int settledPileSize)
{
    const int declarer = claim_.declarer;
    if (truthful && players_[declarer].task == SecretTask::BaitChallenge
        && claim_.pileSizeBeforePlay >= BaitPileRequirement
        && claim_.cards.size() >= 2) {
        completeSecretTask(declarer);
    }
    if (!truthful && players_[challenger].task == SecretTask::BoldChallenge
        && settledPileSize >= BoldPileRequirement) {
        completeSecretTask(challenger);
    }
}

void QtWidgetsApplication1::grantRandomReward(int playerIndex)
{
    Player &player = players_[playerIndex];
    if (player.reward != SecretReward::None)
        return;
    player.reward = static_cast<SecretReward>(1 + QRandomGenerator::global()->bounded(4));
    player.rewardAwardRound = std::max(1, roundNumber_);
    if (playerIndex == localPlayerId_) {
        addLog(QStringLiteral("【获得秘密奖励】%1：%2（可在本轮及下一轮使用）")
                   .arg(rewardName(player.reward), rewardDescription(player.reward)));
    }
}

void QtWidgetsApplication1::grantPendingReward(int playerIndex)
{
    Player &player = players_[playerIndex];
    if (player.reward == SecretReward::None && player.pendingRewards > 0) {
        --player.pendingRewards;
        grantRandomReward(playerIndex);
    }
}

void QtWidgetsApplication1::consumeReward(int playerIndex)
{
    Player &player = players_[playerIndex];
    player.reward = SecretReward::None;
    player.rewardAwardRound = 0;
    grantPendingReward(playerIndex);
    updateUi();
}

void QtWidgetsApplication1::expireRewardsForNewRound()
{
    for (int i = 0; i < players_.size(); ++i) {
        Player &player = players_[i];
        if (player.reward != SecretReward::None
            && roundNumber_ > player.rewardAwardRound + 1) {
            if (i == localPlayerId_)
                addLog(QStringLiteral("【奖励失效】%1 未在有效期内使用，已经作废。")
                           .arg(rewardName(player.reward)));
            player.reward = SecretReward::None;
            player.rewardAwardRound = 0;
            grantPendingReward(i);
        }
    }
}

void QtWidgetsApplication1::aiUseInformationReward(int playerIndex, int &suspicion)
{
    Player &player = players_[playerIndex];
    if (!claim_.valid || claim_.cards.isEmpty()
        || claim_.declarer < 0 || claim_.declarer >= players_.size()
        || player.reward == SecretReward::None)
        return;

    if (player.reward == SecretReward::PeekCard
        && QRandomGenerator::global()->bounded(100) < 45) {
        const int seen = claim_.cards[QRandomGenerator::global()->bounded(static_cast<int>(claim_.cards.size()))];
        if (seen != claim_.declaredRank && seen != JokerRank)
            suspicion = 100;
        else
            suspicion += 8;
        consumeReward(playerIndex);
    } else if (player.reward == SecretReward::RankScout
        && QRandomGenerator::global()->bounded(100) < 25) {
        int count = 0;
        for (int card : players_[claim_.declarer].hand)
            if (card == claim_.declaredRank)
                ++count;
        suspicion += count * 7;
        consumeReward(playerIndex);
    } else if (player.reward == SecretReward::PileScout && !tablePile_.isEmpty()
        && QRandomGenerator::global()->bounded(100) < 30) {
        int count = 0;
        for (int card : tablePile_)
            if (card == claim_.declaredRank)
                ++count;
        suspicion += std::max(0, count - 2) * 6;
        consumeReward(playerIndex);
    }
}

bool QtWidgetsApplication1::rewardUsable(int playerIndex) const
{
    if (playerIndex < 0 || playerIndex >= players_.size()
        || players_[playerIndex].finished || phase_ == Phase::GameOver
        || players_[playerIndex].reward == SecretReward::None)
        return false;
    if (playerIndex == localPlayerId_ && currentPlayer_ != localPlayerId_)
        return false;

    switch (players_[playerIndex].reward) {
    case SecretReward::PeekCard:
        return phase_ == Phase::Decide && claim_.valid && !claim_.cards.isEmpty();
    case SecretReward::RankScout:
        return phase_ == Phase::Decide || phase_ == Phase::Play;
    case SecretReward::PileScout:
        return (phase_ == Phase::Decide || phase_ == Phase::Play) && !tablePile_.isEmpty();
    case SecretReward::GamblerAllIn:
        return phase_ == Phase::Decide && claim_.valid && challengeAllowed(playerIndex);
    case SecretReward::None:
        return false;
    }
    return false;
}

void QtWidgetsApplication1::handleAllInResult(int challenger, bool success)
{
    Player &player = players_[challenger];
    if (!success) {
        player.gamblingBan = true;
        addLog(QStringLiteral("%1 的梭哈失败：在完成下一次真实出牌前不能质疑，且下一次声明必须真实。")
                   .arg(player.name));
        if (challenger == localPlayerId_) {
            QMessageBox::information(this, QStringLiteral("梭哈失败"),
                QStringLiteral("你已进入禁赌状态：\n"
                               "1. 完成下一次出牌前不能质疑；\n"
                               "2. 下一次出牌必须作出完全真实的声明。"));
        } else if (mode_ == GameMode::Host && host_) {
            QJsonObject result;
            result.insert(QStringLiteral("kind"), QStringLiteral("allInFailed"));
            host_->sendTo(challenger, Protocol::Msg::RewardResult, result);
        }
        return;
    }

    QVector<int> candidates;
    QStringList names;
    for (int i = 0; i < players_.size(); ++i) {
        if (i != challenger && !players_[i].finished) {
            candidates.append(i);
            names << players_[i].name;
        }
    }
    if (candidates.isEmpty())
        return;

    int target = candidates[QRandomGenerator::global()->bounded(static_cast<int>(candidates.size()))];
    if (challenger == localPlayerId_) {
        bool accepted = false;
        const QString chosen = QInputDialog::getItem(this, QStringLiteral("梭哈成功"),
            QStringLiteral("选择一名玩家，秘密查看其当前全部手牌："), names, 0, false, &accepted);
        const int chosenIndex = names.indexOf(chosen);
        if (accepted && chosenIndex >= 0 && chosenIndex < candidates.size())
            target = candidates[chosenIndex];
    }

    QVector<int> hand = players_[target].hand;
    std::sort(hand.begin(), hand.end());
    QStringList cards;
    for (int card : hand)
        cards << cardName(card);
    addLog(QStringLiteral("%1 梭哈成功，秘密查看了 %2 当前的全部手牌。")
               .arg(player.name, players_[target].name));
    if (challenger == localPlayerId_) {
        QMessageBox::information(this, QStringLiteral("梭哈情报"),
            QStringLiteral("%1 当前的全部手牌：\n[ %2 ]\n\n这条情报只代表当前时刻。")
                .arg(players_[target].name, cards.join(QStringLiteral("、"))));
    } else if (mode_ == GameMode::Host && host_) {
        QJsonObject result;
        result.insert(QStringLiteral("kind"), QStringLiteral("allInResult"));
        result.insert(QStringLiteral("targetName"), players_[target].name);
        result.insert(QStringLiteral("hand"), cards.join(QStringLiteral("、")));
        host_->sendTo(challenger, Protocol::Msg::RewardResult, result);
    }
}

void QtWidgetsApplication1::showTableCards(const QVector<int> &cards, bool revealed)
{
    const int visibleCount = std::min(3, static_cast<int>(cards.size()));
    if (claim_.valid && claim_.declarer >= 0 && claim_.declarer < players_.size()) {
        ui.lastPlayPlayerLabel->setText(QStringLiteral("玩家%1出牌").arg(claim_.declarer + 1));
        claimLabel_->setText(QStringLiteral("%1张%2")
            .arg(claim_.cards.size()).arg(cardName(claim_.declaredRank)));
        ui.lastPlayFrame->show();
    }
    for (int i = 0; i < 3; ++i) {
        QLabel *cardLabel = tableCardLabels_[i];
        if (!cardLabel)
            continue;
        if (i >= visibleCount) {
            cardLabel->hide();
            continue;
        }

        cardLabel->show();
        if (revealed) {
            const bool joker = cards[i] == JokerRank;
            cardLabel->setText(joker
                ? QStringLiteral("JOKER\n🐯")
                : QStringLiteral("%1\n虎").arg(cardName(cards[i])));
            cardLabel->setStyleSheet(joker
                ? "background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #fff2ce,stop:1 #d9c092);"
                  "border:4px solid #8a3d67;border-radius:9px;color:#7d174e;font-family:'Georgia';font-size:19px;font-weight:bold;"
                : "background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #fff1cb,stop:1 #d5bd90);"
                  "border:4px solid #6f5130;border-radius:9px;color:#21150d;font-family:'Georgia';font-size:25px;font-weight:bold;");
        } else {
            cardLabel->setText(QStringLiteral("虎\n牌"));
            cardLabel->setStyleSheet(
                "background:qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #263d55,stop:0.5 #10263b,stop:1 #07131f);"
                "border:4px double #b78b48;border-radius:9px;color:#e5c47e;font-family:'STKaiti','KaiTi';font-size:19px;font-weight:bold;");
        }
    }

    if (tableCardStatusLabel_) {
        tableCardStatusLabel_->setText(revealed ? QStringLiteral("已揭牌") : QStringLiteral("暗牌"));
        tableCardStatusLabel_->setStyleSheet(revealed
            ? "color:#efb279;font-size:12px;font-weight:bold;background:transparent;border:none;"
            : "color:#9e875f;font-size:12px;background:transparent;border:none;");
    }
}

void QtWidgetsApplication1::clearTableCards()
{
    for (QLabel *cardLabel : tableCardLabels_) {
        if (cardLabel)
            cardLabel->hide();
    }
    if (tableCardStatusLabel_) {
        tableCardStatusLabel_->setText(QString());
        tableCardStatusLabel_->setStyleSheet(
            "color:#9e875f;font-size:12px;background:transparent;border:none;");
    }
    ui.lastPlayFrame->hide();
}

void QtWidgetsApplication1::showTableAction(const QString &text, bool important)
{
    tableActionText_ = text;
    tableActionImportant_ = important;
    if (!actionLabel_)
        return;

    QString compactText = tableActionText_;
    compactText.replace(QLatin1Char('\n'), QStringLiteral(" · "));
    actionLabel_->setText(compactText);
    actionLabel_->setStyleSheet(tableActionImportant_
        ? "background:transparent;border:none;color:#efb071;font-size:13px;font-weight:bold;padding:2px;"
        : "background:transparent;border:none;color:#bda06d;font-size:13px;padding:2px;");
}

void QtWidgetsApplication1::startAiProgress(int durationMs, const QString &stageText)
{
    stopAiProgress();
    if (!aiProgressBar_ || !aiProgressTimer_)
        return;

    aiProgressDurationMs_ = std::max(100, durationMs);
    aiProgressElapsedMs_ = 0;
    aiProgressStageText_ = stageText;
    aiProgressBar_->setRange(0, aiProgressDurationMs_);
    aiProgressBar_->setValue(0);
    aiProgressBar_->setFormat(QStringLiteral("⏳ %1　剩余 %2 秒")
        .arg(aiProgressStageText_)
        .arg(aiProgressDurationMs_ / 1000.0, 0, 'f', 1));
    aiProgressBar_->show();
    aiProgressTimer_->start();
}

void QtWidgetsApplication1::stopAiProgress()
{
    if (aiProgressTimer_)
        aiProgressTimer_->stop();
    if (aiProgressBar_)
        aiProgressBar_->hide();
    aiProgressDurationMs_ = 0;
    aiProgressElapsedMs_ = 0;
    aiProgressStageText_.clear();
}

void QtWidgetsApplication1::showRankPopup(int playerIndex)
{
    if (playerIndex < 0 || playerIndex >= players_.size())
        return;
    const Player &player = players_[playerIndex];
    if (player.rank <= 0)
        return;

    if (playerIndex == localPlayerId_) {
        QMessageBox::information(this, QStringLiteral("恭喜获得名次"),
            QStringLiteral("恭喜！你已经出完全部手牌，获得本局第 %1 名。\n\n"
                           "点击“确定”后将进入观战模式，继续查看电脑玩家的后续决策。")
                .arg(player.rank));
    } else {
        QMessageBox::information(this, QStringLiteral("新名次产生"),
            QStringLiteral("%1 已经出完全部手牌，获得本局第 %2 名！\n\n"
                           "点击“确定”继续游戏。")
                .arg(player.name).arg(player.rank));
    }
}

void QtWidgetsApplication1::updateUi()
{
    if (players_.isEmpty())
        return;

    const bool validCurrentPlayer = currentPlayer_ >= 0 && currentPlayer_ < players_.size();
    glowingPlayer_ = (!validCurrentPlayer || phase_ == Phase::GameOver
        || players_[currentPlayer_].finished) ? -1 : currentPlayer_;

    // 显示席 0 = 本地玩家自己，1..3 = 依次的其他玩家（联机时按 localPlayerId_ 旋转）。
    for (int d = 0; d < PlayerCount; ++d) {
        const int pid = playerAtDisplaySeat(d);
        const Player &player = players_[pid];
        const bool activeNow = currentPlayer_ == pid && !player.finished && phase_ != Phase::GameOver;
        playerNameLabels_[d]->setText(d == 0
            ? QStringLiteral("玩家%1（你）").arg(pid + 1)
            : QStringLiteral("玩家%1").arg(pid + 1));
        playerCountLabels_[d]->setText(QString::number(player.hand.size()));
        playerRankBadges_[d]->setText(QString::number(player.rank));
        playerRankBadges_[d]->setVisible(player.rank > 0);
        if (player.finished) {
            playerSeatFrames_[d]->setStyleSheet(
                "background:rgba(11,10,8,205);border:1px solid #413a30;border-radius:13px;");
            playerAvatarLabels_[d]->setStyleSheet(
                "background:#26231f;border:3px solid #514a40;border-radius:34px;color:#777067;font-size:33px;");
            playerNameLabels_[d]->setStyleSheet("color:#777067;font-size:15px;font-weight:bold;");
            playerCountLabels_[d]->setStyleSheet("color:#766d60;font-family:'Georgia';font-size:27px;font-weight:bold;");
        } else if (activeNow) {
            playerSeatFrames_[d]->setStyleSheet(
                "background:rgba(67,38,13,230);border:2px solid #d3a246;border-radius:13px;");
            playerAvatarLabels_[d]->setStyleSheet(
                "background:qradialgradient(cx:.5,cy:.45,radius:.65,stop:0 #8b571f,stop:1 #281506);"
                "border:5px double #f1bf57;border-radius:34px;color:#ffe7a7;font-size:33px;");
            playerNameLabels_[d]->setStyleSheet("color:#ffe0a0;font-size:15px;font-weight:bold;");
            playerCountLabels_[d]->setStyleSheet("color:#ffe09a;font-family:'Georgia';font-size:27px;font-weight:bold;");
        } else {
            playerSeatFrames_[d]->setStyleSheet(
                "background:rgba(10,7,4,218);border:1px solid rgba(118,83,40,170);border-radius:13px;");
            playerAvatarLabels_[d]->setStyleSheet(
                "background:qradialgradient(cx:.5,cy:.45,radius:.65,stop:0 #6e431c,stop:1 #201208);"
                "border:3px solid #7f592b;border-radius:34px;color:#ffe1a0;font-size:33px;");
            playerNameLabels_[d]->setStyleSheet("color:#f2dab0;font-size:15px;font-weight:bold;");
            playerCountLabels_[d]->setStyleSheet("color:#f5cf82;font-family:'Georgia';font-size:27px;font-weight:bold;");
        }
    }

    const Player &human = players_[localPlayerId_];
    rankingLabel_->setText(finishOrder_.isEmpty()
        ? QStringLiteral("当前排名：尚未有人出完手牌")
        : QStringLiteral("当前排名：%1").arg(rankingSummary()));

    if (human.task == SecretTask::None) {
        taskLabel_->setText(QStringLiteral("任务已全部完成"));
    } else {
        taskLabel_->setText(QStringLiteral("任务：%1\n%2")
            .arg(taskName(human.task), taskDescription(human.task)));
    }
    if (human.reward == SecretReward::None) {
        rewardLabel_->setText(QStringLiteral("暂无奖励"));
        rewardButton_->setText(QStringLiteral("使用秘密奖励"));
    } else {
        QString pendingText;
        if (human.pendingRewards > 0)
            pendingText = QStringLiteral("　另有 %1 份待领取").arg(human.pendingRewards);
        rewardLabel_->setText(QStringLiteral("奖励：%1%2\n%3")
                                  .arg(rewardName(human.reward))
                                  .arg(pendingText)
                                  .arg(rewardDescription(human.reward)));
        rewardButton_->setText(QStringLiteral("使用：%1").arg(rewardName(human.reward)));
    }

    const bool showingReward = human.reward != SecretReward::None;
    ui.privateInfoTitle->setText(showingReward ? QStringLiteral("🎁  奖励") : QStringLiteral("📜  任务"));
    taskLabel_->setVisible(!showingReward);
    rewardLabel_->setVisible(showingReward);
    rewardButton_->setVisible(showingReward);

    QString eventEffect;
    switch (currentEvent_) {
    case TavernEvent::BartenderRush:
        eventEffect = QStringLiteral("本轮每次必须出3张");
        break;
    case TavernEvent::DrinkBeforeCatch:
        eventEffect = QStringLiteral("牌堆不足%1张时不可质疑").arg(DrinkChallengePile);
        break;
    case TavernEvent::FinalTable:
        eventEffect = QStringLiteral("每次至少出2张");
        break;
    case TavernEvent::ClosingTime:
        eventEffect = QStringLiteral("每次至少出2张");
        break;
    case TavernEvent::None:
        break;
    }
    const int eventCode = static_cast<int>(currentEvent_);
    if (currentEvent_ == TavernEvent::None) {
        ui.eventBannerFrame->hide();
        ui.eventStatusLabel->hide();
        presentedEventCode_ = -1;
    } else {
        const QString eventTitle = eventName(currentEvent_);
        eventLabel_->setText(QStringLiteral("🍺  酒馆事件 · %1    %2").arg(eventTitle, eventEffect));
        ui.eventStatusLabel->setText(QStringLiteral("%1 · %2").arg(eventTitle, eventEffect));
        if (presentedEventCode_ != eventCode) {
            presentedEventCode_ = eventCode;
            ui.eventStatusLabel->hide();
            ui.eventBannerFrame->show();
            QTimer::singleShot(1800, this, [this, eventCode] {
                if (static_cast<int>(currentEvent_) == eventCode) {
                    ui.eventBannerFrame->hide();
                    ui.eventStatusLabel->show();
                }
            });
        } else if (!ui.eventBannerFrame->isVisible()) {
            ui.eventStatusLabel->show();
        }
    }

    ui.pileCountLabel->setText(QStringLiteral("%1张").arg(tablePile_.size()));
    if (claim_.valid) {
        ui.lastPlayPlayerLabel->setText(QStringLiteral("玩家%1出牌").arg(claim_.declarer + 1));
        claimLabel_->setText(QStringLiteral("%1张%2")
            .arg(claim_.cards.size()).arg(cardName(claim_.declaredRank)));
        ui.lastPlayFrame->show();
    }

    const bool humanTurn = currentPlayer_ == localPlayerId_ && !human.finished;
    believeButton_->setEnabled(humanTurn && phase_ == Phase::Decide);
    challengeButton_->setEnabled(humanTurn && phase_ == Phase::Decide && challengeAllowed(localPlayerId_));
    rewardButton_->setEnabled(rewardUsable(localPlayerId_));
    playButton_->setEnabled(humanTurn && phase_ == Phase::Play && !human.hand.isEmpty());
    rankCombo_->setEnabled(humanTurn && phase_ == Phase::Play && !human.hand.isEmpty());
    const bool declaring = humanTurn && phase_ == Phase::Play && !human.hand.isEmpty();
    const bool deciding = humanTurn && phase_ == Phase::Decide;
    ui.rankPromptLabel->setVisible(declaring);
    rankCombo_->setVisible(declaring);
    playButton_->setVisible(declaring);
    believeButton_->setVisible(deciding);
    challengeButton_->setVisible(deciding);
    restartButton_->setVisible(phase_ == Phase::GameOver && mode_ != GameMode::Client);
    rebuildHandButtons();

    if (mode_ == GameMode::Host && host_)
        QTimer::singleShot(0, this, [this] { if (mode_ == GameMode::Host && host_) broadcastState(); });
}

void QtWidgetsApplication1::rebuildHandButtons()
{
    while (QLayoutItem *item = handLayout_->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    if (players_.isEmpty())
        return;
    if (selected_.size() != players_[localPlayerId_].hand.size())
        selected_.fill(false, players_[localPlayerId_].hand.size());

    for (int i = 0; i < players_[localPlayerId_].hand.size(); ++i) {
        const int card = players_[localPlayerId_].hand[i];
        auto *button = new QPushButton(card == JokerRank
            ? QStringLiteral("JOKER\n🐯")
            : QStringLiteral("%1\n虎").arg(cardName(card)));
        button->setCheckable(true);
        button->setChecked(selected_[i]);
        button->setEnabled(currentPlayer_ == localPlayerId_ && phase_ == Phase::Play && !players_[localPlayerId_].finished);
        button->setMinimumSize(62, 112);
        button->setMaximumWidth(76);
        button->setStyleSheet(
            "QPushButton{font-family:'Georgia','Microsoft YaHei UI';font-size:21px;font-weight:bold;"
            "background:qlineargradient(x1:0,y1:0,x2:0,y2:1,stop:0 #f8e8bd,stop:.72 #ddc69a,stop:1 #b99c6e);"
            "color:#25170d;border:3px solid #62482f;border-radius:8px;padding:6px 3px;}"
            "QPushButton:hover{background:#fff0c7;border-color:#d2a34d;padding-bottom:10px;}"
            "QPushButton:checked{background:#e8ba5e;border:4px solid #ffe09a;padding-bottom:12px;}"
            "QPushButton:disabled{background:#756b5b;color:#3e382f;border-color:#514738;}");
        connect(button, &QPushButton::toggled, this, [this, i](bool checked) {
            if (checked) {
                int count = 0;
                for (bool value : selected_)
                    if (value)
                        ++count;
                if (count >= MaxPlayCards) {
                    if (auto *clickedButton = qobject_cast<QPushButton *>(sender()))
                        clickedButton->setChecked(false);
                    return;
                }
            }
            selected_[i] = checked;
        });
        handLayout_->addWidget(button, i / HandColumns, i % HandColumns);
    }
}

void QtWidgetsApplication1::setPhase(Phase phase, const QString &text)
{
    stopAiProgress();
    phase_ = phase;
    phaseLabel_->setText(text);
    if (text.contains(QStringLiteral("电脑决策阶段"))) {
        decisionActionPanel_->setStyleSheet(
            "QFrame#decisionActionPanel{background:rgba(61,35,12,215);border:2px solid #c8963d;border-radius:9px;}");
        phaseLabel_->setStyleSheet(
            "background:transparent;border:none;font-size:15px;font-weight:bold;color:#ffe0a0;padding:3px;");
    } else if (text.contains(QStringLiteral("电脑决策结果"))) {
        decisionActionPanel_->setStyleSheet(
            "QFrame#decisionActionPanel{background:rgba(28,55,37,215);border:2px solid #6f9865;border-radius:9px;}");
        phaseLabel_->setStyleSheet(
            "background:transparent;border:none;font-size:15px;font-weight:bold;color:#dce9c8;padding:3px;");
    } else if (text.contains(QStringLiteral("揭牌"))) {
        decisionActionPanel_->setStyleSheet(
            "QFrame#decisionActionPanel{background:rgba(67,28,18,220);border:2px solid #c86f45;border-radius:9px;}");
        phaseLabel_->setStyleSheet(
            "background:transparent;border:none;font-size:15px;font-weight:bold;color:#f3c59f;padding:3px;");
    } else {
        decisionActionPanel_->setStyleSheet(
            "QFrame#decisionActionPanel{background:rgba(10,6,3,180);border:1px solid rgba(143,99,45,155);border-radius:9px;}");
        phaseLabel_->setStyleSheet(
            "background:transparent;border:none;font-size:15px;font-weight:bold;color:#f1d49a;padding:3px;");
    }
}

void QtWidgetsApplication1::addLog(const QString &text)
{
    logEdit_->append(QStringLiteral("• ") + text);
    logEdit_->verticalScrollBar()->setValue(logEdit_->verticalScrollBar()->maximum());
    if (mode_ == GameMode::Host && host_) {
        QJsonObject payload;
        payload.insert(QStringLiteral("text"), text);
        host_->broadcast(Protocol::Msg::Log, payload);
    }
}

int QtWidgetsApplication1::nextActive(int from) const
{
    for (int step = 1; step <= players_.size(); ++step) {
        const int candidate = (from + step) % players_.size();
        if (!players_[candidate].finished)
            return candidate;
    }
    return from;
}

bool QtWidgetsApplication1::claimIsTrue() const
{
    return claim_.valid && cardsMatchClaim(claim_.cards, claim_.declaredRank);
}

bool QtWidgetsApplication1::cardsMatchClaim(const QVector<int> &cards, int declaredRank) const
{
    if (cards.isEmpty() || declaredRank < 0 || declaredRank >= RankCount)
        return false;
    for (int card : cards)
        if (card != declaredRank && card != JokerRank)
            return false;
    return true;
}

bool QtWidgetsApplication1::isLegalPlaySelection(
    int playerIndex, const QVector<int> &indices, int declaredRank) const
{
    if (playerIndex < 0 || playerIndex >= players_.size()
        || players_[playerIndex].finished
        || declaredRank < 0 || declaredRank >= RankCount
        || indices.isEmpty()) {
        return false;
    }

    const Player &player = players_[playerIndex];
    const int minimum = minimumPlayCount(playerIndex);
    const int maximum = maximumPlayCount(playerIndex);
    if (indices.size() < minimum || indices.size() > maximum)
        return false;

    QVector<bool> seen;
    seen.fill(false, player.hand.size());
    QVector<int> cards;
    cards.reserve(indices.size());
    for (int index : indices) {
        if (index < 0 || index >= player.hand.size() || seen[index])
            return false;
        seen[index] = true;
        cards.append(player.hand[index]);
    }

    return !player.gamblingBan || cardsMatchClaim(cards, declaredRank);
}

QString QtWidgetsApplication1::cardName(int rank) const
{
    static const QString names[] = {
        QStringLiteral("8"), QStringLiteral("9"), QStringLiteral("10"),
        QStringLiteral("J"), QStringLiteral("Q"), QStringLiteral("K"),
        QStringLiteral("A"), QStringLiteral("Joker")
    };
    return (rank >= 0 && rank <= JokerRank) ? names[rank] : QStringLiteral("?");
}

QString QtWidgetsApplication1::playerStatusText(const Player &player) const
{
    if (player.rank > 0)
        return QStringLiteral("第 %1 名").arg(player.rank);
    if (player.hand.isEmpty())
        return QStringLiteral("等待最后声明确认");
    if (player.gamblingBan)
        return QStringLiteral("进行中·禁赌");
    return QStringLiteral("进行中");
}

QString QtWidgetsApplication1::rankingSummary() const
{
    QStringList entries;
    for (int playerIndex : finishOrder_) {
        const Player &player = players_[playerIndex];
        entries << QStringLiteral("第%1名：%2").arg(player.rank).arg(player.name);
    }
    return entries.join(QStringLiteral("　"));
}

QString QtWidgetsApplication1::eventName(TavernEvent event) const
{
    switch (event) {
    case TavernEvent::BartenderRush: return QStringLiteral("酒保催单");
    case TavernEvent::DrinkBeforeCatch: return QStringLiteral("先喝再抓");
    case TavernEvent::FinalTable: return QStringLiteral("最后一桌");
    case TavernEvent::ClosingTime: return QStringLiteral("酒馆打烊");
    case TavernEvent::None: return QStringLiteral("无");
    }
    return QStringLiteral("无");
}

QString QtWidgetsApplication1::eventDescription(TavernEvent event) const
{
    switch (event) {
    case TavernEvent::BartenderRush:
        return QStringLiteral("触发：最近 %1 次出牌中有至少 %2 次不超过 %3 张。本轮每次必须打出 %4 张；手牌不足时全部打出。")
            .arg(ConservativePlayWindow).arg(ConservativePlayNeed)
            .arg(ConservativePlayMaxCards).arg(MaxPlayCards);
    case TavernEvent::DrinkBeforeCatch:
        return QStringLiteral("触发：连续 %1 轮的结算牌堆都不超过 %2 张。中央牌池达到 %3 张前禁止质疑；上家打出最后一手时例外。")
            .arg(SmallPileRounds).arg(SmallPileMax).arg(DrinkChallengePile);
    case TavernEvent::FinalTable:
        return QStringLiteral("剩余两人每次至少打出 2 张；仅剩 1 张时可正常打出。");
    case TavernEvent::ClosingTime:
        return QStringLiteral("触发：连续 %1 次合法出牌没有产生新名次。直到产生下一名次前，每次至少打出 2 张；仅剩 1 张时可正常打出。")
            .arg(ClosingTimePlays);
    case TavernEvent::None:
        return QStringLiteral("按基础规则每次出 1–%1 张牌。").arg(MaxPlayCards);
    }
    return {};
}

QString QtWidgetsApplication1::taskName(SecretTask task) const
{
    switch (task) {
    case SecretTask::DeceiveAtRisk: return QStringLiteral("瞒天过海");
    case SecretTask::BaitChallenge: return QStringLiteral("请你抓我");
    case SecretTask::BoldChallenge: return QStringLiteral("胆大包天");
    case SecretTask::DesperateBluff: return QStringLiteral("孤注一掷");
    case SecretTask::None: return QStringLiteral("无");
    }
    return QStringLiteral("无");
}

QString QtWidgetsApplication1::taskDescription(SecretTask task) const
{
    switch (task) {
    case SecretTask::DeceiveAtRisk:
        return QStringLiteral("出牌前中央牌堆至少 %1 张，一次打出至少 2 张并虚假声明，让下一家相信。")
            .arg(DeceivePileRequirement);
    case SecretTask::BaitChallenge:
        return QStringLiteral("出牌前中央牌堆至少 %1 张，一次打出至少 2 张并完全真实声明，让下一家质疑失败。")
            .arg(BaitPileRequirement);
    case SecretTask::BoldChallenge:
        return QStringLiteral("中央牌堆达到至少 %1 张时主动质疑，并成功识破上一家的谎言。")
            .arg(BoldPileRequirement);
    case SecretTask::DesperateBluff:
        return QStringLiteral("出牌前手中剩余 %1–%2 张，一次打出 %3 张并虚假声明，让下一家相信。")
            .arg(DesperateHandMin).arg(DesperateHandMax).arg(MaxPlayCards);
    case SecretTask::None:
        return QStringLiteral("本局不再获得新任务。");
    }
    return {};
}

QString QtWidgetsApplication1::rewardName(SecretReward reward) const
{
    switch (reward) {
    case SecretReward::PeekCard: return QStringLiteral("窥牌");
    case SecretReward::RankScout: return QStringLiteral("点数侦查");
    case SecretReward::PileScout: return QStringLiteral("牌堆侦察");
    case SecretReward::GamblerAllIn: return QStringLiteral("赌徒·梭哈");
    case SecretReward::None: return QStringLiteral("无");
    }
    return QStringLiteral("无");
}

QString QtWidgetsApplication1::rewardDescription(SecretReward reward) const
{
    switch (reward) {
    case SecretReward::PeekCard:
        return QStringLiteral("判断时随机查看上一家刚打出的 1 张牌，再正常选择相信或质疑。");
    case SecretReward::RankScout:
        return QStringLiteral("选择一名在局玩家和一种牌型，秘密获知其当前准确持有数量。");
    case SecretReward::PileScout:
        return QStringLiteral("中央牌堆非空时选择一种牌型，秘密获知牌堆中的准确数量。");
    case SecretReward::GamblerAllIn:
        return QStringLiteral("发动后必须质疑；成功可查看一名玩家全部手牌，失败则进入禁赌状态。");
    case SecretReward::None:
        return QStringLiteral("暂无可用奖励。");
    }
    return {};
}

// ======================= 联机同步与网络 =======================

int QtWidgetsApplication1::playerAtDisplaySeat(int displaySeat) const
{
    if (players_.isEmpty())
        return 0;
    if (displaySeat <= 0)
        return localPlayerId_;
    int idx = 1;
    for (int pid = 0; pid < players_.size(); ++pid) {
        if (pid == localPlayerId_)
            continue;
        if (idx == displaySeat)
            return pid;
        ++idx;
    }
    return localPlayerId_;
}

bool QtWidgetsApplication1::isAiSeat(int playerIndex) const
{
    if (mode_ == GameMode::LocalVsAi)
        return playerIndex != localPlayerId_;
    if (mode_ == GameMode::Host) {
        if (playerIndex == 0)
            return false; // 房主本人
        if (playerIndex < 0 || playerIndex >= aiSeat_.size())
            return false;
        return aiSeat_[playerIndex];
    }
    return false; // 客户端不运行本地 AI
}

void QtWidgetsApplication1::broadcastState()
{
    if (mode_ != GameMode::Host || !host_)
        return;
    for (int i = 1; i < PlayerCount; ++i)
        if (host_->isConnected(i))
            sendStateTo(i);
}

void QtWidgetsApplication1::sendStateTo(int playerId)
{
    if (!host_ || !host_->isConnected(playerId))
        return;
    host_->sendTo(playerId, Protocol::Msg::State, buildStateSnapshot(playerId));
}

QJsonObject QtWidgetsApplication1::buildStateSnapshot(int forPlayerId) const
{
    QJsonObject pub;

    QJsonArray playersArr;
    for (int i = 0; i < players_.size(); ++i) {
        const Player &p = players_[i];
        QJsonObject o;
        o.insert(QStringLiteral("name"), p.name);
        o.insert(QStringLiteral("finished"), p.finished);
        o.insert(QStringLiteral("rank"), p.rank);
        o.insert(QStringLiteral("handSize"), p.hand.size());
        playersArr.append(o);
    }
    pub.insert(QStringLiteral("players"), playersArr);

    QJsonObject claimObj;
    claimObj.insert(QStringLiteral("valid"), claim_.valid);
    claimObj.insert(QStringLiteral("declarer"), claim_.declarer);
    claimObj.insert(QStringLiteral("declaredRank"), claim_.declaredRank);
    claimObj.insert(QStringLiteral("count"), claim_.cards.size());
    if (claim_.valid && claimRevealed_) {
        QJsonArray cards;
        for (int card : claim_.cards)
            cards.append(card);
        claimObj.insert(QStringLiteral("cards"), cards);
    }
    pub.insert(QStringLiteral("claim"), claimObj);

    pub.insert(QStringLiteral("tablePileSize"), tablePile_.size());
    pub.insert(QStringLiteral("currentPlayer"), currentPlayer_);
    pub.insert(QStringLiteral("phase"), static_cast<int>(phase_));
    pub.insert(QStringLiteral("event"), static_cast<int>(currentEvent_));
    pub.insert(QStringLiteral("round"), roundNumber_);
    QJsonArray finishArr;
    for (int id : finishOrder_)
        finishArr.append(id);
    pub.insert(QStringLiteral("finishOrder"), finishArr);
    pub.insert(QStringLiteral("tableAction"), tableActionText_);
    pub.insert(QStringLiteral("tableActionImportant"), tableActionImportant_);

    QJsonObject priv;
    if (forPlayerId >= 0 && forPlayerId < players_.size()) {
        const Player &me = players_[forPlayerId];
        QJsonArray hand;
        for (int card : me.hand)
            hand.append(card);
        priv.insert(QStringLiteral("hand"), hand);
        priv.insert(QStringLiteral("task"), static_cast<int>(me.task));
        priv.insert(QStringLiteral("reward"), static_cast<int>(me.reward));
        priv.insert(QStringLiteral("completedTasks"), me.completedTasks);
        priv.insert(QStringLiteral("pendingRewards"), me.pendingRewards);
        priv.insert(QStringLiteral("gamblingBan"), me.gamblingBan);
    }

    QJsonObject state;
    state.insert(QStringLiteral("gameId"), gameId_);
    state.insert(QStringLiteral("public"), pub);
    state.insert(QStringLiteral("private"), priv);
    return state;
}

void QtWidgetsApplication1::applyStateSnapshot(const QJsonObject &state)
{
    const QJsonObject pub = state.value(QStringLiteral("public")).toObject();
    const QJsonObject priv = state.value(QStringLiteral("private")).toObject();

    const QJsonArray playersArr = pub.value(QStringLiteral("players")).toArray();
    const int count = playersArr.size();
    if (count <= 0)
        return;

    QVector<Player> newPlayers(count);
    for (int i = 0; i < count; ++i) {
        const QJsonObject o = playersArr[i].toObject();
        Player &np = newPlayers[i];
        np.name = o.value(QStringLiteral("name")).toString();
        np.finished = o.value(QStringLiteral("finished")).toBool();
        np.rank = o.value(QStringLiteral("rank")).toInt();
        np.hand.resize(o.value(QStringLiteral("handSize")).toInt());
    }

    // 本地玩家私有信息
    if (localPlayerId_ >= 0 && localPlayerId_ < newPlayers.size()) {
        Player &me = newPlayers[localPlayerId_];
        const QJsonArray handArr = priv.value(QStringLiteral("hand")).toArray();
        QVector<int> hand;
        hand.reserve(handArr.size());
        for (const QJsonValue &v : handArr)
            hand.append(v.toInt());
        me.hand = hand;
        me.task = static_cast<SecretTask>(priv.value(QStringLiteral("task")).toInt());
        me.reward = static_cast<SecretReward>(priv.value(QStringLiteral("reward")).toInt());
        me.completedTasks = priv.value(QStringLiteral("completedTasks")).toInt();
        me.pendingRewards = priv.value(QStringLiteral("pendingRewards")).toInt();
        me.gamblingBan = priv.value(QStringLiteral("gamblingBan")).toBool();
    }

    players_ = newPlayers;

    // 声明镜像
    claim_ = Claim{};
    const QJsonObject claimObj = pub.value(QStringLiteral("claim")).toObject();
    claim_.valid = claimObj.value(QStringLiteral("valid")).toBool();
    claim_.declarer = claimObj.value(QStringLiteral("declarer")).toInt(-1);
    claim_.declaredRank = claimObj.value(QStringLiteral("declaredRank")).toInt();
    const int claimCount = claimObj.value(QStringLiteral("count")).toInt();
    const QJsonArray cardsArr = claimObj.value(QStringLiteral("cards")).toArray();
    if (!cardsArr.isEmpty()) {
        for (const QJsonValue &v : cardsArr)
            claim_.cards.append(v.toInt());
        claimRevealed_ = true;
    } else {
        claim_.cards.resize(claimCount);
        claimRevealed_ = false;
    }

    tablePile_.resize(pub.value(QStringLiteral("tablePileSize")).toInt());
    currentPlayer_ = pub.value(QStringLiteral("currentPlayer")).toInt();
    phase_ = static_cast<Phase>(pub.value(QStringLiteral("phase")).toInt());
    currentEvent_ = static_cast<TavernEvent>(pub.value(QStringLiteral("event")).toInt());
    roundNumber_ = pub.value(QStringLiteral("round")).toInt();

    finishOrder_.clear();
    const QJsonArray finishArr = pub.value(QStringLiteral("finishOrder")).toArray();
    for (const QJsonValue &v : finishArr)
        finishOrder_.append(v.toInt());

    // 本地玩家视角的阶段提示
    QString phaseText;
    if (phase_ == Phase::GameOver) {
        phaseText = QStringLiteral("对局结束");
    } else if (currentPlayer_ == localPlayerId_ && !players_[localPlayerId_].finished) {
        if (phase_ == Phase::Decide && claim_.valid
            && claim_.declarer >= 0 && claim_.declarer < players_.size()) {
            if (players_[localPlayerId_].gamblingBan)
                phaseText = QStringLiteral("【禁赌状态】你在完成下一次真实出牌前不能质疑，本次只能相信。");
            else if (challengeAllowed(localPlayerId_))
                phaseText = QStringLiteral("轮到你判断：相信 %1，还是质疑并揭牌？").arg(players_[claim_.declarer].name);
            else
                phaseText = QStringLiteral("【先喝再抓】中央牌池不足 %1 张，本次只能相信。").arg(DrinkChallengePile);
        } else {
            phaseText = QStringLiteral("轮到你出牌，选择手牌并声明牌型。");
        }
    } else if (players_[localPlayerId_].finished) {
        phaseText = QStringLiteral("【观战模式】等待其他玩家操作……");
    } else {
        phaseText = QStringLiteral("等待 %1 操作……").arg(
            (currentPlayer_ >= 0 && currentPlayer_ < players_.size())
                ? players_[currentPlayer_].name : QStringLiteral("玩家"));
    }

    setPhase(phase_, phaseText);
    showTableAction(pub.value(QStringLiteral("tableAction")).toString(),
                    pub.value(QStringLiteral("tableActionImportant")).toBool());
    if (claim_.valid)
        showTableCards(claim_.cards, claimRevealed_);
    else
        clearTableCards();

    const bool myTurn = currentPlayer_ == localPlayerId_ && !players_[localPlayerId_].finished;
    if (myTurn || phase_ == Phase::GameOver)
        stopAiProgress();
    else if (currentPlayer_ >= 0 && currentPlayer_ < players_.size())
        startWaitProgress(QStringLiteral("等待 %1 操作").arg(players_[currentPlayer_].name));

    updateUi();
}

void QtWidgetsApplication1::onHostMessage(int playerId, const QString &type, const QJsonObject &payload)
{
    if (type == Protocol::Msg::Sync) {
        sendStateTo(playerId);
        return;
    }

    if (type == Protocol::Msg::Action) {
        if (playerId != currentPlayer_)
            return; // 非当前行动玩家，忽略乱序/过期操作
        const QString kind = payload.value(QStringLiteral("kind")).toString();
        if (kind == QStringLiteral("play")) {
            const QJsonArray idxArr = payload.value(QStringLiteral("indices")).toArray();
            QVector<int> indices;
            indices.reserve(idxArr.size());
            for (const QJsonValue &v : idxArr)
                indices.append(v.toInt());
            const int declaredRank = payload.value(QStringLiteral("declaredRank")).toInt();
            doPlay(playerId, indices, declaredRank);
        } else if (kind == QStringLiteral("believe")) {
            doBelieve(playerId);
        } else if (kind == QStringLiteral("challenge")) {
            doChallenge(playerId);
        } else if (kind == QStringLiteral("reward")) {
            handleRemoteReward(playerId, payload);
        }
        return;
    }
}

void QtWidgetsApplication1::onClientMessage(const QString &type, const QJsonObject &payload)
{
    if (type == Protocol::Msg::Start) {
        players_.clear();
        players_.resize(PlayerCount);
        for (int i = 0; i < PlayerCount; ++i)
            players_[i].name = QStringLiteral("玩家%1").arg(i + 1);
        finishOrder_.clear();
        tablePile_.clear();
        claim_ = Claim{};
        claimRevealed_ = false;
        logEdit_->clear();
        stopAiProgress();
        updateUi();
        return;
    }

    if (type == Protocol::Msg::State) {
        applyStateSnapshot(payload);
        return;
    }

    if (type == Protocol::Msg::Log) {
        addLog(payload.value(QStringLiteral("text")).toString());
        return;
    }

    if (type == Protocol::Msg::RewardResult) {
        const QString kind = payload.value(QStringLiteral("kind")).toString();
        if (kind == QStringLiteral("peek")) {
            QMessageBox::information(this, QStringLiteral("窥牌结果"),
                QStringLiteral("你随机看到上一家刚打出的 1 张牌：%1\n\n其余牌仍然隐藏，请继续选择相信或质疑。")
                    .arg(payload.value(QStringLiteral("card")).toString()));
        } else if (kind == QStringLiteral("rankScout")) {
            QMessageBox::information(this, QStringLiteral("点数侦查结果"),
                QStringLiteral("%1 当前手中有 %2 张 %3。\n\n这条情报只代表当前时刻。")
                    .arg(payload.value(QStringLiteral("targetName")).toString())
                    .arg(payload.value(QStringLiteral("count")).toInt())
                    .arg(payload.value(QStringLiteral("rank")).toString()));
        } else if (kind == QStringLiteral("pileScout")) {
            QMessageBox::information(this, QStringLiteral("牌堆侦察结果"),
                QStringLiteral("当前中央牌堆中共有 %1 张 %2。\n\n系统不会显示这些牌由谁打出。")
                    .arg(payload.value(QStringLiteral("count")).toInt())
                    .arg(payload.value(QStringLiteral("rank")).toString()));
        } else if (kind == QStringLiteral("allInResult")) {
            QMessageBox::information(this, QStringLiteral("梭哈情报"),
                QStringLiteral("%1 当前的全部手牌：\n[ %2 ]\n\n这条情报只代表当前时刻。")
                    .arg(payload.value(QStringLiteral("targetName")).toString(),
                         payload.value(QStringLiteral("hand")).toString()));
        } else if (kind == QStringLiteral("allInFailed")) {
            QMessageBox::information(this, QStringLiteral("梭哈失败"),
                QStringLiteral("你已进入禁赌状态：\n"
                               "1. 完成下一次出牌前不能质疑；\n"
                               "2. 下一次出牌必须作出完全真实的声明。"));
        }
        return;
    }

    if (type == Protocol::Msg::GameOver) {
        const QString result = payload.value(QStringLiteral("result")).toString();
        setPhase(Phase::GameOver, QStringLiteral("对局结束！%1").arg(result));
        QMessageBox::information(this, QStringLiteral("游戏结束"),
            QStringLiteral("最终排名\n%1").arg(result));
        updateUi();
        return;
    }

    if (type == Protocol::Msg::Error) {
        QMessageBox::warning(this, QStringLiteral("错误"),
            payload.value(QStringLiteral("message")).toString());
    }
}

void QtWidgetsApplication1::sendAction(const QString &kind, const QJsonObject &payload)
{
    if (mode_ != GameMode::Client || !client_)
        return;
    QJsonObject p = payload;
    p.insert(QStringLiteral("kind"), kind);
    client_->send(Protocol::Msg::Action, p);
}

void QtWidgetsApplication1::handleRemoteReward(int playerIndex, const QJsonObject &payload)
{
    if (mode_ != GameMode::Host || !host_)
        return;
    if (playerIndex != currentPlayer_)
        return;
    if (!rewardUsable(playerIndex))
        return;

    const SecretReward reward = players_[playerIndex].reward;
    const QString kind = payload.value(QStringLiteral("reward")).toString();

    if (reward == SecretReward::PeekCard && kind == QStringLiteral("peek")) {
        if (claim_.cards.isEmpty())
            return;
        const int index = QRandomGenerator::global()->bounded(static_cast<int>(claim_.cards.size()));
        const QString seenCard = cardName(claim_.cards[index]);
        consumeReward(playerIndex);
        QJsonObject result;
        result.insert(QStringLiteral("kind"), QStringLiteral("peek"));
        result.insert(QStringLiteral("card"), seenCard);
        host_->sendTo(playerIndex, Protocol::Msg::RewardResult, result);
    } else if (reward == SecretReward::RankScout && kind == QStringLiteral("rankScout")) {
        const int target = payload.value(QStringLiteral("target")).toInt(-1);
        const int rank = payload.value(QStringLiteral("rank")).toInt(-1);
        if (target < 0 || target >= players_.size() || target == playerIndex
            || players_[target].finished || rank < 0 || rank > JokerRank)
            return;
        int found = 0;
        for (int card : players_[target].hand)
            if (card == rank)
                ++found;
        consumeReward(playerIndex);
        QJsonObject result;
        result.insert(QStringLiteral("kind"), QStringLiteral("rankScout"));
        result.insert(QStringLiteral("targetName"), players_[target].name);
        result.insert(QStringLiteral("count"), found);
        result.insert(QStringLiteral("rank"), cardName(rank));
        host_->sendTo(playerIndex, Protocol::Msg::RewardResult, result);
    } else if (reward == SecretReward::PileScout && kind == QStringLiteral("pileScout")) {
        const int rank = payload.value(QStringLiteral("rank")).toInt(-1);
        if (rank < 0 || rank > JokerRank || tablePile_.isEmpty())
            return;
        int found = 0;
        for (int card : tablePile_)
            if (card == rank)
                ++found;
        consumeReward(playerIndex);
        QJsonObject result;
        result.insert(QStringLiteral("kind"), QStringLiteral("pileScout"));
        result.insert(QStringLiteral("count"), found);
        result.insert(QStringLiteral("rank"), cardName(rank));
        host_->sendTo(playerIndex, Protocol::Msg::RewardResult, result);
    } else if (reward == SecretReward::GamblerAllIn && kind == QStringLiteral("allIn")) {
        if (!claim_.valid || claim_.cards.isEmpty() || !challengeAllowed(playerIndex))
            return;
        consumeReward(playerIndex);
        addLog(QStringLiteral("%1 发动【赌徒·梭哈】，并质疑了 %2！")
                   .arg(players_[playerIndex].name, players_[claim_.declarer].name));
        showTableAction(QStringLiteral("%1 发动【赌徒·梭哈】！\n正在揭开 %2 刚才打出的牌……")
                            .arg(players_[playerIndex].name, players_[claim_.declarer].name), true);
        resolveChallenge(playerIndex, true);
    }
}

void QtWidgetsApplication1::startWaitProgress(const QString &stageText)
{
    stopAiProgress();
    if (!aiProgressBar_)
        return;
    aiProgressStageText_ = stageText;
    aiProgressBar_->setRange(0, 0); // 不确定进度
    aiProgressBar_->setValue(0);
    aiProgressBar_->setFormat(QStringLiteral("⏳ %1").arg(stageText));
    aiProgressBar_->show();
}
