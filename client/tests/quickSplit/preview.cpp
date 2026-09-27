#include <QGuiApplication>
#include <QQuickView>
#include <QQmlEngine>
#include <QTimer>
#include <QTranslator>
#include <QImage>
#include <QQuickStyle>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    QQuickStyle::setStyle("Basic"); // Match AmneziaApplication's control style.
    QTranslator translator;
    if (argc > 2 && translator.load(QString::fromLocal8Bit(argv[2])))
        app.installTranslator(&translator);
    QQuickView view;
    view.engine()->addImportPath(QStringLiteral(QML_DIR "/Modules"));
    view.setColor(QColor("#0E0E11"));
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    // Optional state argument captures each visual state without connecting to a VPN.
    const QString state = argc > 3 ? QString::fromLocal8Bit(argv[3]) : QStringLiteral("connected");
    const bool switching = state == QStringLiteral("switching");
    view.setInitialProperties({{"selectedMode", 0}, {"connected", state == QStringLiteral("connected")},
                               {"phase", switching ? 3 : 0}, {"pendingMode", switching ? 2 : -1}});
    view.setSource(QUrl::fromLocalFile(QStringLiteral(QML_DIR "/Components/QuickSplitTunneling.qml")));
    if (view.status() == QQuickView::Error)
        return 1;
    view.resize(328, 60);
    view.show();
    // Render the production component without accessing the running VPN client.
    QTimer::singleShot(500, &app, [&]() {
        const bool saved = view.grabWindow().save(argc > 1 ? QString::fromLocal8Bit(argv[1]) : "quick-split.png");
        app.exit(saved ? 0 : 2);
    });
    return app.exec();
}
