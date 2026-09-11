#include "uinputkeyboard.h"

#include <linux/uinput.h>
#include <linux/input-event-codes.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <QDebug>

UinputKeyboard::UinputKeyboard()
{
    m_fd = ::open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (m_fd < 0) {
        qWarning() << "open /dev/uinput failed:" << strerror(errno);
        return;
    }

    ioctl(m_fd, UI_SET_EVBIT, EV_KEY);
    ioctl(m_fd, UI_SET_EVBIT, EV_SYN);
    for (int i = 0; i < KEY_MAX; ++i)
        ioctl(m_fd, UI_SET_KEYBIT, i);

    struct uinput_setup usetup {};
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor  = 0x1234;
    usetup.id.product = 0x5678;
    ::strncpy(usetup.name, "radial-keyboard", UINPUT_MAX_NAME_SIZE - 1);

    if (ioctl(m_fd, UI_DEV_SETUP, &usetup) < 0 ||
        ioctl(m_fd, UI_DEV_CREATE) < 0) {
        qWarning() << "uinput setup failed:" << strerror(errno);
        ::close(m_fd);
        m_fd = -1;
        return;
    }

    // Give userspace (X server / libinput) a moment to notice the new device.
    ::usleep(250 * 1000);
}

UinputKeyboard::~UinputKeyboard()
{
    if (m_fd >= 0) {
        ioctl(m_fd, UI_DEV_DESTROY);
        ::close(m_fd);
    }
}

void UinputKeyboard::emitEvent(int type, int code, int value)
{
    struct input_event ev {};
    ev.type  = static_cast<__u16>(type);
    ev.code  = static_cast<__u16>(code);
    ev.value = value;
    if (::write(m_fd, &ev, sizeof(ev)) < 0)
        qWarning() << "uinput write failed:" << strerror(errno);
}

void UinputKeyboard::tap(int keycode)
{
    if (m_fd < 0) return;
    emitEvent(EV_KEY, keycode, 1);
    emitEvent(EV_SYN, SYN_REPORT, 0);
    ::usleep(20 * 1000);
    emitEvent(EV_KEY, keycode, 0);
    emitEvent(EV_SYN, SYN_REPORT, 0);
}