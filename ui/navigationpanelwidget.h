#ifndef NAVIGATIONPANELWIDGET_H
#define NAVIGATIONPANELWIDGET_H

#include <QWidget>
#include <QTreeWidgetItem>
#include <QMenu>
#include "appcontroller.h"
#include "multimedia.h"

namespace Ui { class NavigationPanelWidget; }

class NavigationPanelWidget : public QWidget {
    Q_OBJECT
public:
    explicit NavigationPanelWidget(AppController& controller, QWidget *parent = nullptr);
    ~NavigationPanelWidget();

    void odswiezDrzewo();

signals:
    void zadaniePokazaniaSzczegolow(int idMedium);
    void zadanieDodaniaMedium(int idKategorii, int idPlatformy);
    void zadanieEdycjiMedium(int idMedium);
    void zadaniePowrotuDoDashboardu();

private slots:
    void obsluzWyborElementuDrzewa(QTreeWidgetItem *item, int kolumna);
    void obsluzWyszukiwanie(const QString &tekst);
    void pokazMenuDrzewa(const QPoint &pos);

private:
    Ui::NavigationPanelWidget *ui;
    AppController& appController;

    void zaladujDaneDoDrzewa();
};
#endif
