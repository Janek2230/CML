#ifndef STATISTICSWIDGET_H
#define STATISTICSWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QEvent>
#include <QMouseEvent>
#include "databasemanager.h"

namespace Ui {
class StatisticsWidget;
}

class StatisticsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StatisticsWidget(DatabaseManager& db, QWidget *parent = nullptr);
    ~StatisticsWidget();

    // To wywoła MainWindow, gdy użytkownik wejdzie w zakładkę
    void odswiezWykresAktywnosci();
    void odswiezDane();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    Ui::StatisticsWidget *ui;
    DatabaseManager& dbManager; // Wstrzyknięta zależność!
    QLabel *etykietaTooltip;

private slots:
    void onComboWidokChanged(int index);
};



#endif // STATISTICSWIDGET_H