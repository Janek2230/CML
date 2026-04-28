#include "databasemanager.h"
#include <QSettings>

DatabaseManager::DatabaseManager() {
    db = QSqlDatabase::addDatabase("QPSQL");

    QSettings settings("config.ini", QSettings::IniFormat);

    db.setHostName(settings.value("database/host", "localhost").toString());
    db.setDatabaseName(settings.value("database/name", "cml").toString());
    db.setUserName(settings.value("database/user", "postgres").toString());
    db.setPassword(settings.value("database/password", "").toString());
}

bool DatabaseManager::openConnection() {
    if (!db.open()) {
        qDebug() << "Błąd połączenia z bazą:" << db.lastError().text();
        return false;
    }
    qDebug() << "Połączono z bazą PostgreSQL!";
    return true;
}

void DatabaseManager::closeConnection() {
    db.close();
}

QMap<int, QString> DatabaseManager::getCategories() {
    QMap<int, QString> kategorie;
    QSqlQuery query("SELECT id, nazwa FROM kategorie ORDER BY id");

    while (query.next()) {
        kategorie.insert(query.value(0).toInt(), query.value(1).toString());
    }
    return kategorie;
}

QList<std::shared_ptr<Multimedia>> DatabaseManager::getAllMultimedia() {
    QList<std::shared_ptr<Multimedia>> list;

    QSqlQuery query(
        "SELECT DISTINCT ON (m.id) "
        "m.id, m.tytul, p.status, k.id, k.jednostka, "
        "p.wartosc_aktualna, p.wartosc_docelowa, m.data_dodania, "
        "m.id_platformy, p.ocena, "
        "COALESCE(MAX(COALESCE(d.czas_zakonczenia, d.czas_rozpoczecia)) OVER (PARTITION BY p.id), m.data_dodania) AS ostatnia_aktywnosc, "
        "p.numer_podejscia "
        "FROM multimedia m "
        "LEFT JOIN kategorie k ON m.id_kategorii = k.id "
        "LEFT JOIN podejscia p ON m.id = p.id_medium "
        "LEFT JOIN dziennik_aktywnosci d ON p.id = d.id_podejscia "
        "ORDER BY m.id, p.numer_podejscia DESC;"
        );

    while (query.next()) {
        Postep postep;
        postep.aktualna = query.value(5).toInt();
        postep.docelowa = query.value(6).toInt();
        postep.jednostka = query.value(4).toString();
        postep.numer_podejscia = query.value(11).toInt();

        auto medium = std::make_shared<Multimedia>(
            query.value(0).toInt(), query.value(1).toString(),
            query.value(3).toInt(), query.value(8).toInt(),
            query.value(2).toString(), postep
            );
        medium->setDataDodania(query.value(7).toDateTime());
        medium->setOcena(query.value(9).toInt());
        medium->setDataOstatniejAktywnosci(query.value(10).toDateTime());
        list.append(medium);
    }
    return list;
}


QMap<QString, int> DatabaseManager::getGlobalStats() {
    QMap<QString, int> stats;
    QSqlQuery query(
        "SELECT status, COUNT(*) FROM ("
        "  SELECT DISTINCT ON (id_medium) status "
        "  FROM podejscia "
        "  ORDER BY id_medium, numer_podejscia DESC"
        ") AS najnowsze "
        "GROUP BY status"
        );

    int suma = 0;
    while (query.next()) {
        QString status = query.value(0).toString();
        int ilosc = query.value(1).toInt();
        stats.insert(status, ilosc);
        suma += ilosc;
    }
    stats.insert("Suma", suma);
    return stats;
}

