#ifndef KATEGORIEDIALOG_H
#define KATEGORIEDIALOG_H

#include <QDialog>
#include "appcontroller.h"

QT_BEGIN_NAMESPACE
namespace Ui { class KategorieDialog; }
QT_END_NAMESPACE

// Dialog zarządzania kategoriami — pozwala dodawać, edytować i usuwać kategorie.
// Każda kategoria ma nazwę i jednostkę (np. "strony", "odcinki"), która
// jest dziedziczona przez multimedia przypisane do tej kategorii.
class KategorieDialog : public QDialog {
    Q_OBJECT

public:
    explicit KategorieDialog(AppController& controller, QWidget *parent = nullptr);
    ~KategorieDialog();

private slots:
    void onBtnDodajClicked();
    void onBtnEdytujClicked();
    void onBtnUsunClicked();

private:
    Ui::KategorieDialog *ui;
    AppController& appController;

    // Czyści tabelę i wypełnia ją aktualnymi danymi z bazy.
    void wypelnijTabele();
};

#endif
