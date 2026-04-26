DashboardWidget::DashboardWidget(AppController& controller, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DashboardWidget),
    appController(controller)
{
    ui->setupUi(this);
    connect(ui->btnLosuj, &QPushButton::clicked, this, &DashboardWidget::onBtnLosujClicked);
}

void DashboardWidget::odswiezStatystykiGlowne() {
    auto stats = appController.getGlobalStats();
    if (stats.value("Suma", 0) == 0) return;

    QPieSeries *series = new QPieSeries();
    series->append("Planowane", stats.value("Planowane", 0));
    series->append("W trakcie", stats.value("W trakcie", 0));
    series->append("Ukończone", stats.value("Ukończone", 0));
    series->append("Porzucone", stats.value("Porzucone", 0));

    series->slices().at(0)->setColor(QColor("#7f8c8d"));
    series->slices().at(1)->setColor(QColor("#2980b9"));
    series->slices().at(2)->setColor(QColor("#27ae60"));
    series->slices().at(3)->setColor(QColor("#c0392b"));

    for(auto slice : series->slices()) {
        if (slice->value() > 0) {
            slice->setLabelVisible(true);
            slice->setLabel(QString("%1: %2").arg(slice->label()).arg(slice->value()));
        }
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Status Biblioteki");
    chart->legend()->hide();
    chart->setAnimationOptions(QChart::SeriesAnimations);

    ui->wykresOwalny->setChart(chart);
    ui->wykresOwalny->setRenderHint(QPainter::Antialiasing);

    QLayoutItem *child;
    while ((child = ui->gridOstatnie->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    QList<int> ostatnieId = appController.pobierzOstatnioAktywne(6);
    int wiersz = 0, kolumna = 0;

    auto lista = appController.pobierzWszystkieMultimedia();

    for (int id : ostatnieId) {
        for (const auto& m : lista) {
            if (m->getId() == id) {
                QPushButton *btnKafel = new QPushButton(m->getTytul(), this);
                btnKafel->setMinimumHeight(50);
                connect(btnKafel, &QPushButton::clicked, this, [this, id]() { emit zadaniePokazaniaSzczegolow(id); });
                ui->gridOstatnie->addWidget(btnKafel, wiersz, kolumna);
                kolumna++;
                if (kolumna > 1) { kolumna = 0; wiersz++; }
                break;
            }
        }
    }
}

void DashboardWidget::onBtnLosujClicked() {
    auto lista = appController.pobierzWszystkieMultimedia();
    QList<int> doWylosowania;
    for (const auto& m : lista) {
        if (m->getStatus() == "Planowane") doWylosowania.append(m->getId());
    }
    if (doWylosowania.isEmpty()) {
        QMessageBox::information(this, "Pusto!", "Nie masz żadnych 'Planowanych' pozycji.");
        return;
    }
    int wylosowaneId = doWylosowania.at(QRandomGenerator::global()->bounded(doWylosowania.size()));

    emit zadaniePokazaniaSzczegolow(wylosowaneId);
}
