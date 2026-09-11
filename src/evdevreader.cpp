#include "evdevreader.h"

#include <linux/input.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <errno.h>
#include <QDebug>

namespace {

QString buttonName(int code)
{
    switch (code) {
    case BTN_LEFT:    return QStringLiteral("BTN_LEFT(272)");
    case BTN_RIGHT:   return QStringLiteral("BTN_RIGHT(273)");
    case BTN_MIDDLE:  return QStringLiteral("BTN_MIDDLE(274)");
    case BTN_SIDE:    return QStringLiteral("BTN_SIDE(275)");
    case BTN_EXTRA:   return QStringLiteral("BTN_EXTRA(276)");
    case BTN_FORWARD: return QStringLiteral("BTN_FORWARD(277)");
    case BTN_BACK:    return QStringLiteral("BTN_BACK(278)");
    case BTN_TASK:    return QStringLiteral("BTN_TASK(279)");
    default:          return QStringLiteral("BTN_%1").arg(code);
    }
}

} // namespace

QVector<DeviceInfo> EvdevReader::enumerateDevices(int trigger)
{
    QVector<DeviceInfo> out;

    for (int i = 0; i < 64; ++i) {
        const QString path = QStringLiteral("/dev/input/event%1").arg(i);
        int fd = ::open(path.toLocal8Bit().constData(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;

        unsigned long evbits [(EV_MAX  / 8) + 1] = {0};
        unsigned long relbits[(REL_MAX / 8) + 1] = {0};
        unsigned long keybits[(KEY_MAX / 8) + 1] = {0};

        DeviceInfo info;
        info.path = path;

        char nameBuf[256] = {0};
        if (ioctl(fd, EVIOCGNAME(sizeof(nameBuf) - 1), nameBuf) >= 0)
            info.name = QString::fromLocal8Bit(nameBuf);

        ioctl(fd, EVIOCGBIT(0,       sizeof(evbits)),  evbits);
        ioctl(fd, EVIOCGBIT(EV_REL,  sizeof(relbits)), relbits);
        ioctl(fd, EVIOCGBIT(EV_KEY,  sizeof(keybits)), keybits);

        const bool hasRel = (evbits[EV_REL / 8] & (1UL << (EV_REL % 8)));
        const bool hasKey = (evbits[EV_KEY / 8] & (1UL << (EV_KEY % 8)));

        info.hasRelXY = hasRel
                     && (relbits[REL_X / 8] & (1UL << (REL_X % 8)))
                     && (relbits[REL_Y / 8] & (1UL << (REL_Y % 8)));
        info.hasTrigger = hasKey
                       && (keybits[trigger / 8] & (1UL << (trigger % 8)));

        if (hasKey) {
            const int hi = qMin<int>(BTN_GEAR_UP, KEY_MAX);
            for (int code = BTN_MISC; code <= hi; ++code) {
                if (keybits[code / 8] & (1UL << (code % 8)))
                    info.buttons.append(code);
            }
        }

        ::close(fd);
        out.push_back(info);
    }
    return out;
}

namespace {

QString autoDetectDevice(int trigger)
{
    const auto all = EvdevReader::enumerateDevices(trigger);
    // Prefer a full pointer (has REL_X/REL_Y) that also has the trigger button.
    for (const auto& d : all)
        if (d.hasRelXY && d.hasTrigger) return d.path;
    // Fall back to any device that has the trigger button at all.
    for (const auto& d : all)
        if (d.hasTrigger) return d.path;
    return {};
}

} // namespace

EvdevReader::EvdevReader(const QString& device, int triggerButton, QObject* parent)
    : QThread(parent), m_device(device), m_trigger(triggerButton)
{
    if (m_device.isEmpty())
        m_device = autoDetectDevice(m_trigger);
}

EvdevReader::~EvdevReader()
{
    stop();
    wait(1000);
}

void EvdevReader::stop()
{
    m_running = false;
}

void EvdevReader::run()
{
    if (m_device.isEmpty()) {
        qWarning() << "EvdevReader: no device path";
        return;
    }

    const int fd = ::open(m_device.toLocal8Bit().constData(), O_RDONLY);
    if (fd < 0) {
        qWarning() << "Cannot open" << m_device << ":" << strerror(errno);
        return;
    }

    struct pollfd pfd { fd, POLLIN, 0 };
    struct input_event ev;

    while (m_running) {
        const int r = ::poll(&pfd, 1, 100);
        if (r <= 0) continue;
        const ssize_t n = ::read(fd, &ev, sizeof(ev));
        if (n != static_cast<ssize_t>(sizeof(ev))) continue;

        if (ev.type == EV_KEY && ev.code == m_trigger) {
            if (ev.value == 1)      emit triggerPressed();
            else if (ev.value == 0) emit triggerReleased();
        } else if (ev.type == EV_REL) {
            if (ev.code == REL_WHEEL && ev.value != 0)
                emit scrolled(ev.value);
            else if (ev.code == REL_X && ev.value != 0)
                emit moved(ev.value, 0);
            else if (ev.code == REL_Y && ev.value != 0)
                emit moved(0, ev.value);
        }
    }

    ::close(fd);
}