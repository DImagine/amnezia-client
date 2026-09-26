#pragma once

#include <QObject>
#include <QTimer>

// Coordinates a single settings change using confirmed connection states.
class QuickSplitController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int mode READ mode NOTIFY changed)
    Q_PROPERTY(int pendingMode READ pendingMode NOTIFY changed)
    Q_PROPERTY(int phase READ phase NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool available READ available NOTIFY changed)

public:
    enum LinkState { Disconnected, Connected, Transition, Failed };
    enum Phase { Idle, Disconnecting, Applying, Reconnecting };
    explicit QuickSplitController(QObject *parent = nullptr, int disconnectTimeoutMs = 30000,
                                  int reconnectTimeoutMs = 120000);
    int mode() const { return m_mode; }
    int pendingMode() const { return m_pendingMode; }
    int phase() const { return m_phase; }
    bool busy() const { return m_phase != Idle; }
    bool available() const { return m_available; }
    void setModes(bool sites, bool apps);
    void setContext(const QString &context, bool available);
    void observeConnection(LinkState state);
    void connectionFailed();

public slots:
    void requestMode(int mode);

signals:
    void changed();
    void applyMode(int mode);
    void disconnectRequested();
    void reconnectRequested();
    void cancelReconnectRequested();
    void failed(const QString &message);
    void automaticSwitchStarted();
    void automaticSwitchFinished(bool success);

private:
    void applyPending(bool reconnect);
    void finish(bool success = false);
    void fail(bool report = true);
    void abort();
    int m_mode = 0;
    int m_pendingMode = -1;
    int m_generation = 0;
    Phase m_phase = Idle;
    LinkState m_link = Disconnected;
    bool m_available = false;
    bool m_applied = false;
    bool m_automaticSwitch = false;
    QString m_context;
    QTimer m_timeout;
    int m_disconnectTimeoutMs;
    int m_reconnectTimeoutMs;
};
