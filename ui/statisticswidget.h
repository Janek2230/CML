#ifndef STATISTICSWIDGET_H
#define STATISTICSWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QEvent>
#include <QMouseEvent>
#include <QHBoxLayout>
#include <QLayout>
#include <QVariantMap>
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
    void odswiezPodsumowanieOgolne();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void zadaniePokazaniaSzczegolow(int idMedium);

private:
    QWidget* zbudujKafelek(const std::shared_ptr<Multimedia>& medium, const QMap<int, QString>& mapaPlatform);
    int policzDniBezczynnosci(const std::shared_ptr<Multimedia>& medium) const;
    QDateTime wyznaczDateReferencyjna(const std::shared_ptr<Multimedia>& medium) const;
    void wyczyscLayout(QLayout* layout);
    void odswiezKupkeWstydu();
    QString sformatujCzas(long long sekundy) const;

    Ui::StatisticsWidget *ui;
    AppController& appController;
    QLabel *etykietaTooltip;
};

#endif
