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
 * Zamiast sortować po dacie dodania, lepiej to zrobić po dacie ostatniej aktywności
 * Dodać historię podejść do danego medium
 * Do rozważenia jakieś okładki w tło, choć z tym może być za dużo niepotrzbnego zachodu.
 * Aktualnie wyświetla się data dodania i ostatniej edycji, zastąpimy ją datą ostatniej aktywnosći, dodatkowo można jeszcze dodać licznik dni które mineły od daty dodania w przypadku gdy jeszcze nie rozpączęto akcji (czyli medium ma status "planowane") z tą grą oraz dni od rozpąćzecia aktywnośći w przypadku gdy zmieniono status na "w trakcie"

1. Kupkę wstydu czyli rzeczy które mają datę dodania bardzo dawną i dalej status planowany lub rzeczy które są w trakcie i od dawana nie były w dzienniku aktywności. (wybór pewnie też combooxem aby zdecydować które chemy wyświetlić) + możliwość szybkiej zmiany statusu na porzucony w przypadku aktywności w trakcie.

2. Wyświetlić zestawinie ulubionych, np. najwięcej czasu spędzonego czy najlepsza ocena itd.

3. Statystyki ogólne, czyli łączny czas spędzyony, czas na poszczególne kategorie, ilość jedostek poszczególnych kategorii jakie wprowadziliśm itd takie ogólne podsumowanie.
Tu można użyć tych pięciokątnych wykresów, wybrać te kateogire itd. najwyżej punktowane i je przedstawić na takich wkresach.

4. Trzeba popracować na wyglądem wykresu: 1. Trzeba jakoś grupować albo nie wyświetlać wartości nieistotnych 2. Dobrze było by zrezygnować z jasnych koloróW, wtapiają się w tło, albo zmienic tło wykresu na ciemne, albo dodać obramowania do słupków.




*/
