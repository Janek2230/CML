#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qtreewidget.h>
#include "databasemanager.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    Ui::MainWindow *ui;
    DatabaseManager dbManager;
    QList<std::shared_ptr<Multimedia>> listaMultimediow;

    void zaladujDaneDoDrzewa();
    void odswiezStatystykiGlowne();

    bool czyTrybEdycji = false;
    int idEdytowanegoMedium = -1;
    void przygotujFormularz(int idMedium = -1, int idDomyslnejKategorii = 0, int idDomyslnejPlatformy = 0);
    void usunWybraneMedium(int id);
    void pokazSzczegolyMedium(int idMedium);

    void uzupelnijComboBoxy();

private slots:
    // Ten slot odpali się, gdy klikniesz coś w drzewie
    void onWybieranieElementuDrzewa(QTreeWidgetItem *item, int column);

    // Ten slot przyda Ci się za chwilę do paska wyszukiwania
    void onWyszukiwanie(const QString &text);

    void pokazMenuDrzewa(const QPoint &pos);


};
#endif // MAINWINDOW_H
