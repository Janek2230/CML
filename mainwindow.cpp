#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    if (dbManager.openConnection()) {
        listaKsiazek = dbManager.getBooks();
        ui->tableWidget->setRowCount(listaKsiazek.size());
        ui->tableWidget->setColumnCount(2); // Tytuł i Postęp

        for(int i = 0; i < listaKsiazek.size(); ++i) {
            ui->tableWidget->setItem(i, 0, new QTableWidgetItem(listaKsiazek[i]->getTytul()));
            ui->tableWidget->setItem(i, 1, new QTableWidgetItem(QString::number(listaKsiazek[i]->getProcentPostepu()) + "%"));
        }
    }

}

MainWindow::~MainWindow()
{
    qDeleteAll(listaKsiazek);
    listaKsiazek.clear();

    delete ui;
}