bool DatabaseManager::aktualizujPostep(int idMedium, const QString& status, int aktualna, int docelowa, int ocena) {
    db.transaction();

    QSqlQuery qSel(db);
    qSel.prepare("SELECT id, wartosc_aktualna, data_rozpoczecia FROM podejscia WHERE id_medium = :id ORDER BY numer_podejscia DESC LIMIT 1");
    qSel.bindValue(":id", idMedium);
    if (!qSel.exec() || !qSel.next()) { db.rollback(); return false; }

    int idPodejscia = qSel.value(0).toInt();
    int staraWartosc = qSel.value(1).toInt();
    bool brakDatyRozpoczecia = qSel.value(2).isNull();

    int przyrost = aktualna - staraWartosc;

    QSqlQuery qP(db);
    QString qpSql = "UPDATE podejscia SET wartosc_aktualna = :akt, wartosc_docelowa = :doc, status = :status, ocena = :ocena";

    if (status == "W trakcie" && brakDatyRozpoczecia) {
        qpSql += ", data_rozpoczecia = CURRENT_TIMESTAMP";
    } else if (status == "Ukończone" || status == "Porzucone") {
        qpSql += ", data_ukonczenia = CURRENT_TIMESTAMP";
    }
    qpSql += " WHERE id = :id";

    qP.prepare(qpSql);
    qP.bindValue(":akt", aktualna);
    qP.bindValue(":doc", docelowa);
    qP.bindValue(":status", status);
    qP.bindValue(":ocena", ocena > 0 ? ocena : QVariant(QMetaType::fromType<int>()));
    qP.bindValue(":id", idPodejscia);
    if (!qP.exec()) { db.rollback(); return false; }

    if (przyrost != 0) {
        QSqlQuery qLog(db);
        qLog.prepare("INSERT INTO dziennik_aktywnosci (id_podejscia, przyrost_jednostek, czas_rozpoczecia, czas_zakonczenia, czas_trwania_sekundy) "
                     "VALUES (:id, :przyrost, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, :sekundy)");
        qLog.bindValue(":id", idPodejscia);
        qLog.bindValue(":przyrost", przyrost);
        qLog.bindValue(":sekundy", przyrost * 3600);

        if (!qLog.exec()) { db.rollback(); return false; }
    }

    db.commit();
    return true;
}

bool DatabaseManager::zacznijOdNowa(int idMedium) {
    db.transaction();

    QSqlQuery qSel(db);
    qSel.prepare("SELECT numer_podejscia, wartosc_docelowa FROM podejscia WHERE id_medium = :id ORDER BY numer_podejscia DESC LIMIT 1");
    qSel.bindValue(":id", idMedium);

    if (!qSel.exec() || !qSel.next()) { db.rollback(); return false; }

    int nowyNumer = qSel.value(0).toInt() + 1;
    int staryCel = qSel.value(1).toInt();

    QSqlQuery qIns(db);
    qIns.prepare("INSERT INTO podejscia (id_medium, numer_podejscia, status, wartosc_aktualna, wartosc_docelowa, data_rozpoczecia) "
                 "VALUES (:id, :nr, 'W trakcie', 0, :cel, CURRENT_TIMESTAMP)");
    qIns.bindValue(":id", idMedium);
    qIns.bindValue(":nr", nowyNumer);
    qIns.bindValue(":cel", staryCel);

    if (!qIns.exec()) { db.rollback(); return false; }

    db.commit();
    return true;
}


bool DatabaseManager::dodajNoweMedium(const QString &tytul, int idKat, int idPlatformy, int cel) {
    db.transaction();

    QSqlQuery query1(db);
    query1.prepare("INSERT INTO multimedia (tytul, id_kategorii, id_platformy) VALUES (:tytul, :idKat, :idPlat) RETURNING id");
    query1.bindValue(":tytul", tytul);
    query1.bindValue(":idKat", idKat > 0 ? QVariant(idKat) : QVariant(QMetaType::fromType<int>()));
    query1.bindValue(":idPlat", idPlatformy > 0 ? QVariant(idPlatformy) : QVariant(QMetaType::fromType<int>()));

    if (!query1.exec() || !query1.next()) { db.rollback(); return false; }
    int noweId = query1.value(0).toInt();

    QSqlQuery query2(db);
    query2.prepare("INSERT INTO podejscia (id_medium, numer_podejscia, wartosc_aktualna, wartosc_docelowa, status) VALUES (:id, 1, 0, :cel, 'Planowane')");
    query2.bindValue(":id", noweId);
    query2.bindValue(":cel", cel);

    if (!query2.exec()) { db.rollback(); return false; }

    db.commit();
    return true;
}


