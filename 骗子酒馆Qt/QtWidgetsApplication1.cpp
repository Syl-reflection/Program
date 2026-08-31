#include "QtWidgetsApplication1.h"
//1231321
#include <QApplication>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
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
    setWindowTitle(QStringLiteral("虎牌：动态事件排名赛"));
    resize(1220, 780);
    setMinimumSize(1020, 700);

    auto *central = new QWidget(this);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(22, 18, 22, 18);
    root->setSpacing(12);

    titleLabel_ = new QLabel(QStringLiteral("虎牌 · 动态事件排名赛"));
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
    tableLayout->addWidget(phaseLabel_);
    tableLayout->addWidget(eventLabel_);
    tableLayout->addWidget(claimLabel_, 1);

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
    pendingBartenderRush_ = false;
    pendingDrinkBeforeCatch_ = false;
    dealHands();
    selected_.fill(false, players_[Human].hand.size());

    logEdit_->clear();
    addLog(QStringLiteral("四人对局开始：所有玩家的开局条件与操作规则完全相同。"));
    addLog(QStringLiteral("每人获得 5 张牌。率先出完手牌者获得第 1 名，其余玩家继续争夺后续名次。"));
    addLog(QStringLiteral("选择相信后，盖牌会留在中央牌池；质疑判断失败者收走整个中央牌池。"));
    addLog(QStringLiteral("系统会在牌局节奏变慢时自动触发公开的酒馆事件。"));
    startNewRound(Human);
}

void QtWidgetsApplication1::dealHands()
{
    QVector<int> deck;
    for (int rank = 0; rank < 3; ++rank)
        for (int i = 0; i < 8; ++i)
            deck.append(rank);
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
    claim_ = Claim{};
    tablePile_.clear();
    chooseEventForNewRound();
    addLog(QStringLiteral("—— 第 %1 轮开始 ——").arg(roundNumber_));
    beginTurn(players_[starter].finished ? nextActive(starter) : starter);
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
        setPhase(Phase::Waiting, QStringLiteral("%1 正在思考……").arg(players_[playerIndex].name));
        updateUi();
        const int expectedGame = gameId_;
        QTimer::singleShot(700, this, [this, expectedGame] {
            if (expectedGame == gameId_)
                runAiTurn();
        });
    }
}

void QtWidgetsApplication1::beginHumanTurn()
{
    if (claim_.valid) {
        if (challengeAllowed())
            setPhase(Phase::Decide, QStringLiteral("轮到你判断：相信 %1，还是质疑并揭牌？").arg(players_[claim_.declarer].name));
        else
            setPhase(Phase::Decide, QStringLiteral("【先喝再抓】中央牌池不足 5 张，本次只能相信。"));
    } else {
        const int minimum = minimumPlayCount(Human);
        const int maximum = maximumPlayCount(Human);
        setPhase(Phase::Play, minimum == maximum
            ? QStringLiteral("轮到你出牌：本次必须打出 %1 张牌。").arg(minimum)
            : QStringLiteral("轮到你出牌：请选择 %1–%2 张牌。").arg(minimum).arg(maximum));
    }
    updateUi();
}

