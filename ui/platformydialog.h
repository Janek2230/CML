#ifndef PLATFORMYDIALOG_H
#define PLATFORMYDIALOG_H

#include <QDialog>
#include "appcontroller.h"

QT_BEGIN_NAMESPACE
namespace Ui { class PlatformyDialog; }
QT_END_NAMESPACE

// Dialog zarządzania platformami dystrybucji (np. Netflix, Steam, Fizyczna).
// Platformy służą wyłącznie jako etykieta — usunięcie platformy nie kasuje
// multimedia, tylko zeruje pole id_platformy (lub kasuje razem, zależnie od wyboru).
class PlatformyDialog : public QDialog {
    Q_OBJECT

public:
    explicit PlatformyDialog(AppController& controller, QWidget *parent = nullptr);
    ~PlatformyDialog();

private slots:
    void onBtnDodajClicked();
    void onBtnEdytujClicked();
    void onBtnUsunClicked();

private:
    Ui::PlatformyDialog *ui;
    AppController& appController;

    void wypelnijTabele();
};

#endif