QList<QPair<int, QString>> DatabaseManager::pobierzKategorie() {
    QList<QPair<int, QString>> lista;
    QSqlQuery query("SELECT id, nazwa FROM kategorie ORDER BY id");
    while(query.next()) {
        lista.append({query.value(0).toInt(), query.value(1).toString()});
    }
    return lista;
}

QList<QPair<int, QString>> DatabaseManager::pobierzPlatformy() {
    QList<QPair<int, QString>> lista;
    QSqlQuery query("SELECT id, nazwa FROM platformy ORDER BY id");
    while(query.next()) {
        lista.append({query.value(0).toInt(), query.value(1).toString()});
    }
    return lista;
}

bool DatabaseManager::aktualizujDaneMedium(int id, const QString &tytul, int idKat, int idPlatformy, int cel) {
    db.transaction();
    QSqlQuery query(db);

    query.prepare("UPDATE multimedia SET tytul = :tytul, id_kategorii = :idKat, id_platformy = :idPlat WHERE id = :id");
    query.bindValue(":tytul", tytul);
    query.bindValue(":idKat", idKat > 0 ? QVariant(idKat) : QVariant(QMetaType::fromType<int>()));
    query.bindValue(":idPlat", idPlatformy > 0 ? QVariant(idPlatformy) : QVariant(QMetaType::fromType<int>()));
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "Błąd UPDATE multimedia:" << query.lastError().text();
        db.rollback();
        return false;
    }

    QSqlQuery query2(db);
    query2.prepare("UPDATE podejscia SET wartosc_docelowa = :cel WHERE id = (SELECT id FROM podejscia WHERE id_medium = :id ORDER BY numer_podejscia DESC LIMIT 1)");
    query2.bindValue(":cel", cel);
    query2.bindValue(":id", id);

    if (!query2.exec()) {
        qDebug() << "Błąd UPDATE podejscia:" << query2.lastError().text();
        db.rollback();
        return false;
    }

    db.commit();
    return true;
}
int DatabaseManager::dodajKategorie(const QString &nazwa, const QString &jednostka) {
    QSqlQuery query;
    query.prepare("INSERT INTO kategorie (nazwa, jednostka) VALUES (:nazwa, :jednostka) RETURNING id");
    query.bindValue(":nazwa", nazwa);
    query.bindValue(":jednostka", jednostka.isEmpty() ? "szt." : jednostka);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return -1;
}

int DatabaseManager::dodajPlatforme(const QString &nazwa) {
    QSqlQuery query;
    query.prepare("INSERT INTO platformy (nazwa, typ_nosnika) VALUES (:nazwa, :typ) RETURNING id");
    query.bindValue(":nazwa", nazwa);
    query.bindValue(":typ", "Cyfrowa"); // Domyślna wartość

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return -1;
}

QStringList DatabaseManager::pobierzUnikalneJednostki() {
    QStringList lista;
    QSqlQuery query("SELECT DISTINCT jednostka FROM kategorie WHERE jednostka IS NOT NULL AND jednostka != ''");

    while (query.next()) {
        lista.append(query.value(0).toString());
    }

    if (lista.isEmpty()) {
        lista << "szt." << "strony" << "odcinki" << "seanse" << "godziny";
    }
    return lista;
}

bool DatabaseManager::aktualizujKategorie(int id, const QString &nazwa, const QString &jednostka) {
    QSqlQuery query;
    query.prepare("UPDATE kategorie SET nazwa = :nazwa, jednostka = :jednostka WHERE id = :id");
    query.bindValue(":nazwa", nazwa);
    query.bindValue(":jednostka", jednostka);
    query.bindValue(":id", id);
    return query.exec();
}

bool DatabaseManager::aktualizujPlatforme(int id, const QString &nazwa) {
    QSqlQuery query;
    query.prepare("UPDATE platformy SET nazwa = :nazwa WHERE id = :id");
    query.bindValue(":nazwa", nazwa);
    query.bindValue(":id", id);
    return query.exec();
}


