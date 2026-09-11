#include "config.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QHash>

#include <linux/input-event-codes.h>

int keyCodeFromName(const QString& name)
{
    static QHash<QString, int> map;
    if (map.isEmpty()) {
#define K(name) map.insert(QStringLiteral(#name), name)
        K(KEY_1); K(KEY_2); K(KEY_3); K(KEY_4); K(KEY_5);
        K(KEY_6); K(KEY_7); K(KEY_8); K(KEY_9); K(KEY_0);
        K(KEY_A); K(KEY_B); K(KEY_C); K(KEY_D); K(KEY_E); K(KEY_F);
        K(KEY_G); K(KEY_H); K(KEY_I); K(KEY_J); K(KEY_K); K(KEY_L);
        K(KEY_M); K(KEY_N); K(KEY_O); K(KEY_P); K(KEY_Q); K(KEY_R);
        K(KEY_S); K(KEY_T); K(KEY_U); K(KEY_V); K(KEY_W); K(KEY_X);
        K(KEY_Y); K(KEY_Z);
        K(KEY_F1); K(KEY_F2); K(KEY_F3); K(KEY_F4); K(KEY_F5); K(KEY_F6);
        K(KEY_F7); K(KEY_F8); K(KEY_F9); K(KEY_F10); K(KEY_F11); K(KEY_F12);
        K(KEY_ENTER); K(KEY_SPACE); K(KEY_TAB); K(KEY_ESC);
        K(KEY_BACKSPACE); K(KEY_DELETE); K(KEY_INSERT);
        K(KEY_LEFT); K(KEY_RIGHT); K(KEY_UP); K(KEY_DOWN);
        K(KEY_HOME); K(KEY_END); K(KEY_PAGEUP); K(KEY_PAGEDOWN);
        K(KEY_MINUS); K(KEY_EQUAL); K(KEY_LEFTBRACE); K(KEY_RIGHTBRACE);
        K(KEY_SEMICOLON); K(KEY_APOSTROPHE); K(KEY_GRAVE);
        K(KEY_COMMA); K(KEY_DOT); K(KEY_SLASH); K(KEY_BACKSLASH);
        K(KEY_LEFTCTRL); K(KEY_LEFTALT); K(KEY_LEFTSHIFT);
        K(KEY_RIGHTCTRL); K(KEY_RIGHTALT); K(KEY_RIGHTSHIFT);
        K(KEY_LEFTMETA); K(KEY_RIGHTMETA);
        K(KEY_CAPSLOCK); K(KEY_NUMLOCK);
#undef K
    }
    return map.value(name, -1);
}

Config Config::load(const QString& path)
{
    Config cfg;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open config" << path << "- using defaults";
        return cfg;
    }
    const auto doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) {
        qWarning() << "Config is not a JSON object";
        return cfg;
    }
    const auto obj = doc.object();

    cfg.device        = obj.value("device").toString(cfg.device);
    cfg.triggerButton = obj.value("trigger_button").toInt(cfg.triggerButton);
    cfg.selectionMode = obj.value("selection_mode").toString(cfg.selectionMode);
    cfg.innerRadius   = obj.value("inner_radius").toInt(cfg.innerRadius);
    cfg.outerRadius   = obj.value("outer_radius").toInt(cfg.outerRadius);

    for (const auto& v : obj.value("items").toArray()) {
        const auto o = v.toObject();
        ItemConfig it;
        it.label = o.value("label").toString();
        it.key   = o.value("key").toString();
        if (!it.key.isEmpty())
            cfg.items.push_back(it);
    }
    return cfg;
}