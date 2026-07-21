#pragma once

#include <QHash>
#include <QString>
#include <QStringList>

class KeyValueConfig
{
public:
    bool load(const QString& path, QString* errorMessage = nullptr);
    QString value(const QString& key, const QString& fallback = QString()) const;
    bool contains(const QString& key) const;
    void setValue(const QString& key, const QString& value);
    bool save(QString* errorMessage = nullptr) const;

private:
    void rebuildIndex();

    QString path_;
    QStringList lines_;
    QHash<QString, int> keyLines_;
    bool trailingNewline_ = true;
};
