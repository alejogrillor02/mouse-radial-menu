#include <QApplication>
#include <QObject>
#include <QDebug>
#include <QStringList>

#include "config.h"
#include "uinputkeyboard.h"
#include "evdevreader.h"
#include "radialoverlay.h"

class Controller : public QObject {
    Q_OBJECT
public:
    Controller(const Config& cfg, UinputKeyboard* kb, RadialOverlay* overlay,
               QObject* parent = nullptr)
        : QObject(parent), m_cfg(cfg), m_kb(kb), m_overlay(overlay) {}

public slots:
    void onTriggerPressed() {
        if (m_active) return;
        m_active = true;
        m_overlay->begin();
    }

    void onTriggerReleased() {
        if (!m_active) return;
        m_active = false;

        const int idx = m_overlay->selectedIndex();
        m_overlay->end();

        if (idx < 0 || idx >= m_cfg.items.size()) return;
        const int code = keyCodeFromName(m_cfg.items[idx].key);
        if (code > 0)
            m_kb->tap(code);
        else
            qWarning() << "Unknown key name:" << m_cfg.items[idx].key;
    }

    void onScroll(int delta) {
        if (m_active) m_overlay->scroll(delta);
    }

private:
    const Config&   m_cfg;
    UinputKeyboard* m_kb;
    RadialOverlay*  m_overlay;
    bool            m_active = false;
};

static void printDeviceList(int trigger)
{
    const auto all = EvdevReader::enumerateDevices(trigger);

    qInfo().noquote() << QStringLiteral("--- Devices exposing the trigger button (%1) ---").arg(trigger);
    bool any = false;
    for (const auto& d : all) {
        if (!d.hasTrigger) continue;
        any = true;
        qInfo().noquote() << QStringLiteral("  * %1  \"%2\"%3")
                             .arg(d.path, d.name,
                                  d.hasRelXY ? QStringLiteral("  [has REL_X/Y]") : QString());
    }
    if (!any)
        qInfo().noquote() << "  (none)";

    qInfo().noquote() << "";
    qInfo().noquote() << "--- All pointer-like devices (REL_X + REL_Y) ---";
    for (const auto& d : all) {
        if (!d.hasRelXY) continue;
        QStringList btns;
        for (int b : d.buttons) btns << QStringLiteral("%1").arg(b);
        qInfo().noquote() << QStringLiteral("    %1  \"%2\"")
                             .arg(d.path, d.name);
        if (!btns.isEmpty())
            qInfo().noquote() << QStringLiteral("        buttons: %1").arg(btns.join(", "));
    }
    qInfo().noquote() << "";
    qInfo().noquote() << "Set \"device\" in config.json to one of the paths above to pin it.";
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);

    QString cfgPath = QStringLiteral("config.json");
    bool listMode = false;

    const QStringList args = QCoreApplication::arguments();
    for (int i = 1; i < args.size(); ++i) {
        const QString& a = args[i];
        if (a == "--list" || a == "-l")
            listMode = true;
        else if (!a.startsWith('-'))
            cfgPath = a;
    }

    Config cfg = Config::load(cfgPath);

    if (listMode) {
        printDeviceList(cfg.triggerButton);
        return 0;
    }

    if (cfg.items.isEmpty()) {
        qWarning() << "No items in config; nothing to do.";
        return 1;
    }

    UinputKeyboard kb;
    if (!kb.isValid()) {
        qWarning() << "Cannot create virtual keyboard. Are you root,"
                      " or in the 'input' group with a uinput udev rule?";
        return 1;
    }

    RadialOverlay overlay;
    overlay.setConfig(cfg);

    // If the user pinned a device, use it verbatim. Otherwise fall back to
    // auto-detect (which now also accepts a non-pointer node exposing the key).
    EvdevReader reader(cfg.device, cfg.triggerButton);
    if (reader.devicePath().isEmpty()) {
        qWarning().noquote()
            << QStringLiteral("No event node exposing trigger button %1 was found.")
                   .arg(cfg.triggerButton);
        qWarning().noquote() << "Run with --list to see available devices and buttons.";
        return 1;
    }
    qInfo().noquote() << QStringLiteral("Listening on %1 for button %2")
                         .arg(reader.devicePath()).arg(cfg.triggerButton);

    Controller ctl(cfg, &kb, &overlay);
    QObject::connect(&reader, &EvdevReader::triggerPressed,
                     &ctl,    &Controller::onTriggerPressed);
    QObject::connect(&reader, &EvdevReader::triggerReleased,
                     &ctl,    &Controller::onTriggerReleased);
    QObject::connect(&reader, &EvdevReader::scrolled,
                     &ctl,    &Controller::onScroll);
    QObject::connect(&reader, &EvdevReader::moved,
                     &overlay, &RadialOverlay::applyMotion);

    reader.start();
    return app.exec();
}

#include "main.moc"