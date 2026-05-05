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

Zakładka z notatkami i recenzjami

Zakładka aktywność:
4. Trzeba popracować na wyglądem wykresu: 1. Trzeba jakoś grupować albo nie wyświetlać wartości nieistotnych 2. Dobrze było by zrezygnować z jasnych koloróW, wtapiają się w tło, albo zmienic tło wykresu na ciemne, albo dodać obramowania do słupków.
5. Dodać haet mapę jak w github

Zakłdka porzucone:
11. Zakładkę z pozycjami porzuconymi. Pozycje które nie mają ani jednego podejścia oznaczonego jako ukończone

13. Grupowanie po tagach w głownym widoku.

14. W momencie odawania nowej sesji dać możliwośc odpalenia stopera który zliczy jej czas trwania i dać potem możliosć ręcznej modyfikacji

15. Usunąć przycisk doadj podejście - rozpoczęcie nowego jest możliwe tylko poprzez przycisk zacznij od nowa


*/
