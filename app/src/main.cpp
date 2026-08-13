#include "MainWindow.h"

#include <QApplication>
#include <QStyleFactory>
#include <QFile>
#include <QPalette>
#include <QIcon>

int main(int argc, char** argv)
{
    QApplication::setOrganizationName("salom600");
    QApplication::setApplicationName("Beta");
    QApplication::setApplicationVersion("0.2.0");

    // Use Fusion style — it's the most consistent cross-platform base
    // for our custom dark theme.
    if (QStyleFactory::keys().contains("Fusion")) {
        QApplication::setStyle(QStyleFactory::create("Fusion"));
    }

    QApplication app(argc, argv);

    // Apply palette + QSS early so first paint is already themed.
    QPalette p = app.palette();
    p.setColor(QPalette::Window,          QColor( 30,  31,  34));
    p.setColor(QPalette::WindowText,      QColor(230, 231, 236));
    p.setColor(QPalette::Base,            QColor( 30,  31,  34));
    p.setColor(QPalette::AlternateBase,   QColor( 37,  38,  42));
    p.setColor(QPalette::Text,            QColor(230, 231, 236));
    p.setColor(QPalette::Button,          QColor( 45,  46,  51));
    p.setColor(QPalette::ButtonText,      QColor(230, 231, 236));
    p.setColor(QPalette::BrightText,      Qt::white);
    p.setColor(QPalette::Highlight,       QColor( 14,  99, 212));
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::ToolTipBase,     QColor( 45,  46,  51));
    p.setColor(QPalette::ToolTipText,     QColor(240, 240, 242));
    app.setPalette(p);

    QFile qss(":/dark.qss");
    if (qss.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(QString::fromUtf8(qss.readAll()));
    }

    beta::MainWindow w;
    w.show();

    return app.exec();
}
