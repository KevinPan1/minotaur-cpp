#include <QApplication>
#include <opencv2/core/mat.hpp>

#include "code/interpreter/pythonengine.h"
#include "gui/mainwindow.h"

int main(int argc, char *argv[]) {
    qRegisterMetaType<cv::Mat>();
    QApplication app(argc, argv);

    auto *w = new MainWindow();
    w->show();

    return app.exec();
}
