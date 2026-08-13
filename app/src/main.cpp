#include "MainWindow.h"

#include <QApplication>
#include <QStyleFactory>

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName("salom600");
    QApplication::setApplicationName("Beta");
    QApplication::setApplicationVersion("0.1.0");

    if (QStyleFactory::keys().contains("Fusion")) {
        QApplication::setStyle(QStyleFactory::create("Fusion"));
    }

    beta::MainWindow w;
    w.show();

    return app.exec();
}
