#include <QtTest>
#include "../../core/utils/connectionPreparationGuard.h"
#include "../../ui/controllers/quickSplitController.h"

class TestQuickSplit : public QObject
{
    Q_OBJECT
private slots:
    void staleValidationCannotConnectOrConsumeNewRequest()
    {
        ConnectionPreparationGuard guard;
        const auto old = guard.begin("server/awg");
        guard.cancel();
        const auto current = guard.begin("other/wg");
        QVERIFY(!guard.consume(old, "other/wg"));
        QVERIFY(guard.active()); // Old successes and errors must leave the new request pending.
        QVERIFY(guard.consume(current, "other/wg"));
        QVERIFY(!guard.consume(current, "other/wg")); // A duplicate result cannot reconnect twice.
        const auto changed = guard.begin("server/awg");
        QVERIFY(!guard.consume(changed, "server/wg"));
        QVERIFY(!guard.active());
    }
    void reconnectTimeoutCancelsLateValidation()
    {
        QuickSplitController c(nullptr, 1000, 25);
        ConnectionPreparationGuard guard;
        quint64 request = 0;
        c.setContext("server/awg", true);
        c.observeConnection(QuickSplitController::Connected);
        connect(&c, &QuickSplitController::reconnectRequested, &c, [&]() {
            request = guard.begin("server/awg");
            c.observeConnection(QuickSplitController::Transition);
        });
        connect(&c, &QuickSplitController::automaticSwitchFinished, &c, [&](bool) { guard.cancel(); });
        connect(&c, &QuickSplitController::cancelReconnectRequested, &c, [&]() {
            c.observeConnection(QuickSplitController::Disconnected);
        });
        QSignalSpy cancel(&c, &QuickSplitController::cancelReconnectRequested);
        QSignalSpy errors(&c, &QuickSplitController::failed);
        QSignalSpy apply(&c, &QuickSplitController::applyMode);
        c.requestMode(1);
        c.observeConnection(QuickSplitController::Disconnected);
        QTRY_COMPARE(cancel.count(), 1);
        QVERIFY(request != 0);
        QVERIFY(!guard.consume(request, "server/awg"));
        QCOMPARE(errors.count(), 1);
        QVERIFY(!c.busy());
        c.requestMode(2); // The user can try again after cancellation.
        QCOMPARE(apply.count(), 2);
    }
    void specificErrorDoesNotDuplicateAndNextClickWorks()
    {
        QuickSplitController c;
        c.setContext("server/awg", true);
        c.observeConnection(QuickSplitController::Connected);
        QSignalSpy errors(&c, &QuickSplitController::failed);
        QSignalSpy apply(&c, &QuickSplitController::applyMode);
        c.requestMode(1);
        c.connectionFailed(); // Specific stock error arrives before nested Disconnected.
        c.observeConnection(QuickSplitController::Disconnected);
        c.observeConnection(QuickSplitController::Disconnected); // Outer UI notification reads settled state.
        c.requestMode(2);
        QCOMPARE(errors.count(), 0);
        QCOMPARE(apply.count(), 1);
        QVERIFY(!c.busy());
    }
    void disconnectTimeoutLeavesSettingsUntouched()
    {
        QuickSplitController c(nullptr, 25, 1000);
        c.setContext("server/awg", true);
        c.observeConnection(QuickSplitController::Connected);
        QSignalSpy apply(&c, &QuickSplitController::applyMode);
        QSignalSpy cancel(&c, &QuickSplitController::cancelReconnectRequested);
        QSignalSpy errors(&c, &QuickSplitController::failed);
        c.requestMode(1);
        QTRY_COMPARE(errors.count(), 1);
        QCOMPARE(apply.count(), 0);
        QCOMPARE(cancel.count(), 0); // Do not issue another disconnect after a disconnect timeout.
        c.observeConnection(QuickSplitController::Disconnected);
        QCoreApplication::processEvents();
        QCOMPARE(apply.count(), 0);
    }
    void changedContextStopsInFlightReconnect()
    {
        QuickSplitController c;
        c.setContext("server/awg", true);
        c.observeConnection(QuickSplitController::Connected);
        connect(&c, &QuickSplitController::reconnectRequested, &c, [&]() {
            c.observeConnection(QuickSplitController::Transition);
        });
        QSignalSpy cancel(&c, &QuickSplitController::cancelReconnectRequested);
        QSignalSpy errors(&c, &QuickSplitController::failed);
        c.requestMode(1);
        c.observeConnection(QuickSplitController::Disconnected);
        QTRY_COMPARE(c.phase(), int(QuickSplitController::Reconnecting));
        c.setContext("other/wg", true);
        QCOMPARE(cancel.count(), 1);
        QCOMPARE(errors.count(), 1);
        QVERIFY(!c.busy());
    }
    void offlinePresets_data()
    {
        QTest::addColumn<int>("target");
        for (int mode = 0; mode < 3; ++mode)
            QTest::newRow(qPrintable(QString::number(mode))) << mode;
    }
    void offlinePresets()
    {
        QFETCH(int, target);
        QuickSplitController c;
        c.setContext("server/awg", true);
        c.setModes(true, true);
        QSignalSpy apply(&c, &QuickSplitController::applyMode);
        QSignalSpy disconnect(&c, &QuickSplitController::disconnectRequested);
        QSignalSpy reconnect(&c, &QuickSplitController::reconnectRequested);
        c.requestMode(target);
        QCOMPARE(apply.count(), 1);
        QCOMPARE(apply.first().first().toInt(), target);
        QCOMPARE(disconnect.count(), 0);
        QCOMPARE(reconnect.count(), 0);
        QVERIFY(!c.busy());
    }
    void waitsForDisconnectAndReconnect()
    {
        QuickSplitController c;
        c.setContext("server/awg", true);
        c.observeConnection(QuickSplitController::Connected);
        QStringList order;
        connect(&c, &QuickSplitController::disconnectRequested, &c, [&]() { order << "disconnect"; });
        connect(&c, &QuickSplitController::applyMode, &c, [&](int mode) {
            order << "apply";
            c.setModes(mode == 1, mode == 2);
        });
        connect(&c, &QuickSplitController::reconnectRequested, &c, [&]() {
            order << "reconnect";
            c.observeConnection(QuickSplitController::Transition);
        });
        c.requestMode(1);
        c.requestMode(2); // A second click must not change the pending transaction.
        QCOMPARE(order, QStringList{"disconnect"});
        QCOMPARE(c.pendingMode(), 1);
        c.observeConnection(QuickSplitController::Transition);
        QCOMPARE(order.size(), 1);
        c.observeConnection(QuickSplitController::Disconnected);
        QCOMPARE(order.size(), 1); // The apply runs after the disconnect signal unwinds.
        QTRY_COMPARE(order.size(), 3);
        QCOMPARE(order, (QStringList{"disconnect", "apply", "reconnect"}));
        QCOMPARE(c.mode(), 1);
        QVERIFY(c.busy());
        c.observeConnection(QuickSplitController::Connected);
        QVERIFY(!c.busy());
        QCOMPARE(c.pendingMode(), -1);
    }
    void failedDisconnectPreservesSettings()
    {
        QuickSplitController c;
        c.setContext("server/awg", true);
        c.observeConnection(QuickSplitController::Connected);
        QSignalSpy apply(&c, &QuickSplitController::applyMode);
        QSignalSpy failure(&c, &QuickSplitController::failed);
        c.requestMode(1);
        c.observeConnection(QuickSplitController::Failed);
        c.observeConnection(QuickSplitController::Disconnected);
        QCoreApplication::processEvents();
        QCOMPARE(apply.count(), 0);
        QCOMPARE(failure.count(), 1);
        QVERIFY(!c.busy());
    }
    void changedServerCancelsQueuedReconnect()
    {
        QuickSplitController c;
        c.setContext("server/awg", true);
        c.observeConnection(QuickSplitController::Connected);
        QSignalSpy apply(&c, &QuickSplitController::applyMode);
        QSignalSpy reconnect(&c, &QuickSplitController::reconnectRequested);
        c.requestMode(1);
        c.observeConnection(QuickSplitController::Disconnected);
        c.setContext("other/awg", true);
        QCoreApplication::processEvents();
        QCOMPARE(apply.count(), 0);
        QCOMPARE(reconnect.count(), 0);
        QVERIFY(!c.busy());
    }
    void rejectedReconnectEndsTransaction()
    {
        QuickSplitController c;
        c.setContext("server/awg", true);
        c.observeConnection(QuickSplitController::Connected);
        QSignalSpy failure(&c, &QuickSplitController::failed);
        c.requestMode(2);
        c.observeConnection(QuickSplitController::Disconnected);
        // No receiver starts a connection, simulating validation rejection.
        QTRY_COMPARE(failure.count(), 1);
        QVERIFY(!c.busy());
        QVERIFY(failure.first().first().toString().contains("Mode saved"));
    }
    void guardsInvalidRequests()
    {
        QuickSplitController c;
        QSignalSpy apply(&c, &QuickSplitController::applyMode);
        c.requestMode(1);
        c.setContext("server/awg", false);
        c.requestMode(1);
        c.setContext("server/awg", true);
        c.requestMode(-1);
        c.requestMode(3);
        c.requestMode(0);
        c.observeConnection(QuickSplitController::Transition);
        c.requestMode(1);
        QCOMPARE(apply.count(), 0);
    }
};

QTEST_GUILESS_MAIN(TestQuickSplit)
#include "testQuickSplit.moc"
