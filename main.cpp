#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return QCoreApplication::exec();
}

// dodaj zabezpieczenia: (szczegolywidget) to co możliwe wykonaj w .ui
//                        1. Jak nie ma żadnych danych w ostatniej aktywności to napis ostatnia ktywonośc i wszystko co z nim ziwązane nie powinien się wyświetlać.
//                        2. Zamiast wykresu powinna pojawić się informacja, że nie ma nic w bilbiotece i zachęcić do dodania, można dodać jakiś duży przycisk na środku.