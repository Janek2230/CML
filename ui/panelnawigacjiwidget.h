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

    void odswiezDrzewo();

signals:
    void zadaniePokazaniaSzczegolow(int idMedium);
    void zadanieDodaniaMedium(int idKategorii, int idPlatformy);
    void zadanieEdycjiMedium(int idMedium);
    void zadaniePowrotuDoDashboardu();
    void drzewoZmieniloBaze();

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