bool DatabaseManager::zmienKategorieWielu(const QList<int>& idMultimediow, int nowaKategoriaId) {
    if (idMultimediow.isEmpty()) return false;

    db.transaction();
    QSqlQuery query(db);
    query.prepare("UPDATE multimedia SET id_kategorii = :idKat WHERE id = :idMed");

    for (int id : idMultimediow) {
        query.bindValue(":idKat", nowaKategoriaId > 0 ? QVariant(nowaKategoriaId) : QVariant(QMetaType::fromType<int>()));
        query.bindValue(":idMed", id);

        if (!query.exec()) {
            qDebug() << "Błąd masowej aktualizacji:" << query.lastError().text();
            db.rollback();
            return false;
        }
    }

    db.commit();
    return true;
}


QMap<int, QString> DatabaseManager::pobierzSlownikJednostek() {
    QMap<int, QString> mapa;
    QSqlQuery q("SELECT id, jednostka FROM kategorie");
    while(q.next()) {
        mapa.insert(q.value(0).toInt(), q.value(1).toString());
    }
    return mapa;
}

QList<int> DatabaseManager::pobierzOstatnioAktywne(int limit) {
    QList<int> lista;
    QSqlQuery query;
    query.prepare(R"(
        SELECT m.id
        FROM multimedia m
        LEFT JOIN podejscia p ON m.id = p.id_medium
        LEFT JOIN dziennik_aktywnosci d ON p.id = d.id_podejscia
        GROUP BY m.id, m.data_dodania
        ORDER BY GREATEST(m.data_dodania, MAX(COALESCE(d.czas_zakonczenia, d.czas_rozpoczecia))) DESC NULLS LAST
        LIMIT :limit
    )");
    query.bindValue(":limit", limit);
    if (query.exec()) {
        while(query.next()) {
            lista.append(query.value(0).toInt());
        }
    }
    return lista;
}

bool DatabaseManager::usunMedium(int id) {
    QSqlQuery query;
    query.prepare("DELETE FROM multimedia WHERE id = :id");
    query.bindValue(":id", id);
    return query.exec();
}

bool DatabaseManager::usunWieleMultimediow(const QList<int>& idList) {
    if (idList.isEmpty()) return false;
    db.transaction();

    QSqlQuery qMultimedia(db);
    qMultimedia.prepare("DELETE FROM multimedia WHERE id = :id");

    for (int id : idList) {
        qMultimedia.bindValue(":id", id);
        if (!qMultimedia.exec()) {
            db.rollback();
            return false;
        }
    }

    db.commit();
    return true;
}

bool DatabaseManager::usunKategorie(int idKat, bool usunPowiazane) {
    db.transaction();

    if (usunPowiazane) {
        QSqlQuery q2(db);
        q2.prepare("DELETE FROM multimedia WHERE id_kategorii = :id");
        q2.bindValue(":id", idKat);
        if(!q2.exec()) { db.rollback(); return false; }
    } else {
        QSqlQuery q3(db);
        q3.prepare("UPDATE multimedia SET id_kategorii = NULL WHERE id_kategorii = :id");
        q3.bindValue(":id", idKat);
        if(!q3.exec()) { db.rollback(); return false; }
    }

    QSqlQuery q4(db);
    q4.prepare("DELETE FROM kategorie WHERE id = :id");
    q4.bindValue(":id", idKat);
    if(!q4.exec()) { db.rollback(); return false; }

    db.commit();
    return true;
}

bool DatabaseManager::usunPlatforme(int idPlat, bool usunPowiazane) {
    db.transaction();

    if (usunPowiazane) {
        QSqlQuery q2(db);
        q2.prepare("DELETE FROM multimedia WHERE id_platformy = :id");
        q2.bindValue(":id", idPlat);
        if(!q2.exec()) { db.rollback(); return false; }
    } else {
        QSqlQuery q3(db);
        q3.prepare("UPDATE multimedia SET id_platformy = NULL WHERE id_platformy = :id");
        q3.bindValue(":id", idPlat);
        if(!q3.exec()) { db.rollback(); return false; }
    }

    QSqlQuery q4(db);
    q4.prepare("DELETE FROM platformy WHERE id = :id");
    q4.bindValue(":id", idPlat);
    if(!q4.exec()) { db.rollback(); return false; }

    db.commit();
    return true;
}

