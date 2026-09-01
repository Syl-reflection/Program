#include "QtWidgetsApplication1.h"

#include <QApplication>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QRandomGenerator>
#include <QScrollBar>
#include <QStringList>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <algorithm>
#include <functional>

#if defined(_MSC_VER)
#pragma execution_character_set("utf-8")
#endif

namespace {
constexpr int Human = 0;
constexpr int PlayerCount = 4;
constexpr int InitialHandSize = 5;
constexpr int JokerRank = 3;

QString panelStyle()
{
    return "QGroupBox { color:#f4d58d; border:1px solid #7b5e3b; border-radius:10px; "
           "margin-top:12px; padding:10px; font-weight:bold; } "
           "QGroupBox::title { subcontrol-origin:margin; left:12px; padding:0 6px; }";
}
}

QtWidgetsApplication1::QtWidgetsApplication1(QWidget *parent)
    : QMainWindow(parent)
{
    buildUi();
    QTimer::singleShot(50, this, [this] { startGame(); });
}

void QtWidgetsApplication1::buildUi()
{
    setWindowTitle(QStringLiteral("虎牌：秘密任务与动态事件排名赛"));
    resize(1260, 900);
    setMinimumSize(1050, 760);

    auto *central = new QWidget(this);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(22, 18, 22, 18);
    root->setSpacing(12);

    titleLabel_ = new QLabel(QStringLiteral("虎牌 · 秘密任务与动态事件排名赛"));
    titleLabel_->setAlignment(Qt::AlignCenter);
    titleLabel_->setStyleSheet("font-size:28px; font-weight:800; color:#f6c85f; letter-spacing:2px;");
    root->addWidget(titleLabel_);

    auto *opponents = new QHBoxLayout;
    for (int i = 0; i < 3; ++i) {
        opponentLabels_[i] = new QLabel;
        opponentLabels_[i]->setAlignment(Qt::AlignCenter);
        opponentLabels_[i]->setMinimumHeight(88);
        opponentLabels_[i]->setStyleSheet("background:#252a34; border:1px solid #555; border-radius:10px; padding:8px;");
        opponents->addWidget(opponentLabels_[i]);
    }
    root->addLayout(opponents);

    auto *middle = new QHBoxLayout;
    middle->setSpacing(12);

    auto *tableBox = new QGroupBox(QStringLiteral("牌桌"));
    tableBox->setStyleSheet(panelStyle());
    auto *tableLayout = new QVBoxLayout(tableBox);
    phaseLabel_ = new QLabel;
    phaseLabel_->setAlignment(Qt::AlignCenter);
    phaseLabel_->setWordWrap(true);
    phaseLabel_->setStyleSheet("font-size:19px; color:#ffffff; padding:8px;");
    eventLabel_ = new QLabel;
    eventLabel_->setAlignment(Qt::AlignCenter);
    eventLabel_->setWordWrap(true);
    eventLabel_->setStyleSheet("background:#3b2f1f; border:1px solid #d6a756; border-radius:8px; color:#ffe0a3; padding:7px; font-weight:bold;");
    claimLabel_ = new QLabel(QStringLiteral("尚无声明"));
    claimLabel_->setAlignment(Qt::AlignCenter);
    claimLabel_->setWordWrap(true);
    claimLabel_->setMinimumHeight(90);
    claimLabel_->setStyleSheet("background:#173f35; border:2px solid #c69c55; border-radius:12px; font-size:20px; padding:14px;");
    actionLabel_ = new QLabel(QStringLiteral("【牌桌即时行动】\n等待游戏开始……"));
    actionLabel_->setAlignment(Qt::AlignCenter);
    actionLabel_->setWordWrap(true);
    actionLabel_->setMinimumHeight(92);
    actionLabel_->setStyleSheet("background:#1d2d44; border:2px solid #5b8db8; border-radius:12px; color:#e8f4ff; font-size:18px; font-weight:bold; padding:12px;");
    tableLayout->addWidget(phaseLabel_);
    tableLayout->addWidget(eventLabel_);
    tableLayout->addWidget(claimLabel_, 1);
    tableLayout->addWidget(actionLabel_);

    auto *decisionRow = new QHBoxLayout;
    believeButton_ = new QPushButton(QStringLiteral("相信，轮到我出牌"));
    challengeButton_ = new QPushButton(QStringLiteral("质疑，立即揭牌"));
    decisionRow->addWidget(believeButton_);
    decisionRow->addWidget(challengeButton_);
    tableLayout->addLayout(decisionRow);
    middle->addWidget(tableBox, 3);

    auto *logBox = new QGroupBox(QStringLiteral("对局记录"));
    logBox->setStyleSheet(panelStyle());
    auto *logLayout = new QVBoxLayout(logBox);
    logEdit_ = new QTextEdit;
    logEdit_->setReadOnly(true);
    logEdit_->setStyleSheet("background:#17191f; border:none; color:#ddd; font-size:14px;");
    logLayout->addWidget(logEdit_);
    middle->addWidget(logBox, 2);
    root->addLayout(middle, 1);

    auto *humanBox = new QGroupBox(QStringLiteral("你的手牌（点击选择 1–3 张）"));
    humanBox->setStyleSheet(panelStyle());
    auto *humanLayout = new QVBoxLayout(humanBox);
    playerInfoLabel_ = new QLabel;
    rankingLabel_ = new QLabel;
    rankingLabel_->setWordWrap(true);
    rankingLabel_->setStyleSheet("color:#cbd5e1;");
    humanLayout->addWidget(playerInfoLabel_);
    humanLayout->addWidget(rankingLabel_);

    auto *secretRow = new QHBoxLayout;
    taskLabel_ = new QLabel;
    taskLabel_->setWordWrap(true);
    taskLabel_->setMinimumHeight(62);
    taskLabel_->setStyleSheet("background:#27213c; border:1px solid #7868a6; border-radius:8px; padding:8px; color:#e9ddff;");
    rewardLabel_ = new QLabel;
    rewardLabel_->setWordWrap(true);
    rewardLabel_->setMinimumHeight(62);
    rewardLabel_->setStyleSheet("background:#173f35; border:1px solid #4e8b78; border-radius:8px; padding:8px; color:#d9fff1;");
    rewardButton_ = new QPushButton(QStringLiteral("使用秘密奖励"));
    rewardButton_->setMinimumWidth(145);
    secretRow->addWidget(taskLabel_, 3);
    secretRow->addWidget(rewardLabel_, 3);
    secretRow->addWidget(rewardButton_);
    humanLayout->addLayout(secretRow);

    handLayout_ = new QHBoxLayout;
    handLayout_->setAlignment(Qt::AlignLeft);
    humanLayout->addLayout(handLayout_);

    auto *actionRow = new QHBoxLayout;
    actionRow->addWidget(new QLabel(QStringLiteral("声明牌面：")));
    rankCombo_ = new QComboBox;
    rankCombo_->addItems({QStringLiteral("A"), QStringLiteral("K"), QStringLiteral("Q")});
    actionRow->addWidget(rankCombo_);
    playButton_ = new QPushButton(QStringLiteral("盖牌并作出声明"));
    restartButton_ = new QPushButton(QStringLiteral("重新开始"));
    actionRow->addWidget(playButton_, 1);
    actionRow->addStretch();
    actionRow->addWidget(restartButton_);
    humanLayout->addLayout(actionRow);
    root->addWidget(humanBox);

    setCentralWidget(central);
    setStyleSheet(
        "QMainWindow, QWidget { background:#101319; color:#f2f2f2; font-family:'Microsoft YaHei UI'; }"
        "QPushButton { background:#7c2d2d; color:white; border:1px solid #b45353; border-radius:7px; padding:9px 14px; font-weight:bold; }"
        "QPushButton:hover { background:#9f3a3a; } QPushButton:disabled { background:#343840; color:#777; border-color:#444; }"
        "QComboBox { background:#252a34; border:1px solid #777; border-radius:6px; padding:7px 20px 7px 10px; min-width:80px; }"
    );

    connect(playButton_, &QPushButton::clicked, this, [this] { onPlayClicked(); });
    connect(believeButton_, &QPushButton::clicked, this, [this] { onBelieveClicked(); });
    connect(challengeButton_, &QPushButton::clicked, this, [this] { onChallengeClicked(); });
    connect(rewardButton_, &QPushButton::clicked, this, [this] { onRewardClicked(); });
    connect(restartButton_, &QPushButton::clicked, this, [this] { startGame(); });
}

