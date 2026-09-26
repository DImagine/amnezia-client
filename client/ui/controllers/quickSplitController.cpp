#include "quickSplitController.h"

QuickSplitController::QuickSplitController(QObject *parent, int disconnectTimeoutMs, int reconnectTimeoutMs)
    : QObject(parent), m_disconnectTimeoutMs(disconnectTimeoutMs), m_reconnectTimeoutMs(reconnectTimeoutMs)
{
    m_timeout.setSingleShot(true);
    connect(&m_timeout, &QTimer::timeout, this, &QuickSplitController::abort);
}

void QuickSplitController::setModes(bool sites, bool apps)
{
    // Represent legacy mixed settings honestly until the user chooses a preset.
    m_mode = sites && apps ? -1 : sites ? 1 : apps ? 2 : 0;
    emit changed();
}

void QuickSplitController::setContext(const QString &context, bool available)
{
    const bool interrupted = busy() && (context != m_context || !available);
    m_context = context;
    m_available = available && !context.isEmpty();
    if (interrupted)
        abort();
    emit changed();
}

void QuickSplitController::requestMode(int mode)
{
    if (mode < 0 || mode > 2 || mode == m_mode || busy() || !m_available
        || (m_link != Disconnected && m_link != Connected))
        return;
    m_pendingMode = mode;
    m_applied = false;
    ++m_generation;
    if (m_link == Connected) {
        m_automaticSwitch = true;
        emit automaticSwitchStarted(); // Suppress notifications before requesting disconnection.
        m_phase = Disconnecting;
        m_timeout.start(m_disconnectTimeoutMs);
        emit changed();
        emit disconnectRequested();
    } else {
        m_phase = Applying;
        emit changed();
        applyPending(false);
    }
}

void QuickSplitController::observeConnection(LinkState state)
{
    m_link = state;
    if (!busy())
        return;
    if (state == Failed) {
        fail();
    } else if (m_phase == Disconnecting && state == Disconnected) {
        m_timeout.stop();
        m_phase = Applying;
        emit changed();
        const int generation = m_generation;
        // Let the disconnect signal finish unwinding before requesting a new connection.
        QTimer::singleShot(0, this, [this, generation]() {
            if (m_generation == generation && m_phase == Applying)
                applyPending(true);
        });
    } else if (m_phase == Reconnecting && state == Connected) {
        finish(true);
    } else if ((m_phase == Reconnecting && state == Disconnected)
               || (m_phase == Applying && state != Disconnected)) {
        fail();
    }
}

void QuickSplitController::applyPending(bool reconnect)
{
    if (!m_available || m_link != Disconnected) {
        fail();
        return;
    }
    const int generation = m_generation;
    emit applyMode(m_pendingMode);
    if (generation != m_generation)
        return;
    m_applied = true;
    if (!reconnect) {
        finish();
        return;
    }
    m_phase = Reconnecting;
    m_timeout.start(m_reconnectTimeoutMs);
    emit changed();
    emit reconnectRequested();
    // A rejected request may not emit a connection-state change at all.
    if (m_phase == Reconnecting && m_link == Disconnected)
        fail();
}

void QuickSplitController::connectionFailed()
{
    if (busy())
        fail(false); // The caller is already showing the specific stock error or dialog.
}

void QuickSplitController::abort()
{
    const bool stopReconnect = m_phase == Reconnecting;
    fail();
    if (stopReconnect)
        emit cancelReconnectRequested();
}

void QuickSplitController::fail(bool report)
{
    const QString message = m_applied
        ? tr("Mode saved. Automatic reconnection did not complete; check the VPN connection.")
        : tr("Could not switch the mode. Split tunneling settings were not changed.");
    finish();
    if (report)
        emit failed(message);
}

void QuickSplitController::finish(bool success)
{
    m_timeout.stop();
    ++m_generation;
    m_phase = Idle;
    m_pendingMode = -1;
    if (m_automaticSwitch) {
        m_automaticSwitch = false;
        emit automaticSwitchFinished(success);
    }
    emit changed();
}
