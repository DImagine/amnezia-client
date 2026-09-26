#include "connectionNotificationGate.h"

ConnectionNotificationGate::ConnectionNotificationGate(QObject *parent, int quietPeriodMs)
    : QObject(parent)
{
    m_quietTimer.setSingleShot(true);
    m_quietTimer.setInterval(quietPeriodMs);
    connect(&m_quietTimer, &QTimer::timeout, this, [this]() {
        m_waiting = false;
        if (!m_active && m_connected)
            emit settledConnected();
    });
}

void ConnectionNotificationGate::begin()
{
    ++m_generation;
    m_quietTimer.stop();
    m_waiting = false;
    m_finishing = false;
    m_active = true;
}

void ConnectionNotificationGate::end(bool success)
{
    if (!m_active)
        return;
    m_active = false;
    if (success) {
        m_waiting = true;
        m_quietTimer.start();
    } else {
        // Keep the failure's terminal state silent even when its observers run after us.
        m_finishing = true;
        const int generation = ++m_generation;
        QTimer::singleShot(0, this, [this, generation]() {
            if (generation == m_generation)
                m_finishing = false;
        });
    }
}

bool ConnectionNotificationGate::shouldNotify(bool connected)
{
    m_connected = connected;
    if (!m_active && m_waiting && !connected) {
        // A subsequent ordinary disconnect/failure invalidates any pending success.
        m_quietTimer.stop();
        m_waiting = false;
    }
    return !m_active && !m_waiting && !m_finishing;
}
