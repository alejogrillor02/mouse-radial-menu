#pragma once
#include <QString>
#include <QVector>

struct ItemConfig {
    QString label;
    QString key;   // e.g. "KEY_1"
};

struct Config {
    QString device;                 // empty = auto-detect
    int triggerButton = 276;        // BTN_EXTRA
    QString selectionMode = "mouse";// "mouse" | "scroll"
    int innerRadius = 130;
    int outerRadius = 280;
    QVector<ItemConfig> items;

    static Config load(const QString& path);
};

int keyCodeFromName(const QString& name);