void QtWidgetsApplication1::runAiTurn()
{
    if (currentPlayer_ == Human || phase_ == Phase::GameOver || players_[currentPlayer_].finished)
        return;

    const int ai = currentPlayer_;
    if (claim_.valid) {
        int suspicion = 22 + static_cast<int>(claim_.cards.size()) * 12;
        int knownDeclared = 0;
        for (int card : players_[ai].hand)
            if (card == claim_.declaredRank)
                ++knownDeclared;
        suspicion += knownDeclared * 7;
        if (players_[claim_.declarer].hand.isEmpty())
            suspicion += 10;
        suspicion = std::clamp(suspicion, 5, 92);

        if (challengeAllowed() && QRandomGenerator::global()->bounded(100) < suspicion) {
            addLog(QStringLiteral("%1 质疑了 %2！").arg(players_[ai].name, players_[claim_.declarer].name));
            resolveChallenge(ai);
            return;
        }

        addLog(challengeAllowed()
            ? QStringLiteral("%1 选择相信上一份声明，中央牌池继续累积。").arg(players_[ai].name)
            : QStringLiteral("%1 受【先喝再抓】限制，只能相信，中央牌池继续累积。").arg(players_[ai].name));
        if (players_[claim_.declarer].hand.isEmpty()) {
            confirmFinishedPlayer(claim_.declarer);
            if (finishGameIfReady())
                return;
        }
        claim_ = Claim{};
    }

    const int expectedGame = gameId_;
    QTimer::singleShot(400, this, [this, expectedGame] {
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
    const int declared = QRandomGenerator::global()->bounded(3);
    QVector<int> matching;
    for (int i = 0; i < player.hand.size(); ++i)
        if (player.hand[i] == declared)
            matching.append(i);

    const int minimumCards = minimumPlayCount(ai);
    const int maximumCards = maximumPlayCount(ai);
    const bool canTellTruth = matching.size() >= minimumCards;
    const bool intendsTruth = canTellTruth && QRandomGenerator::global()->bounded(100) < 62;
    int count = minimumCards;
    if (maximumCards > minimumCards)
        count += QRandomGenerator::global()->bounded(maximumCards - minimumCards + 1);
    QVector<int> indices;
    if (intendsTruth) {
        count = std::min(count, static_cast<int>(matching.size()));
        indices = matching.mid(0, count);
    } else {
        for (int i = 0; i < count; ++i)
            indices.append(i);
    }

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
    for (int card : cards)
        tablePile_.append(card);
    recordValidPlay(cards.size());
    addLog(QStringLiteral("%1 盖下 %2 张牌，声明：%2 张 %3。")
               .arg(player.name).arg(cards.size()).arg(cardName(declared)));
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

    claim_ = Claim{};
    claim_.valid = true;
    claim_.declarer = Human;
    claim_.declaredRank = rankCombo_->currentIndex();
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
    beginTurn(nextActive(Human));
}

void QtWidgetsApplication1::onBelieveClicked()
{
    if (phase_ != Phase::Decide || currentPlayer_ != Human || !claim_.valid)
        return;

    addLog(QStringLiteral("你选择相信 %1，中央牌池继续累积。").arg(players_[claim_.declarer].name));
    if (players_[claim_.declarer].hand.isEmpty()) {
        confirmFinishedPlayer(claim_.declarer);
        if (finishGameIfReady())
            return;
    }
    claim_ = Claim{};
    const int minimum = minimumPlayCount(Human);
    const int maximum = maximumPlayCount(Human);
    setPhase(Phase::Play, minimum == maximum
        ? QStringLiteral("现在轮到你出牌：必须打出 %1 张牌。").arg(minimum)
        : QStringLiteral("现在轮到你出牌：请选择 %1–%2 张牌。").arg(minimum).arg(maximum));
    updateUi();
}

void QtWidgetsApplication1::onChallengeClicked()
{
    if (phase_ == Phase::Decide && currentPlayer_ == Human && claim_.valid && challengeAllowed()) {
        addLog(QStringLiteral("你质疑了 %1！").arg(players_[claim_.declarer].name));
        resolveChallenge(Human);
    }
}

void QtWidgetsApplication1::resolveChallenge(int challenger)
{
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

    if (currentEvent_ == TavernEvent::BartenderRush || currentEvent_ == TavernEvent::DrinkBeforeCatch) {
        addLog(QStringLiteral("【酒馆事件结束】%1结束，下轮恢复基础规则。").arg(eventName(currentEvent_)));
        currentEvent_ = TavernEvent::None;
    }
    if (truthful && players_[declarer].hand.isEmpty())
        confirmFinishedPlayer(declarer);
    detectNextRoundEvents(settledPileSize);
    claim_ = Claim{};
    tablePile_.clear();
    updateUi();
    if (finishGameIfReady())
        return;

    const int expectedGame = gameId_;
    QTimer::singleShot(750, this, [this, loser, expectedGame] {
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
    updateEventAfterRank();
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
    if (currentEvent_ == TavernEvent::BartenderRush)
        return std::min(3, handSize);
    if (currentEvent_ == TavernEvent::FinalTable || currentEvent_ == TavernEvent::ClosingTime)
        return std::min(2, handSize);
    return 1;
}

int QtWidgetsApplication1::maximumPlayCount(int playerIndex) const
{
    return std::min(3, static_cast<int>(players_[playerIndex].hand.size()));
}

bool QtWidgetsApplication1::challengeAllowed() const
{
    if (!claim_.valid)
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

void QtWidgetsApplication1::updateUi()
{
    if (players_.isEmpty())
        return;

    for (int i = 1; i < players_.size(); ++i) {
        const Player &player = players_[i];
        opponentLabels_[i - 1]->setText(QStringLiteral("%1\n手牌：%2 张　状态：%3")
            .arg(player.name).arg(player.hand.size()).arg(playerStatusText(player)));
    }

    const Player &human = players_[Human];
    playerInfoLabel_->setText(QStringLiteral("你的手牌：%1 张　　状态：%2　　第 %3 轮")
                                  .arg(human.hand.size()).arg(playerStatusText(human)).arg(roundNumber_));
    rankingLabel_->setText(finishOrder_.isEmpty()
        ? QStringLiteral("当前排名：尚未有人出完手牌")
        : QStringLiteral("当前排名：%1").arg(rankingSummary()));

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
    challengeButton_->setEnabled(humanTurn && phase_ == Phase::Decide && challengeAllowed());
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
    if (!claim_.valid || claim_.cards.isEmpty())
        return false;
    for (int card : claim_.cards)
        if (card != claim_.declaredRank)
            return false;
    return true;
}

QString QtWidgetsApplication1::cardName(int rank) const
{
    static const QString names[] = {QStringLiteral("A"), QStringLiteral("K"), QStringLiteral("Q")};
    return (rank >= 0 && rank < 3) ? names[rank] : QStringLiteral("?");
}

QString QtWidgetsApplication1::playerStatusText(const Player &player) const
{
    if (player.rank > 0)
        return QStringLiteral("第 %1 名").arg(player.rank);
    if (player.hand.isEmpty())
        return QStringLiteral("等待最后声明确认");
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
