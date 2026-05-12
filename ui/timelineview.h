#ifndef TIMELINEVIEW_H
#define TIMELINEVIEW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QPushButton>
#include <QDialog>
#include <QTextBrowser> // DODANE: Wymagane do renderowania recenzji
#include "appcontroller.h"

class TimelineView : public QWidget
{
    Q_OBJECT

public:
    explicit TimelineView(AppController& controller, QWidget *parent = nullptr);
    void renderujTimeline();

private:
    AppController& appController;
    QVBoxLayout* mainLayout;

    void wyczyscLayout(QLayout* layout);
    void pokazDetaleRecenzji(const PodejscieHistoryczne& p);
};

#endif // TIMELINEVIEW_H
