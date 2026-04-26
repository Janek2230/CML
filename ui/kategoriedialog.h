#ifndef KATEGORIEDIALOG_H
#define KATEGORIEDIALOG_H

#include <QDialog>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "databasemanager.h"

class KategorieDialog : public QDialog {
    Q_OBJECT

public:
    explicit KategorieDialog(DatabaseManager& db, QWidget *parent = nullptr);
    ~KategorieDialog();

private slots:
    void onBtnDodajClicked();
    void onBtnEdytujClicked();
    void onBtnUsunClicked();

private:
    DatabaseManager& dbManager;

    QTableWidget *tabela;
    QPushButton *btnDodaj;
    QPushButton *btnEdytuj;
    QPushButton *btnUsun;
    QPushButton *btnZamknij;

    void wypelnijTabele();
};

#endif