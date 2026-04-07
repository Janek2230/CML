#ifndef KATEGORIEDIALOG_H
#define KATEGORIEDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "databasemanager.h" // Dialog musi mieć dostęp do bazy!

class KategorieDialog : public QDialog {
    Q_OBJECT

public:
    // Przekazujemy referencję do istniejącego dbManagera z MainWindow
    explicit KategorieDialog(DatabaseManager& db, QWidget *parent = nullptr);
    ~KategorieDialog();

private slots:
    void onBtnDodajClicked();
    void onBtnEdytujClicked();
    void onBtnUsunClicked();

private:
    DatabaseManager& dbManager;

    // Wskaźniki na elementy UI
    QTableWidget *tabela;
    QPushButton *btnDodaj;
    QPushButton *btnEdytuj;
    QPushButton *btnUsun;
    QPushButton *btnZamknij;

    void wypelnijTabele();
};

#endif // KATEGORIEDIALOG_H