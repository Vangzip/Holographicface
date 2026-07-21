#include "KeyValueConfig.h"

#include <QFile>
#include <QSaveFile>

namespace {

QString normalizedKey(const QString& key)
{
    return key.trimmed().toLower();
}

bool parseLine(const QString& line, QString* key, int* equalsPosition)
{
    const QString trimmed = line.trimmed();
    if (trimmed.isEmpty() || trimmed.startsWith('#') || trimmed.startsWith(';')
        || (trimmed.startsWith('[') && trimmed.endsWith(']'))) {
        return false;
    }

    const int equals = line.indexOf('=');
    if (equals < 0) {
        return false;
    }
    const QString parsedKey = normalizedKey(line.left(equals));
    if (parsedKey.isEmpty()) {
        return false;
    }
    if (key) {
        *key = parsedKey;
    }
    if (equalsPosition) {
        *equalsPosition = equals;
    }
    return true;
}

void setError(QString* errorMessage, const QString& message)
{
    if (errorMessage) {
        *errorMessage = message;
    }
}

} // namespace

bool KeyValueConfig::load(const QString& path, QString* errorMessage)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setError(errorMessage, QString::fromUtf8("无法读取配置文件：\n") + path);
        return false;
    }

    path_ = path;
    QString text = QString::fromUtf8(file.readAll());
    text.replace("\r\n", "\n");
    text.replace('\r', '\n');
    trailingNewline_ = text.endsWith('\n');
    if (trailingNewline_) {
        text.chop(1);
    }
    lines_ = text.isEmpty() ? QStringList{} : text.split('\n');
    rebuildIndex();
    return true;
}

QString KeyValueConfig::value(const QString& key, const QString& fallback) const
{
    const auto it = keyLines_.constFind(normalizedKey(key));
    if (it == keyLines_.constEnd()) {
        return fallback;
    }
    int equals = -1;
    if (!parseLine(lines_.at(it.value()), nullptr, &equals)) {
        return fallback;
    }
    return lines_.at(it.value()).mid(equals + 1).trimmed();
}

bool KeyValueConfig::contains(const QString& key) const
{
    return keyLines_.contains(normalizedKey(key));
}

void KeyValueConfig::setValue(const QString& key, const QString& value)
{
    const QString normalized = normalizedKey(key);
    const auto it = keyLines_.constFind(normalized);
    if (it == keyLines_.constEnd()) {
        lines_.append(key.trimmed() + '=' + value);
        keyLines_.insert(normalized, lines_.size() - 1);
        trailingNewline_ = true;
        return;
    }

    const int lineIndex = it.value();
    int equals = -1;
    if (!parseLine(lines_.at(lineIndex), nullptr, &equals)) {
        return;
    }
    lines_[lineIndex] = lines_.at(lineIndex).left(equals + 1) + value;
}

bool KeyValueConfig::save(QString* errorMessage) const
{
    QSaveFile file(path_);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        setError(errorMessage, QString::fromUtf8("无法写入配置文件：\n") + path_);
        return false;
    }

    QByteArray data = lines_.join('\n').toUtf8();
    if (trailingNewline_ || !data.isEmpty()) {
        data.append('\n');
    }
    if (file.write(data) != data.size() || !file.commit()) {
        setError(errorMessage, QString::fromUtf8("无法原子保存配置文件：\n") + path_);
        return false;
    }
    return true;
}

void KeyValueConfig::rebuildIndex()
{
    keyLines_.clear();
    for (int index = 0; index < lines_.size(); ++index) {
        QString key;
        if (parseLine(lines_.at(index), &key, nullptr)) {
            keyLines_.insert(key, index);
        }
    }
}