void QtWidgetsApplication1::startGame()
{
    ++gameId_;
    players_.clear();
    players_.resize(PlayerCount);
    players_[0].name = QStringLiteral("你");
    players_[1].name = QStringLiteral("电脑·红狐");
    players_[2].name = QStringLiteral("电脑·灰狼");
    players_[3].name = QStringLiteral("电脑·夜鸦");
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

    finishOrder_.clear();
    tablePile_.clear();
    recentPlayCounts_.clear();
    recentRoundPileSizes_.clear();
    claim_ = Claim{};
    phase_ = Phase::Waiting;
    currentEvent_ = TavernEvent::None;
    currentPlayer_ = Human;
    roundNumber_ = 0;
    playsSinceLastRank_ = 0;
    allInChallenger_ = -1;
    pendingBartenderRush_ = false;
    pendingDrinkBeforeCatch_ = false;
    tableActionImportant_ = false;
    tableActionText_.clear();
    dealHands();
    for (int i = 0; i < players_.size(); ++i)
        assignRandomTask(i);
    selected_.fill(false, players_[Human].hand.size());

    logEdit_->clear();
    addLog(QStringLiteral("四人对局开始：所有玩家的开局条件与操作规则完全相同。"));
    addLog(QStringLiteral("牌库包含 A、K、Q 各 6 张及 2 张 Joker；Joker 可充当声明牌型。"));
    addLog(QStringLiteral("每人获得 5 张牌。率先出完手牌者获得第 1 名，其余玩家继续争夺后续名次。"));
    addLog(QStringLiteral("选择相信后，盖牌会留在中央牌池；质疑判断失败者收走整个中央牌池。"));
    addLog(QStringLiteral("系统会在牌局节奏变慢时自动触发公开的酒馆事件。"));
    addLog(QStringLiteral("【你的秘密任务】%1：%2").arg(taskName(players_[Human].task), taskDescription(players_[Human].task)));
    showTableAction(QStringLiteral("四人对局已经开始，等待第一位玩家行动。"));
    startNewRound(Human);
}

