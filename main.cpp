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
 * 1. Przy edycji czy dodawaniu można użyć tego samego panelu, ale z ukrytymi niepotrzebnymi elementami
 * 2. Przycisk anuluj powinien działać
 * 3. Można by dodać w widoku dodatkowe kategorie, oprócz np. gry można by skategoryzować na podstawie statusu tzn, ukończony/planowany itd.
 * 4.
 */