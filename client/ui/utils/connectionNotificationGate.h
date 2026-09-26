#pragma once

#include <QObject>
#include <QTimer>

// Coalesce only explicit quick-switch operations; ordinary VPN events pass through.
class ConnectionNotificationGate : public QObject
{
    Q_OBJECT
public:
    explicit ConnectionNotificationGate(QObject *parent = nullptr, int quietPeriodMs = 1500);
    void begin();
    void end(bool success);
    bool shouldNotify(bool connected);

signals:
    void settledConnected();

private:
    QTimer m_quietTimer;
    bool m_active = false;
    bool m_waiting = false;
    bool m_finishing = false;
    bool m_connected = false;
    int m_generation = 0;
};
