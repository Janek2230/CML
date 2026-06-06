#ifndef TIMELINEVIEW_H
#define TIMELINEVIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include "appcontroller.h"

namespace Ui { class TimelineView; }

class TimelineView : public QWidget
{
    Q_OBJECT

public:
    explicit TimelineView(AppController& controller, QWidget *parent = nullptr);
    ~TimelineView();

    void renderujOsCzasu();

private:
    Ui::TimelineView *ui;
    AppController& appController;
    QVBoxLayout* glownyUklad;

    void wyczyscLayout(QLayout* layout);
    void pokazDetaleRecenzji(const HistoricalAttempt& p);
};

#endif // TIMELINEVIEW_H