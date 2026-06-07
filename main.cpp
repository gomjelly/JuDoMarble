#include <QApplication>
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("JuDoMarble");
    app.setApplicationVersion("0.1");

    MainWindow w;
    w.show();

    return app.exec();
}