QList<StatystykaAktywnosci> DatabaseManager::pobierzSuroweDaneStatystyk(int zakresDni, const QString& metryka) {
    QList<StatystykaAktywnosci> wyniki;
    QSqlQuery query(db);

    QString funkcjaAgregujaca;
    if (metryka == "czas") {
        funkcjaAgregujaca = "SUM(d.czas_trwania_sekundy)"; // SUROWE SEKUNDY!
    } else if (metryka == "sesje") {
        funkcjaAgregujaca = "COUNT(d.id)";
    } else if (metryka == "jednostki") {
        funkcjaAgregujaca = "SUM(d.przyrost_jednostek)";
    } else {
        return wyniki;
    }

    QString formatDaty = (zakresDni > 60) ? "'YYYY-MM'" : "'MM-DD'";

    QString sql = QString(R"(
        SELECT
            TO_CHAR(COALESCE(d.czas_zakonczenia, d.czas_rozpoczecia, CURRENT_TIMESTAMP), %1) as kategoria_czasu,
            m.tytul as nazwa_serii,
            %2 as wartosc
        FROM dziennik_aktywnosci d
        JOIN podejscia p ON d.id_podejscia = p.id
        JOIN multimedia m ON p.id_medium = m.id
        WHERE COALESCE(d.czas_zakonczenia, d.czas_rozpoczecia, CURRENT_TIMESTAMP) >= CURRENT_DATE - INTERVAL '%3 days'
          AND COALESCE(d.czas_zakonczenia, d.czas_rozpoczecia, CURRENT_TIMESTAMP) <= CURRENT_TIMESTAMP
        GROUP BY kategoria_czasu, m.tytul
        ORDER BY kategoria_czasu ASC
    )").arg(formatDaty).arg(funkcjaAgregujaca).arg(zakresDni);

    if (!query.exec(sql)) {
        qDebug() << "Błąd pobierania danych do wykresu:" << query.lastError().text();
        return wyniki;
    }

    while (query.next()) {
        StatystykaAktywnosci s;
        s.data = query.value(0).toString();
        s.nazwaSerii = query.value(1).toString();
        s.wartosc = query.value(2).toDouble();
        wyniki.append(s);
    }
    return wyniki;
}

QList<QList<QVariant>> DatabaseManager::pobierzSuroweKategorie() {
    QList<QList<QVariant>> wynik;
    QSqlQuery q("SELECT id, nazwa, jednostka FROM kategorie ORDER BY nazwa");
    while(q.next()) {
        wynik.append({q.value(0), q.value(1), q.value(2)});
    }
    return wynik;
}

QList<PodejscieHistoryczne> DatabaseManager::pobierzPelnaHistorie(int idMedium) {
    QList<PodejscieHistoryczne> historia;

    // 1. Pobieramy wszystkie podejścia dla danego medium
    QSqlQuery qP(db);
    qP.prepare("SELECT id, numer_podejscia, status, wartosc_aktualna, wartosc_docelowa, ocena, recenzja "
               "FROM podejscia WHERE id_medium = :id ORDER BY numer_podejscia DESC");
    qP.bindValue(":id", idMedium);

    if (!qP.exec()) return historia;

    while (qP.next()) {
        PodejscieHistoryczne p;
        p.id = qP.value(0).toInt();
        p.numer = qP.value(1).toInt();
        p.status = qP.value(2).toString();
        p.aktualna = qP.value(3).toInt();
        p.docelowa = qP.value(4).toInt();
        p.ocena = qP.value(5).toInt();
        p.recenzja = qP.value(6).toString();

        // 2. Dla każdego podejścia pobieramy jego sesje z dziennika
        QSqlQuery qS(db);
        qS.prepare("SELECT id, COALESCE(czas_zakonczenia, czas_rozpoczecia), przyrost_jednostek, czas_trwania_sekundy, notatka "
                   "FROM dziennik_aktywnosci WHERE id_podejscia = :idP ORDER BY czas_rozpoczecia DESC");
        qS.bindValue(":idP", p.id);

        if (qS.exec()) {
            while (qS.next()) {
                Sesja s;
                s.id = qS.value(0).toInt();
                s.data = qS.value(1).toDateTime();
                s.przyrost = qS.value(2).toInt();
                s.sekundy = qS.value(3).toInt();
                s.notatka = qS.value(4).toString();
                p.sesje.append(s);
            }
        }
        historia.append(p);
    }
    return historia;
}
