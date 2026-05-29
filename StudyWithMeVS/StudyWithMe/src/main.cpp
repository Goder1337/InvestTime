#include "MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName("StudyWithMe");
    QApplication::setApplicationName("StudyWithMe");

    MainWindow window;
    window.show();

    return app.exec();
}
