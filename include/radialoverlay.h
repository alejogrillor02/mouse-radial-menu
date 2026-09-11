#pragma once

#include <QWidget>
#include <QPointF>
#include "config.h"

class RadialOverlay : public QWidget {
    Q_OBJECT
public:
    explicit RadialOverlay(QWidget* parent = nullptr);

    void setConfig(const Config& cfg);

    void begin();
    void end();
    void scroll(int delta);

    int selectedIndex() const { return m_selected; }

public slots:
    // Called whenever evdev reports a mouse delta.
    void applyMotion(int dx, int dy);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void recomputeSelection();

    Config   m_cfg;
    int      m_selected   = 0;
    bool     m_scrollMode = false;
    QPointF  m_virtualPos;   // in widget-local coordinates
};