void QtWidgetsApplication1::dealHands()
{
    QVector<int> deck;
    for (int rank = 0; rank < 3; ++rank)
        for (int i = 0; i < 6; ++i)
            deck.append(rank);
    deck.append(JokerRank);
    deck.append(JokerRank);
    std::shuffle(deck.begin(), deck.end(), *QRandomGenerator::global());

    int position = 0;
    for (Player &player : players_)
        for (int i = 0; i < InitialHandSize; ++i)
            player.hand.append(deck[position++]);
}

void QtWidgetsApplication1::startNewRound(int starter)
{
    if (phase_ == Phase::GameOver)
        return;

    ++roundNumber_;
    expireRewardsForNewRound();
    claim_ = Claim{};
    tablePile_.clear();
    chooseEventForNewRound();
    addLog(QStringLiteral("—— 第 %1 轮开始 ——").arg(roundNumber_));
    const int actualStarter = players_[starter].finished ? nextActive(starter) : starter;
    showTableAction(QStringLiteral("第 %1 轮开始，由 %2 先行动。")
                        .arg(roundNumber_).arg(players_[actualStarter].name));
    beginTurn(actualStarter);
}

void QtWidgetsApplication1::beginTurn(int playerIndex)
{
    if (phase_ == Phase::GameOver)
        return;
    if (players_[playerIndex].finished)
        playerIndex = nextActive(playerIndex);

    currentPlayer_ = playerIndex;
    selected_.fill(false, players_[Human].hand.size());
    if (playerIndex == Human) {
        beginHumanTurn();
    } else {
        const QString phaseText = players_[Human].finished
            ? QStringLiteral("【观战模式】你已获得第 %1 名 · %2 正在思考……")
                  .arg(players_[Human].rank).arg(players_[playerIndex].name)
            : QStringLiteral("%1 正在思考……").arg(players_[playerIndex].name);
        setPhase(Phase::Waiting, phaseText);
        updateUi();
        const int expectedGame = gameId_;
        const int thinkDelay = players_[Human].finished ? 1350 : 700;
        QTimer::singleShot(thinkDelay, this, [this, expectedGame] {
            if (expectedGame == gameId_)
                runAiTurn();
        });
    }
}

void QtWidgetsApplication1::beginHumanTurn()
{
    if (claim_.valid) {
        if (players_[Human].gamblingBan)
            setPhase(Phase::Decide, QStringLiteral("【禁赌状态】你在完成下一次真实出牌前不能质疑，本次只能相信。"));
        else if (challengeAllowed(Human))
            setPhase(Phase::Decide, QStringLiteral("轮到你判断：相信 %1，还是质疑并揭牌？").arg(players_[claim_.declarer].name));
        else
            setPhase(Phase::Decide, QStringLiteral("【先喝再抓】中央牌池不足 5 张，本次只能相信。"));
    } else {
        const int minimum = minimumPlayCount(Human);
        const int maximum = maximumPlayCount(Human);
        const QString restriction = players_[Human].gamblingBan
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
    if (currentPlayer_ == Human || phase_ == Phase::GameOver || players_[currentPlayer_].finished)
        return;

    const int ai = currentPlayer_;
    const bool decidedOnClaim = claim_.valid;
    if (claim_.valid) {
        int suspicion = 22 + static_cast<int>(claim_.cards.size()) * 12;
        int knownDeclared = 0;
        for (int card : players_[ai].hand)
            if (card == claim_.declaredRank || card == JokerRank)
                ++knownDeclared;
        suspicion += knownDeclared * 7;
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
            if (players_[Human].finished) {
                setPhase(Phase::Waiting, QStringLiteral("【观战模式】%1 已选择质疑，马上揭牌……")
                    .arg(players_[ai].name));
                updateUi();
                const int expectedGame = gameId_;
                QTimer::singleShot(1200, this, [this, ai, useAllIn, expectedGame] {
                    if (expectedGame == gameId_ && currentPlayer_ == ai && claim_.valid)
                        resolveChallenge(ai, useAllIn);
                });
            } else {
                resolveChallenge(ai, useAllIn);
            }
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
                  .arg(players_[ai].name, players_[claim_.declarer].name));
        checkBeliefTask(claim_.declarer);
        if (players_[claim_.declarer].hand.isEmpty()) {
            confirmFinishedPlayer(claim_.declarer);
            if (finishGameIfReady())
                return;
        }
        claim_ = Claim{};
    }

    const int expectedGame = gameId_;
    const int actionDelay = players_[Human].finished
        ? (decidedOnClaim ? 1150 : 450)
        : 400;
    QTimer::singleShot(actionDelay, this, [this, expectedGame] {
        if (expectedGame == gameId_)
            aiPlay();
    });
}

