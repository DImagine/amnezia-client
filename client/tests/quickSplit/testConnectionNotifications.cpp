#include <QtTest>
#include "../../ui/utils/connectionNotificationGate.h"
#include "../../ui/controllers/quickSplitController.h"

class TestConnectionNotifications : public QObject
{
    Q_OBJECT
private slots:
    void ordinaryEventsAreUnchanged()
    {
        ConnectionNotificationGate gate(nullptr, 30);
        QSignalSpy final(&gate, &ConnectionNotificationGate::settledConnected);
        QVERIFY(gate.shouldNotify(true));
        QVERIFY(gate.shouldNotify(false));
        QVERIFY(gate.shouldNotify(true));
        QTest::qWait(60);
        QCOMPARE(final.count(), 0);
    }
    void rapidSeriesShowsOnlyLastSuccess()
    {
        ConnectionNotificationGate gate(nullptr, 50);
        QSignalSpy final(&gate, &ConnectionNotificationGate::settledConnected);
        for (int i = 0; i < 4; ++i) {
            gate.begin();
            QVERIFY(!gate.shouldNotify(false));
            gate.end(true); // The controller receives Connected before the tray handler.
            QVERIFY(!gate.shouldNotify(true));
            QTest::qWait(10);
            QCOMPARE(final.count(), 0);
        }
        QTRY_COMPARE(final.count(), 1);
        QTest::qWait(80);
        QCOMPARE(final.count(), 1);
        QVERIFY(gate.shouldNotify(false));
    }
    void ordinaryDisconnectCancelsPendingSuccess()
    {
        ConnectionNotificationGate gate(nullptr, 30);
        QSignalSpy final(&gate, &ConnectionNotificationGate::settledConnected);
        gate.begin();
        gate.end(true);
        QVERIFY(!gate.shouldNotify(true));
        QVERIFY(gate.shouldNotify(false)); // Still show the ordinary disconnect immediately.
        QTest::qWait(60);
        QCOMPARE(final.count(), 0);
    }
    void failureDoesNotLeakSuccessOrLeaveSuppressionEnabled()
    {
        ConnectionNotificationGate gate(nullptr, 30);
        QSignalSpy final(&gate, &ConnectionNotificationGate::settledConnected);
        gate.begin();
        gate.end(true);
        gate.shouldNotify(true);
        gate.begin();
        gate.shouldNotify(false);
        gate.end(false);
        QVERIFY(!gate.shouldNotify(false));
        QTest::qWait(60);
        QCOMPARE(final.count(), 0);
        QVERIFY(gate.shouldNotify(true));
    }
    void oldFailureCannotUnmuteNewOperation()
    {
        ConnectionNotificationGate gate(nullptr, 30);
        gate.begin();
        gate.end(false);
        gate.begin();
        QCoreApplication::processEvents();
        QVERIFY(!gate.shouldNotify(true));
    }
    void coordinatorSuppressesTerminalEventRegardlessOfObserverOrder()
    {
        QuickSplitController controller;
        ConnectionNotificationGate gate(nullptr, 30);
        controller.setContext("server/awg", true);
        controller.observeConnection(QuickSplitController::Connected);
        gate.shouldNotify(true);
        connect(&controller, &QuickSplitController::automaticSwitchStarted, &gate, &ConnectionNotificationGate::begin);
        connect(&controller, &QuickSplitController::automaticSwitchFinished, &gate, &ConnectionNotificationGate::end);
        connect(&controller, &QuickSplitController::reconnectRequested, &controller, [&]() {
            controller.observeConnection(QuickSplitController::Transition);
            QVERIFY(!gate.shouldNotify(false));
        });
        QSignalSpy final(&gate, &ConnectionNotificationGate::settledConnected);
        controller.requestMode(1);
        controller.observeConnection(QuickSplitController::Disconnected);
        QVERIFY(!gate.shouldNotify(false));
        QTRY_COMPARE(controller.phase(), int(QuickSplitController::Reconnecting));
        controller.observeConnection(QuickSplitController::Connected);
        QVERIFY(!controller.busy());
        QVERIFY(!gate.shouldNotify(true));
        QTRY_COMPARE(final.count(), 1);
    }
};

QTEST_GUILESS_MAIN(TestConnectionNotifications)
#include "testConnectionNotifications.moc"
