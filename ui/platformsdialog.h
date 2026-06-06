#ifndef PLATFORMSDIALOG_H
#define PLATFORMSDIALOG_H

#include <QDialog>
#include "appcontroller.h"

QT_BEGIN_NAMESPACE
namespace Ui { class PlatformsDialog; }
QT_END_NAMESPACE

// Dialog zarządzania platformami dystrybucji (np. Netflix, Steam, Fizyczna).
// Platformy służą wyłącznie jako etykieta — usunięcie platformy nie kasuje
// multimedia, tylko zeruje pole id_platformy (lub kasuje razem, zależnie od wyboru).
class PlatformsDialog : public QDialog {
    Q_OBJECT

public:
    explicit PlatformsDialog(AppController& controller, QWidget *parent = nullptr);
    ~PlatformsDialog();

private slots:
    void obsluzDodaj();
    void obsluzEdytuj();
    void obsluzUsun();

private:
    Ui::PlatformsDialog *ui;
    AppController& appController;

    void wypelnijTabele();
};

#endif
