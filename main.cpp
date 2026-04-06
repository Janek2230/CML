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
 * 1. Dodać danę dodania/aktualizacji w rogu panelu drugiego
 * 2. Dodać wyświetlanie oceny w paenlu drugim i jej wprowadzanie gdzieś w panelu edycyjnym (do rozważenia blokada tylko po ukończeniu, choć to trochę bez sensu)
 * 3. Dodać wyświetlanie liczby ukończeń
 * 4. Do rozważenia jakieś okładki w tło, choć z tym może być za dużo zachodu.
 * 5. Wykres odświża się przy chowaniu zakładek, do poprawy
 * 6. Przy wprowadzaniu nowego medium nie jest wymagane podanie kategorii czy nazwy, po prostu wtedy trafia do bez kategorii, trzeba dodać by po dodaniu zachowywał się status kat/plat wybranych wcześniej przez gracz (jest to chyba bardziej praktyczne gdy np. ktoś dodaje hurtowo)
 * 7. Zacznij od nowa nie powinno przeniosić na stronę głowną, oraz powinno być okiengo z ostrzeżeniem.
 * 8. Aktualnie zostaje combobox z statusem, (jest to zabezpieczenie gdyby gracz przypadkiem zmienił sobie na ukończone mimio że nie ukończył) można dodać dwu stopniowe zabezpieczenie: 1. Nie da się zmieniać ręcznie na ukończony dzieje się to tylko gdy gracz zrówna aktualny stan jednostki z docelowym, 2. W systuacji gdyby przez przypadek uzuepełnił wystarczy dodać okionko z powiadomieniem że po zapisaniu gra zmieni stan na ukończone i nie będzie można tego zmienić (bo i po co)
 * 9.
 */