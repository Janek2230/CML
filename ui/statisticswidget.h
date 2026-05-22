#ifndef STATISTICSWIDGET_H
#define STATISTICSWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QEvent>
#include <QLayout>
#include "appcontroller.h"

namespace Ui {
class StatisticsWidget;
}

class StatisticsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StatisticsWidget(AppController& controller, QWidget *parent = nullptr);
    ~StatisticsWidget();

    void odswiezWykresAktywnosci();
    void odswiezDane();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void zadaniePokazaniaSzczegolow(int idMedium);

private:
    void odswiezPodsumowanieOgolne();
    void odswiezPorzucone();
    void odswiezUlubione();
    void odswiezKupkeWstydu();

    QWidget* zbudujKafelek(const std::shared_ptr<Multimedia>& medium, const QMap<int, QString>& mapaPlatform, bool pokazWznow = true, bool pokazPorzuc = true);
    int policzDniBezczynnosci(const std::shared_ptr<Multimedia>& medium) const;
    QDateTime wyznaczDateReferencyjna(const std::shared_ptr<Multimedia>& medium) const;
    void wyczyscLayout(QLayout* layout);
    QString sformatujCzas(long long sekundy) const;

    Ui::StatisticsWidget *ui;
    AppController& appController;
    QLabel *etykietaTooltip;
};

#endif
