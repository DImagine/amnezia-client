#include <QtTest>
#include <QQuickView>
#include <QQuickItem>
#include <QQmlEngine>
#include <QTranslator>
#include <QQuickStyle>

class TestQuickSplitUi : public QObject
{
    Q_OBJECT
    QTranslator m_translator;
private:
    QQuickItem *findItem(QQuickItem *parent, const QString &name)
    {
        // Repeater delegates belong to the visual tree, not necessarily the QObject tree.
        if (parent->objectName() == name)
            return parent;
        for (auto *child : parent->childItems()) {
            if (auto *found = findItem(child, name))
                return found;
        }
        return nullptr;
    }
    void load(QQuickView &view, int width)
    {
        view.engine()->addImportPath(QStringLiteral(QML_DIR "/Modules"));
        view.setResizeMode(QQuickView::SizeRootObjectToView);
        view.setSource(QUrl::fromLocalFile(QStringLiteral(QML_DIR "/Components/QuickSplitTunneling.qml")));
        QCOMPARE(view.status(), QQuickView::Ready);
        view.resize(width, 60);
        view.show();
        QVERIFY(QTest::qWaitForWindowExposed(&view));
    }
private slots:
    void initTestCase()
    {
        QQuickStyle::setStyle("Basic");
        // Use the shipped Russian strings, without reading any VPN configuration.
        QVERIFY(m_translator.load(qEnvironmentVariable("QUICK_SPLIT_TRANSLATION")));
        qApp->installTranslator(&m_translator);
    }
    void layout_data()
    {
        QTest::addColumn<int>("width");
        QTest::newRow("360px window") << 328;
        QTest::newRow("450px window") << 418;
    }
    void layout()
    {
        QFETCH(int, width);
        QQuickView view;
        load(view, width);
        auto *root = view.rootObject();
        QVERIFY(root);
        qreal previousRight = 0;
        for (int i = 0; i < 3; ++i) {
            auto *button = findItem(root, "quickSplitMode" + QString::number(i));
            QVERIFY(button);
            const auto rect = button->mapRectToItem(root, button->boundingRect());
            QVERIFY(rect.left() >= previousRight);
            QVERIFY(rect.right() <= width);
            QVERIFY(rect.bottom() <= view.height());
            QVERIFY(button->height() >= 32);
            QVERIFY(button->height() <= 36);
            previousRight = rect.right();
            auto *label = button->property("contentItem").value<QQuickItem *>();
            QVERIFY(label);
            QVERIFY(!label->property("truncated").toBool());
            QVERIFY(label->property("contentWidth").toReal() <= label->width() + 1);
            QVERIFY(label->property("contentHeight").toReal() <= label->height() + 1);
        }
    }
    void clicksAndBusyGuard()
    {
        QQuickView view;
        load(view, 328);
        auto *root = view.rootObject();
        QSignalSpy modes(root, SIGNAL(modeRequested(int)));
        auto click = [&](const QString &name) {
            auto *button = findItem(root, name);
            QVERIFY(button);
            QTest::mouseClick(&view, Qt::LeftButton, Qt::NoModifier,
                             button->mapToScene(button->boundingRect().center()).toPoint());
        };
        click("quickSplitMode1");
        QCOMPARE(modes.count(), 1);
        QCOMPARE(modes.first().first().toInt(), 1);
        root->setProperty("phase", 1);
        click("quickSplitMode2");
        QCOMPARE(modes.count(), 1);
        root->setProperty("phase", 0);
        root->setProperty("available", false);
        click("quickSplitMode2");
        QCOMPARE(modes.count(), 1);
    }
};

QTEST_MAIN(TestQuickSplitUi)
#include "testQuickSplitUi.moc"
