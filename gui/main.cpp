#include <QApplication>
#include <QFontDatabase>
#include <QIcon>
#include <QDebug>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("powtty");
    app.setOrganizationName("mume");

    // Load app window icon
    app.setWindowIcon(QIcon(":/resources/icons/icon.png"));

    // Load embedded PowTTY Mono font
    int fontId = QFontDatabase::addApplicationFont(":/resources/fonts/powttymono.ttf");
    if (fontId != -1) {
        QStringList fontFamilies = QFontDatabase::applicationFontFamilies(fontId);
        if (!fontFamilies.isEmpty()) {
            qDebug() << "Loaded font family:" << fontFamilies.first();
        }
    } else {
        qWarning() << "Failed to load embedded powttymono.ttf font resource.";
    }

    MainWindow w;
    w.show();

    return app.exec();
}
