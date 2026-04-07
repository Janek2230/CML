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
 * Dodać historię podejść do danego medium
 * Do rozważenia jakieś okładki w tło, choć z tym może być za dużo niepotrzbnego zachodu.
 * Aktualnie wyświetla się data dodania i ostatniej edycji, zastąpimy ją datą ostatniej aktywnosći, dodatkowo można jeszcze dodać licznik dni które mineły od daty dodania w przypadku gdy jeszcze nie rozpączęto akcji (czyli medium ma status "planowane") z tą grą oraz dni od rozpąćzecia aktywnośći w przypadku gdy zmieniono status na "w trakcie"
 */