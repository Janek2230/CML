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

1. Kupkę wstydu czyli rzeczy które mają datę dodania bardzo dawną i dalej status planowany lub rzeczy które są w trakcie i od dawana nie były w dzienniku aktywności. (wybór pewnie też combooxem aby zdecydować które chemy wyświetlić) + możliwość szybkiej zmiany statusu na porzucony w przypadku aktywności w trakcie.

2. Wyświetlić zestawinie ulubionych, np. najwięcej czasu spędzonego czy najlepsza ocena itd.

3. Statystyki ogólne, czyli łączny czas spędzyony, czas na poszczególne kategorie, ilość jedostek poszczególnych kategorii jakie wprowadziliśm itd takie ogólne podsumowanie.
Tu można użyć tych pięciokątnych wykresów, wybrać te kateogire itd. najwyżej punktowane i je przedstawić na takich wkresach.

4. Trzeba popracować na wyglądem wykresu: 1. Trzeba jakoś grupować albo nie wyświetlać wartości nieistotnych 2. Dobrze było by zrezygnować z jasnych koloróW, wtapiają się w tło, albo zmienic tło wykresu na ciemne, albo dodać obramowania do słupków.

5. Zapisz zmiany cały czas wyświeyla komunikat o tym żeby automatycznie zakończyć nawet po tym jak już jest zakończone

6. Wprowadzić automatyczną zmianę na porzucone jeśli ktoś nie ustawi wstrzymane i ma w trakcie jakiś długi czas

7. Dodać obłsugę statusu wstyrzymanego

8. Wykres statystyk nie działa (znowu)

9. Dodać wyświetlanie łącznie spędzone czasu w jakimś medium

10. Rozważyć możliwosć dodania dodawania nowych jednostek


*/
