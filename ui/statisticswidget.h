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
    void odswiezNigdyNieukonczone();
    void odswiezUlubione();
    void odswiezKupkeWstydu();

    QWidget* zbudujKafelek(const std::shared_ptr<Multimedia>& medium, const QMap<int, QString>& mapaPlatform, bool pokazWznow = true, bool pokazPorzuc = true, const QString& dodatkowaLinia = QString());
    int policzDniBezczynnosci(const std::shared_ptr<Multimedia>& medium) const;
    QDateTime wyznaczDateReferencyjna(const std::shared_ptr<Multimedia>& medium) const;
    void wyczyscLayout(QLayout* layout);
    QString sformatujCzas(long long sekundy) const;

    // Mapa idPlatformy -> nazwa; budowana w kilku zakładkach, więc wydzielona w jedno miejsce.
    QMap<int, QString> mapaPlatform() const;
    // Łączny czas (w sekundach) zsumowany po wszystkich sesjach danego medium.
    QMap<int, long long> sumaCzasuNaMedium(const QList<std::shared_ptr<Multimedia>>& media) const;

    Ui::StatisticsWidget *ui;
    AppController& appController;
    QLabel *etykietaDymka;
};

#endif
