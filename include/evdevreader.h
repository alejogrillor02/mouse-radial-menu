#pragma once

#include <QThread>
#include <QString>
#include <QVector>
#include <atomic>

struct DeviceInfo {
    QString       path;
    QString       name;
    bool          hasRelXY   = false;
    bool          hasTrigger = false;
    QVector<int>  buttons;   // all BTN_* codes exposed
};

class EvdevReader : public QThread {
    Q_OBJECT
public:
    EvdevReader(const QString& device, int triggerButton, QObject* parent = nullptr);
    ~EvdevReader() override;

    QString devicePath() const { return m_device; }
    void stop();

    // Enumerate /dev/input/event* and report what each one advertises.
    static QVector<DeviceInfo> enumerateDevices(int trigger);

signals:
    void triggerPressed();
    void triggerReleased();
    void scrolled(int delta);
    void moved(int dx, int dy);

protected:
    void run() override;

private:
    QString m_device;
    int     m_trigger;
    std::atomic<bool> m_running { true };
};
