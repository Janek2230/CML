#ifndef TAGSDIALOG_H
#define TAGSDIALOG_H

#include <QDialog>
#include "appcontroller.h"

QT_BEGIN_NAMESPACE
namespace Ui { class TagsDialog; }
QT_END_NAMESPACE

// Dialog zarządzania tagami. Tagi to etykiety wiele-do-wielu — jeden wpis może
// mieć wiele tagów, jeden tag może być przypisany do wielu wpisów.
// Usunięcie tagu automatycznie kasuje powiązania w tabeli multimedia_tagi
// (ON DELETE CASCADE po stronie bazy danych).
class TagsDialog : public QDialog {
    Q_OBJECT

public:
    explicit TagsDialog(AppController& controller, QWidget *parent = nullptr);
    ~TagsDialog();

private slots:
    void obsluzDodaj();
    void obsluzEdytuj();
    void obsluzUsun();

private:
    Ui::TagsDialog *ui;
    AppController& appController;

    void wypelnijTabele();
};

#endif
