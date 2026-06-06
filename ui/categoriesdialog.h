#ifndef CATEGORIESDIALOG_H
#define CATEGORIESDIALOG_H

#include <QDialog>
#include "appcontroller.h"

QT_BEGIN_NAMESPACE
namespace Ui { class CategoriesDialog; }
QT_END_NAMESPACE

// Dialog zarządzania kategoriami — pozwala dodawać, edytować i usuwać kategorie.
// Każda kategoria ma nazwę i jednostkę (np. "strony", "odcinki"), która
// jest dziedziczona przez multimedia przypisane do tej kategorii.
class CategoriesDialog : public QDialog {
    Q_OBJECT

public:
    explicit CategoriesDialog(AppController& controller, QWidget *parent = nullptr);
    ~CategoriesDialog();

private slots:
    void obsluzDodaj();
    void obsluzEdytuj();
    void obsluzUsun();

private:
    Ui::CategoriesDialog *ui;
    AppController& appController;

    // Czyści tabelę i wypełnia ją aktualnymi danymi z bazy.
    void wypelnijTabele();
};

#endif
