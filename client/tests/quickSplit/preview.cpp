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
    view.setInitialProperties({{"selectedMode", 1}, {"connected", true}});
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
