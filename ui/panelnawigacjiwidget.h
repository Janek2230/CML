#ifndef PANELNAWIGACJIWIDGET_H
#define PANELNAWIGACJIWIDGET_H

#include <QWidget>
#include <QTreeWidgetItem>
#include <QMenu>
#include "databasemanager.h"
#include "multimedia.h"

namespace Ui { class PanelNawigacjiWidget; }

class PanelNawigacjiWidget : public QWidget {
    Q_OBJECT
public:
    explicit PanelNawigacjiWidget(DatabaseManager& db, QWidget *parent = nullptr);
    ~PanelNawigacjiWidget();

    // Metoda pozwalająca wymusić odświeżenie drzewa (np. po tym, jak Formularz po prawej zapisze dane)
    void odswiezDrzewo();

signals:
    // Sygnały dla MainWindow, żeby wiedziało, co przełączyć po prawej stronie
    void zadaniePokazaniaSzczegolow(int idMedium);
    void zadanieDodaniaMedium(int idKategorii, int idPlatformy);
    void zadanieEdycjiMedium(int idMedium);
    void zadaniePowrotuDoDashboardu();
    void drzewoZmieniloBaze(); // Krzyczy, gdy usuniesz z menu platformę/kategorię, by Dashboard wiedział o zmianach

private slots:
    void onWybieranieElementuDrzewa(QTreeWidgetItem *item, int column);
    void onWyszukiwanie(const QString &text);
    void pokazMenuDrzewa(const QPoint &pos);

private:
    Ui::PanelNawigacjiWidget *ui;
    DatabaseManager& dbManager;
    QList<std::shared_ptr<Multimedia>> listaMultimediow;

    void zaladujDaneDoDrzewa();
};
#endif