#ifndef TAGIDIALOG_H
#define TAGIDIALOG_H

#include <QDialog>
#include "appcontroller.h"

QT_BEGIN_NAMESPACE
namespace Ui { class TagiDialog; }
QT_END_NAMESPACE

// Dialog zarządzania tagami. Tagi to etykiety wiele-do-wielu — jeden wpis może
// mieć wiele tagów, jeden tag może być przypisany do wielu wpisów.
// Usunięcie tagu automatycznie kasuje powiązania w tabeli multimedia_tagi
// (ON DELETE CASCADE po stronie bazy danych).
class TagiDialog : public QDialog {
    Q_OBJECT

public:
    explicit TagiDialog(AppController& controller, QWidget *parent = nullptr);
    ~TagiDialog();

private slots:
    void onBtnDodajClicked();
    void onBtnEdytujClicked();
    void onBtnUsunClicked();

private:
    Ui::TagiDialog *ui;
    AppController& appController;

    void wypelnijTabele();
};

#endif