void QtWidgetsApplication1::aiPlay()
{
    const int ai = currentPlayer_;
    if (ai == Human || phase_ == Phase::GameOver || players_[ai].finished || players_[ai].hand.isEmpty())
        return;

    Player &player = players_[ai];
    const bool mustTellTruth = player.gamblingBan;
    int declared = QRandomGenerator::global()->bounded(3);

    auto matchingIndices = [&player](int rank) {
        QVector<int> result;
        for (int i = 0; i < player.hand.size(); ++i)
            if (player.hand[i] == rank || player.hand[i] == JokerRank)
                result.append(i);
        return result;
    };

    QVector<int> matching = matchingIndices(declared);
    if (mustTellTruth) {
        for (int rank = 0; rank < 3; ++rank) {
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
        for (int i = 0; i < player.hand.size() && indices.size() < count; ++i) {
            if (player.hand[i] != declared && player.hand[i] != JokerRank)
                indices.append(i);
        }
        for (int i = 0; i < player.hand.size() && indices.size() < count; ++i) {
            if (!indices.contains(i))
                indices.append(i);
        }
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
    addLog(QStringLiteral("%1 盖下 %2 张牌，声明：%2 张 %3。")
               .arg(player.name).arg(cards.size()).arg(cardName(declared)));
    showTableAction(QStringLiteral("%1 完成出牌：\n盖下 %2 张牌，并声明“%2 张 %3”。")
                        .arg(player.name).arg(cards.size()).arg(cardName(declared)));
    if (mustTellTruth) {
        player.gamblingBan = false;
        addLog(QStringLiteral("%1 已完成下一次真实出牌，禁赌状态解除。").arg(player.name));
    }
    beginTurn(nextActive(ai));
}

void QtWidgetsApplication1::onPlayClicked()
{
    if (phase_ != Phase::Play || currentPlayer_ != Human || players_[Human].finished)
        return;

    QVector<int> indices;
    for (int i = 0; i < selected_.size(); ++i)
        if (selected_[i])
            indices.append(i);
    const int minimum = minimumPlayCount(Human);
    const int maximum = maximumPlayCount(Human);
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
        previewCards.append(players_[Human].hand[index]);
    if (players_[Human].gamblingBan && !cardsMatchClaim(previewCards, declaredRank)) {
        QMessageBox::information(this, QStringLiteral("禁赌状态"),
            QStringLiteral("梭哈失败后的下一次出牌必须完全真实。\n"
                           "所选牌必须都是声明牌型或 Joker。"));
        return;
    }

    claim_ = Claim{};
    claim_.valid = true;
    claim_.declarer = Human;
    claim_.declaredRank = declaredRank;
    claim_.pileSizeBeforePlay = tablePile_.size();
    claim_.declarerHandSizeBeforePlay = players_[Human].hand.size();
    std::sort(indices.begin(), indices.end(), std::greater<int>());
    for (int index : indices) {
        claim_.cards.prepend(players_[Human].hand[index]);
        players_[Human].hand.removeAt(index);
    }
    for (int card : claim_.cards)
        tablePile_.append(card);
    recordValidPlay(claim_.cards.size());

    addLog(QStringLiteral("你盖下 %1 张牌，声明：%1 张 %2。")
               .arg(claim_.cards.size()).arg(cardName(claim_.declaredRank)));
    showTableAction(QStringLiteral("你完成出牌：\n盖下 %1 张牌，并声明“%1 张 %2”。")
                        .arg(claim_.cards.size()).arg(cardName(claim_.declaredRank)));
    if (players_[Human].gamblingBan) {
        players_[Human].gamblingBan = false;
        addLog(QStringLiteral("你已完成下一次真实出牌，禁赌状态解除。"));
    }
    beginTurn(nextActive(Human));
}

void QtWidgetsApplication1::onBelieveClicked()
{
    if (phase_ != Phase::Decide || currentPlayer_ != Human || !claim_.valid)
        return;

    addLog(QStringLiteral("你选择相信 %1，中央牌池继续累积。").arg(players_[claim_.declarer].name));
    showTableAction(QStringLiteral("你选择【相信】%1 的声明。\n中央牌池继续保留，现在轮到你出牌。")
                        .arg(players_[claim_.declarer].name));
    checkBeliefTask(claim_.declarer);
    if (players_[claim_.declarer].hand.isEmpty()) {
        confirmFinishedPlayer(claim_.declarer);
        if (finishGameIfReady())
            return;
    }
    claim_ = Claim{};
    beginHumanTurn();
}

void QtWidgetsApplication1::onChallengeClicked()
{
    if (phase_ == Phase::Decide && currentPlayer_ == Human && claim_.valid && challengeAllowed(Human)) {
        addLog(QStringLiteral("你质疑了 %1！").arg(players_[claim_.declarer].name));
        showTableAction(QStringLiteral("你选择【质疑】！\n正在揭开 %1 刚才打出的牌……")
                            .arg(players_[claim_.declarer].name), true);
        resolveChallenge(Human);
    }
}

void QtWidgetsApplication1::onRewardClicked()
{
    if (!rewardUsable(Human))
        return;

    const SecretReward reward = players_[Human].reward;
    if (reward == SecretReward::PeekCard) {
        const int index = QRandomGenerator::global()->bounded(static_cast<int>(claim_.cards.size()));
        const QString seenCard = cardName(claim_.cards[index]);
        consumeReward(Human);
        QMessageBox::information(this, QStringLiteral("窥牌结果"),
            QStringLiteral("你随机看到上一家刚打出的 1 张牌：%1\n\n其余牌仍然隐藏，请继续选择相信或质疑。")
                .arg(seenCard));
        return;
    }

    if (reward == SecretReward::RankScout) {
        QStringList targets;
        QVector<int> targetIndices;
        for (int i = 0; i < players_.size(); ++i) {
            if (i != Human && !players_[i].finished) {
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
        const QStringList ranks = { QStringLiteral("A"), QStringLiteral("K"), QStringLiteral("Q"), QStringLiteral("Joker") };
        const QString rankText = QInputDialog::getItem(this, QStringLiteral("点数侦查"),
            QStringLiteral("选择要侦查的牌型："), ranks, 0, false, &accepted);
        if (!accepted)
            return;
        const int target = targetIndices[targets.indexOf(targetText)];
        const int rank = ranks.indexOf(rankText);
        int count = 0;
        for (int card : players_[target].hand)
            if (card == rank)
                ++count;
        consumeReward(Human);
        QMessageBox::information(this, QStringLiteral("点数侦查结果"),
            QStringLiteral("%1 当前手中有 %2 张 %3。\n\n这条情报只代表当前时刻。")
                .arg(players_[target].name).arg(count).arg(cardName(rank)));
        return;
    }

    if (reward == SecretReward::PileScout) {
        bool accepted = false;
        const QStringList ranks = { QStringLiteral("A"), QStringLiteral("K"), QStringLiteral("Q"), QStringLiteral("Joker") };
        const QString rankText = QInputDialog::getItem(this, QStringLiteral("牌堆侦察"),
            QStringLiteral("选择要侦察的牌型："), ranks, 0, false, &accepted);
        if (!accepted)
            return;
        const int rank = ranks.indexOf(rankText);
        int count = 0;
        for (int card : tablePile_)
            if (card == rank)
                ++count;
        consumeReward(Human);
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
        consumeReward(Human);
        addLog(QStringLiteral("你发动【赌徒·梭哈】，并质疑了 %1！").arg(players_[claim_.declarer].name));
        showTableAction(QStringLiteral("你发动【赌徒·梭哈】！\n正在揭开 %1 刚才打出的牌……")
                            .arg(players_[claim_.declarer].name), true);
        resolveChallenge(Human, true);
    }
}

void QtWidgetsApplication1::resolveChallenge(int challenger, bool allIn)
{
    allInChallenger_ = allIn ? challenger : -1;
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
    const int settlementDelay = players_[Human].finished ? 1800 : 750;
    QTimer::singleShot(settlementDelay, this, [this, loser, expectedGame] {
        if (expectedGame == gameId_)
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
}

bool QtWidgetsApplication1::finishGameIfReady()
{
    if (finishOrder_.size() < players_.size() - 1)
        return false;

    if (finishOrder_.size() == players_.size() - 1) {
        for (int i = 0; i < players_.size(); ++i) {
            if (!players_[i].finished) {
                players_[i].finished = true;
                players_[i].rank = players_.size();
                finishOrder_.append(i);
                addLog(QStringLiteral("%1 为最后一位未出完手牌的玩家，获得第 %2 名。")
                           .arg(players_[i].name).arg(players_[i].rank));
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
    QMessageBox::information(this, QStringLiteral("游戏结束"), QStringLiteral("最终排名\n%1").arg(result));
    return true;
}

void QtWidgetsApplication1::recordValidPlay(int cardCount)
{
    recentPlayCounts_.append(cardCount);
    while (recentPlayCounts_.size() > 6)
        recentPlayCounts_.removeAt(0);

    ++playsSinceLastRank_;
    if (activePlayerCount() >= 3 && playsSinceLastRank_ >= 20
        && currentEvent_ != TavernEvent::FinalTable
        && currentEvent_ != TavernEvent::ClosingTime) {
        activateEvent(TavernEvent::ClosingTime);
    }
}

void QtWidgetsApplication1::detectNextRoundEvents(int settledPileSize)
{
    recentRoundPileSizes_.append(settledPileSize);
    while (recentRoundPileSizes_.size() > 3)
        recentRoundPileSizes_.removeAt(0);

    pendingBartenderRush_ = false;
    pendingDrinkBeforeCatch_ = false;
    if (activePlayerCount() < 3 || currentEvent_ == TavernEvent::FinalTable
        || currentEvent_ == TavernEvent::ClosingTime) {
        return;
    }

    if (recentPlayCounts_.size() == 6) {
        int conservativePlays = 0;
        for (int count : recentPlayCounts_)
            if (count <= 2)
                ++conservativePlays;
        pendingBartenderRush_ = conservativePlays >= 5;
    }

    if (recentRoundPileSizes_.size() == 3) {
        pendingDrinkBeforeCatch_ = true;
        for (int pileSize : recentRoundPileSizes_)
            if (pileSize > 4)
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
    if (playsSinceLastRank_ >= 20)
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
        minimum = std::min(3, handSize);
    else if (currentEvent_ == TavernEvent::FinalTable || currentEvent_ == TavernEvent::ClosingTime)
        minimum = std::min(2, handSize);

    if (players_[playerIndex].gamblingBan)
        minimum = std::min(minimum, truthfulPlayCapacity(playerIndex));
    return minimum;
}

int QtWidgetsApplication1::maximumPlayCount(int playerIndex) const
{
    int maximum = std::min(3, static_cast<int>(players_[playerIndex].hand.size()));
    if (players_[playerIndex].gamblingBan)
        maximum = std::min(maximum, truthfulPlayCapacity(playerIndex));
    return maximum;
}

int QtWidgetsApplication1::truthfulPlayCapacity(int playerIndex) const
{
    int jokers = 0;
    int counts[3] = { 0, 0, 0 };
    for (int card : players_[playerIndex].hand) {
        if (card == JokerRank)
            ++jokers;
        else if (card >= 0 && card < 3)
            ++counts[card];
    }
    int best = 0;
    for (int count : counts)
        best = std::max(best, count + jokers);
    return std::min(3, best);
}

bool QtWidgetsApplication1::challengeAllowed(int challenger) const
{
    if (!claim_.valid)
        return false;
    if (challenger < 0 || challenger >= players_.size() || players_[challenger].gamblingBan)
        return false;
    if (currentEvent_ != TavernEvent::DrinkBeforeCatch)
        return true;
    return tablePile_.size() >= 5 || players_[claim_.declarer].hand.isEmpty();
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
    if (playerIndex == Human)
        addLog(QStringLiteral("【秘密任务完成】%1（本局已完成 %2/2）。")
                   .arg(taskName(completedTask)).arg(player.completedTasks));

    if (player.reward == SecretReward::None) {
        grantRandomReward(playerIndex);
    } else {
        ++player.pendingRewards;
        if (playerIndex == Human)
            addLog(QStringLiteral("你当前已有奖励，新奖励已进入待领取状态；旧奖励使用或失效后自动发放。"));
    }

    if (player.completedTasks < 2) {
        assignRandomTask(playerIndex);
        if (playerIndex == Human)
            addLog(QStringLiteral("【新的秘密任务】%1：%2")
                       .arg(taskName(player.task), taskDescription(player.task)));
    } else if (playerIndex == Human) {
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
        && claim_.pileSizeBeforePlay >= 6
        && claim_.cards.size() >= 2 && !truthful) {
        completeSecretTask(declarer);
    } else if (task == SecretTask::DesperateBluff
        && claim_.declarerHandSizeBeforePlay >= 4
        && claim_.declarerHandSizeBeforePlay <= 5
        && claim_.cards.size() == 3 && !truthful) {
        completeSecretTask(declarer);
    }
}

void QtWidgetsApplication1::checkChallengeTasks(int challenger, bool truthful, int settledPileSize)
{
    const int declarer = claim_.declarer;
    if (truthful && players_[declarer].task == SecretTask::BaitChallenge
        && claim_.pileSizeBeforePlay >= 6
        && claim_.cards.size() >= 2) {
        completeSecretTask(declarer);
    }
    if (!truthful && players_[challenger].task == SecretTask::BoldChallenge
        && settledPileSize >= 9) {
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
    if (playerIndex == Human) {
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
            if (i == Human)
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
    if (!claim_.valid || player.reward == SecretReward::None)
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
    if (playerIndex == Human && currentPlayer_ != Human)
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
        if (challenger == Human) {
            QMessageBox::information(this, QStringLiteral("梭哈失败"),
                QStringLiteral("你已进入禁赌状态：\n"
                               "1. 完成下一次出牌前不能质疑；\n"
                               "2. 下一次出牌必须作出完全真实的声明。"));
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
    if (challenger == Human) {
        bool accepted = false;
        const QString chosen = QInputDialog::getItem(this, QStringLiteral("梭哈成功"),
            QStringLiteral("选择一名玩家，秘密查看其当前全部手牌："), names, 0, false, &accepted);
        if (accepted)
            target = candidates[names.indexOf(chosen)];
    }

    QVector<int> hand = players_[target].hand;
    std::sort(hand.begin(), hand.end());
    QStringList cards;
    for (int card : hand)
        cards << cardName(card);
    addLog(QStringLiteral("%1 梭哈成功，秘密查看了 %2 当前的全部手牌。")
               .arg(player.name, players_[target].name));
    if (challenger == Human) {
        QMessageBox::information(this, QStringLiteral("梭哈情报"),
            QStringLiteral("%1 当前的全部手牌：\n[ %2 ]\n\n这条情报只代表当前时刻。")
                .arg(players_[target].name, cards.join(QStringLiteral("、"))));
    }
}

void QtWidgetsApplication1::showTableAction(const QString &text, bool important)
{
    tableActionText_ = text;
    tableActionImportant_ = important;
    if (!actionLabel_)
        return;

    const bool spectatorMode = !players_.isEmpty() && players_[Human].finished
        && phase_ != Phase::GameOver;
    const QString heading = spectatorMode
        ? QStringLiteral("【观战模式 · 你已获得第 %1 名】\n").arg(players_[Human].rank)
        : QStringLiteral("【牌桌即时行动】\n");
    actionLabel_->setText(heading + tableActionText_);
    actionLabel_->setStyleSheet(tableActionImportant_
        ? "background:#4a2027; border:3px solid #f59e6b; border-radius:12px; color:#fff1df; font-size:19px; font-weight:bold; padding:12px;"
        : "background:#1d2d44; border:2px solid #5b8db8; border-radius:12px; color:#e8f4ff; font-size:18px; font-weight:bold; padding:12px;");
}

void QtWidgetsApplication1::updateUi()
{
    if (players_.isEmpty())
        return;

    for (int i = 1; i < players_.size(); ++i) {
        const Player &player = players_[i];
        const bool activeNow = currentPlayer_ == i && !player.finished && phase_ != Phase::GameOver;
        opponentLabels_[i - 1]->setText(QStringLiteral("%1\n手牌：%2 张　状态：%3%4")
            .arg(player.name).arg(player.hand.size()).arg(playerStatusText(player))
            .arg(activeNow ? QStringLiteral("\n▶ 当前正在行动") : QString()));
        if (player.finished) {
            opponentLabels_[i - 1]->setStyleSheet(
                "background:#20242b; border:1px solid #4b5563; border-radius:10px; padding:8px; color:#9ca3af;");
        } else if (activeNow) {
            opponentLabels_[i - 1]->setStyleSheet(
                "background:#4a321f; border:3px solid #f6c85f; border-radius:10px; padding:8px; color:#fff4cf; font-weight:bold;");
        } else {
            opponentLabels_[i - 1]->setStyleSheet(
                "background:#252a34; border:1px solid #555; border-radius:10px; padding:8px;");
        }
    }

    const Player &human = players_[Human];
    playerInfoLabel_->setText(QStringLiteral("你的手牌：%1 张　　状态：%2　　第 %3 轮")
                                  .arg(human.hand.size()).arg(playerStatusText(human)).arg(roundNumber_));
    rankingLabel_->setText(finishOrder_.isEmpty()
        ? QStringLiteral("当前排名：尚未有人出完手牌")
        : QStringLiteral("当前排名：%1").arg(rankingSummary()));

    if (human.task == SecretTask::None) {
        taskLabel_->setText(QStringLiteral("【你的秘密任务】本局已完成 2/2，不再获得新任务。"));
    } else {
        taskLabel_->setText(QStringLiteral("【你的秘密任务：%1】（已完成 %2/2）\n%3")
                                .arg(taskName(human.task)).arg(human.completedTasks)
                                .arg(taskDescription(human.task)));
    }
    if (human.reward == SecretReward::None) {
        rewardLabel_->setText(QStringLiteral("【你的秘密奖励】暂无奖励"));
        rewardButton_->setText(QStringLiteral("使用秘密奖励"));
    } else {
        QString pendingText;
        if (human.pendingRewards > 0)
            pendingText = QStringLiteral("　另有 %1 份待领取").arg(human.pendingRewards);
        rewardLabel_->setText(QStringLiteral("【你的秘密奖励：%1】有效至第 %2 轮结束%3\n%4")
                                  .arg(rewardName(human.reward))
                                  .arg(human.rewardAwardRound + 1)
                                  .arg(pendingText)
                                  .arg(rewardDescription(human.reward)));
        rewardButton_->setText(QStringLiteral("使用：%1").arg(rewardName(human.reward)));
    }

    eventLabel_->setText(currentEvent_ == TavernEvent::None
        ? QStringLiteral("当前无酒馆事件 · 基础规则：每次出 1–3 张牌")
        : QStringLiteral("【酒馆事件：%1】%2").arg(eventName(currentEvent_), eventDescription(currentEvent_)));

    if (claim_.valid) {
        claimLabel_->setText(QStringLiteral("中央牌池：%1 张\n%2 的声明：“这里有 %3 张 %4”")
                                 .arg(tablePile_.size())
                                 .arg(players_[claim_.declarer].name)
                                 .arg(claim_.cards.size())
                                 .arg(cardName(claim_.declaredRank)));
    } else {
        claimLabel_->setText(QStringLiteral("中央牌池：%1 张\n桌面目前没有待判断的声明").arg(tablePile_.size()));
    }

    const bool humanTurn = currentPlayer_ == Human && !human.finished;
    believeButton_->setEnabled(humanTurn && phase_ == Phase::Decide);
    challengeButton_->setEnabled(humanTurn && phase_ == Phase::Decide && challengeAllowed(Human));
    rewardButton_->setEnabled(rewardUsable(Human));
    playButton_->setEnabled(humanTurn && phase_ == Phase::Play && !human.hand.isEmpty());
    rankCombo_->setEnabled(humanTurn && phase_ == Phase::Play && !human.hand.isEmpty());
    rebuildHandButtons();
}

void QtWidgetsApplication1::rebuildHandButtons()
{
    while (QLayoutItem *item = handLayout_->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    if (players_.isEmpty())
        return;
    if (selected_.size() != players_[Human].hand.size())
        selected_.fill(false, players_[Human].hand.size());

    for (int i = 0; i < players_[Human].hand.size(); ++i) {
        auto *button = new QPushButton(cardName(players_[Human].hand[i]));
        button->setCheckable(true);
        button->setChecked(selected_[i]);
        button->setEnabled(currentPlayer_ == Human && phase_ == Phase::Play && !players_[Human].finished);
        button->setMinimumSize(70, 76);
        button->setStyleSheet("QPushButton{font-size:24px;background:#e8dcc4;color:#201a16;border:3px solid #8b7355;border-radius:9px;}"
                              "QPushButton:checked{background:#f6c85f;border-color:#fff;}"
                              "QPushButton:disabled{background:#8f8778;color:#4b463f;}");
        connect(button, &QPushButton::toggled, this, [this, i](bool checked) {
            if (checked) {
                int count = 0;
                for (bool value : selected_)
                    if (value)
                        ++count;
                if (count >= 3) {
                    if (auto *clickedButton = qobject_cast<QPushButton *>(sender()))
                        clickedButton->setChecked(false);
                    return;
                }
            }
            selected_[i] = checked;
        });
        handLayout_->addWidget(button);
    }
    handLayout_->addStretch();
}

void QtWidgetsApplication1::setPhase(Phase phase, const QString &text)
{
    phase_ = phase;
    phaseLabel_->setText(text);
}

void QtWidgetsApplication1::addLog(const QString &text)
{
    logEdit_->append(QStringLiteral("• ") + text);
    logEdit_->verticalScrollBar()->setValue(logEdit_->verticalScrollBar()->maximum());
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
    if (cards.isEmpty() || declaredRank < 0 || declaredRank >= 3)
        return false;
    for (int card : cards)
        if (card != declaredRank && card != JokerRank)
            return false;
    return true;
}

QString QtWidgetsApplication1::cardName(int rank) const
{
    static const QString names[] = {
        QStringLiteral("A"), QStringLiteral("K"), QStringLiteral("Q"), QStringLiteral("Joker")
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
        return QStringLiteral("本轮每次必须打出 3 张；不足 3 张时打出全部剩余手牌。");
    case TavernEvent::DrinkBeforeCatch:
        return QStringLiteral("中央牌池达到 5 张前禁止质疑；上一名玩家打出最后一手时例外。");
    case TavernEvent::FinalTable:
        return QStringLiteral("剩余两人每次至少打出 2 张；仅剩 1 张时可正常打出。");
    case TavernEvent::ClosingTime:
        return QStringLiteral("直到产生下一名次前，每次至少打出 2 张；仅剩 1 张时可正常打出。");
    case TavernEvent::None:
        return QStringLiteral("按基础规则每次出 1–3 张牌。");
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
        return QStringLiteral("出牌前中央牌堆至少 6 张，一次打出至少 2 张并虚假声明，让下一家相信。");
    case SecretTask::BaitChallenge:
        return QStringLiteral("出牌前中央牌堆至少 6 张，一次打出至少 2 张并完全真实声明，让下一家质疑失败。");
    case SecretTask::BoldChallenge:
        return QStringLiteral("中央牌堆达到至少 9 张时主动质疑，并成功识破上一家的谎言。");
    case SecretTask::DesperateBluff:
        return QStringLiteral("出牌前手中剩余 4–5 张，一次打出 3 张并虚假声明，让下一家相信。");
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
