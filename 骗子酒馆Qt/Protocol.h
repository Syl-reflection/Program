#pragma once

// 联机协议工具：消息类型、JSON 编解码、TCP 帧、席位类型。
// 仅依赖 QtCore，header-only，供房主/客户端双方共用。

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>

#if defined(_MSC_VER)
#pragma execution_character_set("utf-8")
#endif

namespace Protocol {

constexpr quint16 DefaultPort = 35720;

// —— 消息类型 ——
namespace Msg {
inline const QString Announce = QStringLiteral("tavern_announce"); // UDP 房间公告
inline const QString Hello    = QStringLiteral("hello");           // 客户端→房主 握手
inline const QString Welcome  = QStringLiteral("welcome");         // 房主→客户端 分配 playerId
inline const QString Lobby    = QStringLiteral("lobby");           // 大厅状态（席位/玩家）
inline const QString Start    = QStringLiteral("start");           // 房主开局
inline const QString State    = QStringLiteral("state");           // 权威状态快照
inline const QString Action   = QStringLiteral("action");          // 客户端→房主 操作
inline const QString Sync     = QStringLiteral("sync");            // 客户端→房主 请求当前状态
inline const QString Log      = QStringLiteral("log");             // 房主→客户端 同步一条日志
inline const QString RewardResult = QStringLiteral("rewardResult");// 房主→客户端 奖励私密结果
inline const QString GameOver = QStringLiteral("gameover");        // 终局与排名
inline const QString Error    = QStringLiteral("error");           // 错误信息
inline const QString Bye      = QStringLiteral("bye");             // 离开
}

// —— 席位类型（联机房间大厅用）——
enum class SeatKind { Human, Ai, Empty };

// 席位类型 → 字符串 / 反向解析
inline QString seatKindToString(SeatKind kind)
{
    switch (kind) {
    case SeatKind::Human: return QStringLiteral("human");
    case SeatKind::Ai:    return QStringLiteral("ai");
    case SeatKind::Empty: return QStringLiteral("empty");
    }
    return QStringLiteral("empty");
}

inline SeatKind seatKindFromString(const QString &s)
{
    if (s == QStringLiteral("human")) return SeatKind::Human;
    if (s == QStringLiteral("ai"))    return SeatKind::Ai;
    return SeatKind::Empty;
}

// —— JSON 编解码 ——

// 把 type + payload 编码为紧凑 JSON（消息 = payload 基础上插入 type 字段）
inline QByteArray encode(const QString &type, const QJsonObject &payload = {})
{
    QJsonObject obj = payload;
    obj.insert(QStringLiteral("type"), type);
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

// 解码消息，返回是否成功；type 与 payload 通过指针带出
inline bool decode(const QByteArray &bytes, QString *type, QJsonObject *payload)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(bytes, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return false;
    const QJsonObject obj = doc.object();
    if (type)
        *type = obj.value(QStringLiteral("type")).toString();
    if (payload)
        *payload = obj;
    return true;
}

// —— TCP 消息分帧 ——
// 每条消息 = 4 字节大端长度前缀 + 紧凑 JSON 字节。
// 发送端调用 frame() 得到可一次 write 的块；接收端累积字节并用
// takeFrame() 逐个取出完整消息。

inline QByteArray frame(const QByteArray &payload)
{
    QByteArray out;
    out.reserve(payload.size() + 4);
    const quint32 size = static_cast<quint32>(payload.size());
    out.append(static_cast<char>((size >> 24) & 0xff));
    out.append(static_cast<char>((size >> 16) & 0xff));
    out.append(static_cast<char>((size >> 8) & 0xff));
    out.append(static_cast<char>(size & 0xff));
    out.append(payload);
    return out;
}

// 从缓冲中取出一整帧。成功返回 true 并置 payload；数据不足返回 false。
// 注意：buffer 为引用，取出后调用方应删除已消费的字节（见 removeConsumed）。
inline bool takeFrame(const QByteArray &buffer, QByteArray *payload, int *consumed)
{
    if (buffer.size() < 4)
        return false;
    const auto u8 = [&](int i) { return static_cast<quint8>(buffer.at(i)); };
    const quint32 size = (u8(0) << 24) | (u8(1) << 16) | (u8(2) << 8) | u8(3);
    if (size > (1u << 24)) // 防御异常长度
        return false;
    const int total = 4 + static_cast<int>(size);
    if (buffer.size() < total)
        return false;
    if (payload)
        *payload = buffer.mid(4, static_cast<int>(size));
    if (consumed)
        *consumed = total;
    return true;
}

} // namespace Protocol
