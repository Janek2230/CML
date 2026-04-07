#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return QCoreApplication::exec();
}

/*
 * QSS - to może pomóc w tyam aby jakoś to wyglądało, do zapoznania się.
 * Do rozważenia jakieś okładki w tło, choć z tym może być za dużo niepotrzbnego zachodu.
 */