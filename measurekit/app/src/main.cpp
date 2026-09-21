#include <QApplication>
#include <QGuiApplication>

#include "mainwindow.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("MeasureKit"));
    QCoreApplication::setApplicationName(QStringLiteral("measurekit"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("度量衡 MeasureKit"));
    MainWindow w;
    w.show();
    return app.exec();
}
