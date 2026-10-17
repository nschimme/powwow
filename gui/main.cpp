#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("powwow-gui");
    app.setOrganizationName("powwow");

    MainWindow w;
    w.show();

    return app.exec();
}
