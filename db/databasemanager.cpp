#include "databasemanager.h"
#include <QSettings>

DatabaseManager::DatabaseManager() {
    db = QSqlDatabase::addDatabase("QPSQL");

    // Dane połączenia z pliku config.ini, klucze w sekcji [database].
    // Plik musi istnieć obok pliku wykonywalnego; wartości domyślne działają lokalnie.
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

    // DISTINCT ON (m.id) to składnia specyficzna dla PostgreSQL.
    // Działa tak: wiersze są najpierw sortowane przez ORDER BY (m.id ASC, p.numer_podejscia DESC),
    // a następnie dla każdej unikalnej wartości m.id zostaje tylko PIERWSZY wiersz z grupy.
    // Efekt: dla każdego medium dostajemy dane NAJNOWSZEGO podejścia.
    //
    // Okno MAX(...) OVER (PARTITION BY p.id) wyznacza datę ostatniej sesji w ramach
    // danego podejścia. COALESCE z m.data_dodania służy jako fallback gdy brak sesji.
    //
    // Mapa kolumn wynikowych:
    //  0: m.id          1: m.tytul        2: p.status       3: k.id (kategoria)
    //  4: k.jednostka   5: p.wartosc_akt  6: p.wartosc_doc  7: m.data_dodania
    //  8: m.id_platformy  9: p.ocena     10: m.czy_ulubione 11: ostatnia_aktywnosc
    // 12: p.numer_podejscia  13: m.rok_wydania  14: m.tworcy
    QSqlQuery query(
        "SELECT DISTINCT ON (m.id) "
        "m.id, m.tytul, p.status, k.id, k.jednostka, "
        "p.wartosc_aktualna, p.wartosc_docelowa, m.data_dodania, "
        "m.id_platformy, p.ocena, m.czy_ulubione, "
        "COALESCE(MAX(COALESCE(d.czas_zakonczenia, d.czas_rozpoczecia)) OVER (PARTITION BY p.id), m.data_dodania) AS ostatnia_aktywnosc, "
        "p.numer_podejscia, m.rok_wydania, m.tworcy "
        "FROM multimedia m "
        "LEFT JOIN kategorie k ON m.id_kategorii = k.id "
        "LEFT JOIN podejscia p ON m.id = p.id_medium "
        "LEFT JOIN dziennik_aktywnosci d ON p.id = d.id_podejscia "
        "ORDER BY m.id, p.numer_podejscia DESC;"
        );

    while (query.next()) {
        Postep postep;
        postep.aktualna       = query.value(5).toInt();
        postep.docelowa       = query.value(6).toInt();
        postep.jednostka      = query.value(4).toString();
        postep.numer_podejscia = query.value(12).toInt();

        auto medium = std::make_shared<Multimedia>(
            query.value(0).toInt(),  // id
            query.value(1).toString(), // tytul
            query.value(3).toInt(),  // id_kategorii
            query.value(8).toInt(),  // id_platformy
            query.value(2).toString(), // status
            postep
            );
        medium->setDataDodania(query.value(7).toDateTime());
        medium->setOcena(query.value(9).toInt());
        medium->setCzyUlubione(query.value(10).toBool());
        medium->setDataOstatniejAktywnosci(query.value(11).toDateTime());
        medium->setRokWydania(query.value(13).toInt());
        medium->setTworcy(query.value(14).toString());
        list.append(medium);
    }
    return list;
}


QMap<QString, int> DatabaseManager::getGlobalStats() {
    QMap<QString, int> stats;

    // Podzapytanie z DISTINCT ON pobiera aktualny status każdego medium (z najnowszego podejścia).
    // Zewnętrzne GROUP BY zlicza ile mediów jest w każdym statusie.
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

    // Odczytujemy aktualne dane podejścia, żeby:
    // a) obliczyć przyrost (różnicę) do wpisu w dzienniku,
    // b) sprawdzić czy data_rozpoczecia jeszcze nie była ustawiona.
    QSqlQuery qSel(db);
    qSel.prepare("SELECT id, wartosc_aktualna, data_rozpoczecia FROM podejscia WHERE id_medium = :id ORDER BY numer_podejscia DESC LIMIT 1");
    qSel.bindValue(":id", idMedium);
    if (!qSel.exec() || !qSel.next()) { db.rollback(); return false; }

    int idPodejscia        = qSel.value(0).toInt();
    int staraWartosc       = qSel.value(1).toInt();
    bool brakDatyRozpoczecia = qSel.value(2).isNull();

    int przyrost = aktualna - staraWartosc;

    // Dynamicznie budujemy UPDATE — daty są ustawiane jednorazowo przy pierwszym przejściu
    // w dany status, a nie nadpisywane przy każdym zapisie.
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
    // QVariant(QMetaType::fromType<int>()) to Qt6-owy sposób bindowania NULL dla kolumny INT.
    qP.bindValue(":ocena", ocena > 0 ? QVariant(ocena) : QVariant(QMetaType::fromType<int>()));
    qP.bindValue(":id", idPodejscia);
    if (!qP.exec()) { db.rollback(); return false; }

    // Jeśli wartość się zmieniła, logujemy automatyczny wpis w dzienniku.
    if (przyrost != 0) {
        QSqlQuery qLog(db);
        qLog.prepare("INSERT INTO dziennik_aktywnosci (id_podejscia, przyrost_jednostek, czas_rozpoczecia, czas_zakonczenia, czas_trwania_sekundy) "
                     "VALUES (:id, :przyrost, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, :sekundy)");
        qLog.bindValue(":id", idPodejscia);
        qLog.bindValue(":przyrost", przyrost);
        // UWAGA: czas_trwania_sekundy jest tu szacowany jako przyrost * 3600 (1 jednostka = 1h).
        // To przybliżenie wynikające z braku stopera w tej ścieżce zapisu.
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


int DatabaseManager::dodajNoweMedium(const QString &tytul, int idKat, int idPlatformy, int cel, int rokWydania, const QString &tworcy) {
    // Dodanie medium to zawsze dwa INSERT-y w jednej transakcji:
    // najpierw wiersz w multimedia (RETURNING id), potem pierwsze podejście ze statusem 'Planowane'.
    // id=0 oznacza brak wyboru, bindujemy NULL żeby FK nie wskazywał na nieistniejący rekord.
    db.transaction();

    QSqlQuery query1(db);
    query1.prepare("INSERT INTO multimedia (tytul, id_kategorii, id_platformy, rok_wydania, tworcy) "
                   "VALUES (:tytul, :idKat, :idPlat, :rok, :tworcy) RETURNING id");
    query1.bindValue(":tytul", tytul);
    query1.bindValue(":idKat",  idKat      > 0 ? QVariant(idKat)      : QVariant(QMetaType::fromType<int>()));
    query1.bindValue(":idPlat", idPlatformy > 0 ? QVariant(idPlatformy) : QVariant(QMetaType::fromType<int>()));
    // rok=0 oznacza "nieznany" → zapisujemy NULL zamiast zera.
    query1.bindValue(":rok",    rokWydania > 0 ? QVariant(rokWydania) : QVariant(QMetaType::fromType<int>()));
    query1.bindValue(":tworcy", tworcy.trimmed().isEmpty() ? QVariant(QMetaType::fromType<QString>()) : QVariant(tworcy.trimmed()));

    if (!query1.exec() || !query1.next()) { db.rollback(); return -1; }
    int noweId = query1.value(0).toInt();

    QSqlQuery query2(db);
    query2.prepare("INSERT INTO podejscia (id_medium, numer_podejscia, wartosc_aktualna, wartosc_docelowa, status) VALUES (:id, 1, 0, :cel, 'Planowane')");
    query2.bindValue(":id",  noweId);
    query2.bindValue(":cel", cel);

    if (!query2.exec()) { db.rollback(); return -1; }

    db.commit();
    return noweId;
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

QList<QPair<int, QString>> DatabaseManager::pobierzTagi() {
    QList<QPair<int, QString>> lista;
    QSqlQuery query("SELECT id, nazwa FROM tagi ORDER BY nazwa");
    while (query.next()) {
        lista.append({query.value(0).toInt(), query.value(1).toString()});
    }
    return lista;
}

bool DatabaseManager::aktualizujDaneMedium(int id, const QString &tytul, int idKat, int idPlatformy, int cel, int rokWydania, const QString &tworcy) {
    db.transaction();
    QSqlQuery query(db);

    query.prepare("UPDATE multimedia SET tytul = :tytul, id_kategorii = :idKat, id_platformy = :idPlat, "
                  "rok_wydania = :rok, tworcy = :tworcy WHERE id = :id");
    query.bindValue(":tytul", tytul);
    query.bindValue(":idKat", idKat > 0 ? QVariant(idKat) : QVariant(QMetaType::fromType<int>()));
    query.bindValue(":idPlat", idPlatformy > 0 ? QVariant(idPlatformy) : QVariant(QMetaType::fromType<int>()));
    query.bindValue(":rok",    rokWydania > 0 ? QVariant(rokWydania) : QVariant(QMetaType::fromType<int>()));
    query.bindValue(":tworcy", tworcy.trimmed().isEmpty() ? QVariant(QMetaType::fromType<QString>()) : QVariant(tworcy.trimmed()));
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
    QSqlQuery query(db);
    query.prepare("INSERT INTO kategorie (nazwa, jednostka) VALUES (:nazwa, :jednostka) RETURNING id");
    query.bindValue(":nazwa", nazwa);
    query.bindValue(":jednostka", jednostka.isEmpty() ? "szt." : jednostka);

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return -1;
}

int DatabaseManager::dodajPlatforme(const QString &nazwa, const QString &typNosnika) {
    QSqlQuery query(db);
    query.prepare("INSERT INTO platformy (nazwa, typ_nosnika) VALUES (:nazwa, :typ) RETURNING id");
    query.bindValue(":nazwa", nazwa);
    // Kolumna NOT NULL — gdy nie podano typu, używamy domyślnego "Cyfrowy".
    query.bindValue(":typ", typNosnika.trimmed().isEmpty() ? "Cyfrowy" : typNosnika.trimmed());

    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return -1;
}

QList<QList<QVariant>> DatabaseManager::pobierzSurowePlatformy() {
    QList<QList<QVariant>> wynik;
    QSqlQuery q("SELECT id, nazwa, typ_nosnika FROM platformy ORDER BY id");
    while (q.next()) {
        wynik.append({q.value(0), q.value(1), q.value(2)});
    }
    return wynik;
}

QStringList DatabaseManager::pobierzUnikalneJednostki() {
    QStringList lista;
    QSqlQuery query("SELECT DISTINCT jednostka FROM kategorie WHERE jednostka IS NOT NULL AND jednostka != ''");

    while (query.next()) {
        lista.append(query.value(0).toString());
    }

    // Gdy baza jest pusta (np. świeża instalacja), zwracamy sensowny zestaw podpowiedzi.
    if (lista.isEmpty()) {
        lista << "szt." << "strony" << "odcinki" << "seanse" << "godziny";
    }
    return lista;
}

bool DatabaseManager::aktualizujKategorie(int id, const QString &nazwa, const QString &jednostka) {
    QSqlQuery query(db);
    query.prepare("UPDATE kategorie SET nazwa = :nazwa, jednostka = :jednostka WHERE id = :id");
    query.bindValue(":nazwa", nazwa);
    query.bindValue(":jednostka", jednostka);
    query.bindValue(":id", id);
    return query.exec();
}

bool DatabaseManager::aktualizujPlatforme(int id, const QString &nazwa, const QString &typNosnika) {
    QSqlQuery query(db);
    query.prepare("UPDATE platformy SET nazwa = :nazwa, typ_nosnika = :typ WHERE id = :id");
    query.bindValue(":nazwa", nazwa);
    query.bindValue(":typ", typNosnika.trimmed().isEmpty() ? "Cyfrowy" : typNosnika.trimmed());
    query.bindValue(":id", id);
    return query.exec();
}

bool DatabaseManager::aktualizujTag(int id, const QString &nazwa) {
    QSqlQuery query(db);
    query.prepare("UPDATE tagi SET nazwa = :nazwa WHERE id = :id");
    query.bindValue(":nazwa", nazwa);
    query.bindValue(":id", id);
    return query.exec();
}

int DatabaseManager::dodajTag(const QString &nazwa) {
    QSqlQuery query(db);
    query.prepare("INSERT INTO tagi (nazwa) VALUES (:nazwa) RETURNING id");
    query.bindValue(":nazwa", nazwa);
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    return -1;
}

bool DatabaseManager::usunTag(int idTagu) {
    QSqlQuery query(db);
    query.prepare("DELETE FROM tagi WHERE id = :id");
    query.bindValue(":id", idTagu);
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
    QSqlQuery query(db);
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
    QSqlQuery query(db);
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
        // Tryb "usuń wszystko": kasujemy multimedia należące do tej kategorii.
        QSqlQuery q2(db);
        q2.prepare("DELETE FROM multimedia WHERE id_kategorii = :id");
        q2.bindValue(":id", idKat);
        if (!q2.exec()) { db.rollback(); return false; }
    } else {
        // Tryb "osierocenie": multimedia zostają, ale tracą powiązanie z kategorią (NULL).
        QSqlQuery q3(db);
        q3.prepare("UPDATE multimedia SET id_kategorii = NULL WHERE id_kategorii = :id");
        q3.bindValue(":id", idKat);
        if (!q3.exec()) { db.rollback(); return false; }
    }

    QSqlQuery q4(db);
    q4.prepare("DELETE FROM kategorie WHERE id = :id");
    q4.bindValue(":id", idKat);
    if (!q4.exec()) { db.rollback(); return false; }

    db.commit();
    return true;
}

bool DatabaseManager::usunPlatforme(int idPlat, bool usunPowiazane) {
    db.transaction();

    if (usunPowiazane) {
        // Tryb "usuń wszystko": kasujemy multimedia przypisane do tej platformy.
        QSqlQuery q2(db);
        q2.prepare("DELETE FROM multimedia WHERE id_platformy = :id");
        q2.bindValue(":id", idPlat);
        if (!q2.exec()) { db.rollback(); return false; }
    } else {
        // Tryb "osierocenie": multimedia zostają, id_platformy ustawiane na NULL.
        QSqlQuery q3(db);
        q3.prepare("UPDATE multimedia SET id_platformy = NULL WHERE id_platformy = :id");
        q3.bindValue(":id", idPlat);
        if (!q3.exec()) { db.rollback(); return false; }
    }

    QSqlQuery q4(db);
    q4.prepare("DELETE FROM platformy WHERE id = :id");
    q4.bindValue(":id", idPlat);
    if (!q4.exec()) { db.rollback(); return false; }

    db.commit();
    return true;
}

QList<StatystykaAktywnosci> DatabaseManager::pobierzSuroweDaneStatystyk(int zakresDni, const QString& metryka) {
    QList<StatystykaAktywnosci> wyniki;
    QSqlQuery query(db);

    // Funkcja agregująca zależy od wybranej metryki.
    // "czas" zwraca sekundy (konwersja na godziny odbywa się w AppController).
    QString funkcjaAgregujaca;
    if (metryka == "czas") {
        funkcjaAgregujaca = "SUM(d.czas_trwania_sekundy)";
    } else if (metryka == "sesje") {
        funkcjaAgregujaca = "COUNT(d.id)";
    } else if (metryka == "jednostki") {
        funkcjaAgregujaca = "SUM(d.przyrost_jednostek)";
    } else {
        return wyniki;
    }

    // Format daty na osi X dobieramy do rozdzielczości wybranego zakresu:
    //  ≤ 30 dni  → "DD.MM"   (widoczne pojedyncze dni)
    //  ≤ 365 dni → "MM.YYYY" (widoczne miesiące)
    //  > 1000    → "YYYY"    (Cała historia — granulacja roczna)
    //  pozostałe → "YYYY-MM" (zakres kilku miesięcy)
    QString formatDaty;
    if (zakresDni <= 30) {
        formatDaty = "'DD.MM'";
    } else if (zakresDni <= 365) {
        formatDaty = "'MM.YYYY'";
    } else if (zakresDni > 1000) {
        formatDaty = "'YYYY'";
    } else {
        formatDaty = "'YYYY-MM'";
    }

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

    // Dwa osobne zapytania zamiast jednego JOIN-a, żeby uniknąć duplikowania wierszy podejść.
    // Dla każdego podejścia wykonywane jest osobne SELECT na dziennik — wzorzec N+1.
    // Przy typowych rozmiarach biblioteki (kilkaset tytułów) nie stanowi to problemu wydajnościowego.

    QSqlQuery qP(db);
    qP.prepare("SELECT id, numer_podejscia, status, wartosc_aktualna, wartosc_docelowa, ocena, recenzja, data_rozpoczecia "
               "FROM podejscia WHERE id_medium = :id ORDER BY numer_podejscia DESC");
    qP.bindValue(":id", idMedium);
    if (!qP.exec()) return historia;

    while (qP.next()) {
        PodejscieHistoryczne p;
        p.id             = qP.value(0).toInt();
        p.numer          = qP.value(1).toInt();
        p.status         = qP.value(2).toString();
        p.aktualna       = qP.value(3).toInt();
        p.docelowa       = qP.value(4).toInt();
        p.ocena          = qP.value(5).toInt();
        p.recenzja       = qP.value(6).toString();
        p.data_rozpoczecia = qP.value(7).toDateTime();

        // COALESCE(czas_zakonczenia, czas_rozpoczecia) — używamy daty zakończenia sesji
        // jeśli istnieje, w przeciwnym razie daty jej rozpoczęcia.
        QSqlQuery qS(db);
        qS.prepare("SELECT id, COALESCE(czas_zakonczenia, czas_rozpoczecia), przyrost_jednostek, czas_trwania_sekundy, notatka "
                   "FROM dziennik_aktywnosci WHERE id_podejscia = :idP ORDER BY czas_rozpoczecia DESC");
        qS.bindValue(":idP", p.id);

        if (qS.exec()) {
            while (qS.next()) {
                Sesja s;
                s.id       = qS.value(0).toInt();
                s.data     = qS.value(1).toDateTime();
                s.przyrost = qS.value(2).toInt();
                s.sekundy  = qS.value(3).toInt();
                s.notatka  = qS.value(4).toString();
                p.sesje.append(s);
            }
        }
        historia.append(p);
    }
    return historia;
}

QVariantMap DatabaseManager::pobierzStatystykiPodsumowania() {
    QVariantMap wynik;
    QSqlQuery q(db);
    q.prepare(R"(
        SELECT
            (SELECT COUNT(*) FROM multimedia) AS media_total,
            (SELECT COUNT(*) FROM podejscia) AS podejscia_total,
            (SELECT COUNT(*) FROM dziennik_aktywnosci) AS sesje_total,
            COALESCE((SELECT SUM(czas_trwania_sekundy) FROM dziennik_aktywnosci), 0) AS czas_total,
            COALESCE((SELECT SUM(przyrost_jednostek) FROM dziennik_aktywnosci), 0) AS jednostki_total,
            COALESCE((
                SELECT t.nazwa
                FROM tagi t
                JOIN multimedia_tagi mt ON mt.id_tagu = t.id
                JOIN podejscia p ON p.id_medium = mt.id_medium
                JOIN dziennik_aktywnosci d ON d.id_podejscia = p.id
                GROUP BY t.id, t.nazwa
                ORDER BY SUM(d.czas_trwania_sekundy) DESC
                LIMIT 1
            ), 'Brak danych') AS ulubiony_tag
    )");

    if (!q.exec() || !q.next()) {
        return wynik;
    }

    wynik["mediaTotal"] = q.value("media_total").toInt();
    wynik["podejsciaTotal"] = q.value("podejscia_total").toInt();
    wynik["sesjeTotal"] = q.value("sesje_total").toInt();
    wynik["czasTotal"] = q.value("czas_total").toLongLong();
    wynik["jednostkiTotal"] = q.value("jednostki_total").toLongLong();
    wynik["ulubionyTag"] = q.value("ulubiony_tag").toString();
    return wynik;
}

QList<QVariantMap> DatabaseManager::pobierzPodsumowanieKategorii() {
    QList<QVariantMap> wynik;
    QSqlQuery q(db);
    q.prepare(R"(
        SELECT
            COALESCE(k.nazwa, 'Brak kategorii') AS kategoria,
            COALESCE(SUM(d.czas_trwania_sekundy), 0) AS czas_sekundy,
            COALESCE(SUM(d.przyrost_jednostek), 0) AS przyrost_jednostek
        FROM multimedia m
        LEFT JOIN kategorie k ON m.id_kategorii = k.id
        LEFT JOIN podejscia p ON p.id_medium = m.id
        LEFT JOIN dziennik_aktywnosci d ON d.id_podejscia = p.id
        GROUP BY COALESCE(k.nazwa, 'Brak kategorii')
        ORDER BY czas_sekundy DESC, przyrost_jednostek DESC, kategoria ASC
    )");

    if (!q.exec()) {
        return wynik;
    }

    while (q.next()) {
        QVariantMap row;
        row["kategoria"] = q.value("kategoria").toString();
        row["czasSekundy"] = q.value("czas_sekundy").toLongLong();
        row["przyrostJednostek"] = q.value("przyrost_jednostek").toLongLong();
        wynik.append(row);
    }
    return wynik;
}

QList<QVariantMap> DatabaseManager::pobierzPodsumowanieTagow() {
    QList<QVariantMap> wynik;
    QSqlQuery q(db);
    q.prepare(R"(
        SELECT
            t.nazwa AS tag,
            COUNT(DISTINCT mt.id_medium) AS liczba_mediow,
            COALESCE(SUM(d.czas_trwania_sekundy), 0) AS czas_sekundy
        FROM tagi t
        LEFT JOIN multimedia_tagi mt ON mt.id_tagu = t.id
        LEFT JOIN multimedia m ON m.id = mt.id_medium
        LEFT JOIN podejscia p ON p.id_medium = m.id
        LEFT JOIN dziennik_aktywnosci d ON d.id_podejscia = p.id
        GROUP BY t.id, t.nazwa
        ORDER BY czas_sekundy DESC, liczba_mediow DESC, t.nazwa ASC
    )");

    if (!q.exec()) {
        return wynik;
    }

    while (q.next()) {
        QVariantMap row;
        row["tag"] = q.value("tag").toString();
        row["liczbaMediow"] = q.value("liczba_mediow").toInt();
        row["czasSekundy"] = q.value("czas_sekundy").toLongLong();
        wynik.append(row);
    }
    return wynik;
}

bool DatabaseManager::dodajPodejscie(int idMedium, const QString& status, int docelowa) {
    db.transaction();

    QSqlQuery qSel(db);
    qSel.prepare("SELECT COALESCE(MAX(numer_podejscia), 0) + 1 FROM podejscia WHERE id_medium = :id");
    qSel.bindValue(":id", idMedium);
    if (!qSel.exec() || !qSel.next()) { db.rollback(); return false; }
    int nowyNumer = qSel.value(0).toInt();

    QSqlQuery qIns(db);
    QString sql = "INSERT INTO podejscia (id_medium, numer_podejscia, status, wartosc_aktualna, wartosc_docelowa";
    if (status == "W trakcie") {
        sql += ", data_rozpoczecia";
    }
    sql += ") VALUES (:id, :nr, :status, 0, :doc";
    if (status == "W trakcie") {
        sql += ", CURRENT_TIMESTAMP";
    }
    sql += ")";

    qIns.prepare(sql);
    qIns.bindValue(":id", idMedium);
    qIns.bindValue(":nr", nowyNumer);
    qIns.bindValue(":status", status);
    qIns.bindValue(":doc", docelowa);

    if (!qIns.exec()) { db.rollback(); return false; }
    db.commit();
    return true;
}

bool DatabaseManager::aktualizujPodejscie(int idPodejscia, const QString& status, int aktualna, int docelowa, int ocena, const QString& recenzja) {
    QSqlQuery q(db);
    QString sql = "UPDATE podejscia SET status = :status, wartosc_aktualna = :akt, wartosc_docelowa = :doc, ocena = :ocena, recenzja = :rec";
    if (status == "W trakcie") {
        sql += ", data_rozpoczecia = COALESCE(data_rozpoczecia, CURRENT_TIMESTAMP)";
    }
    if (status == "Ukończone" || status == "Porzucone") {
        sql += ", data_ukonczenia = CURRENT_TIMESTAMP";
    }
    sql += " WHERE id = :id";

    q.prepare(sql);
    q.bindValue(":status", status);
    q.bindValue(":akt", aktualna);
    q.bindValue(":doc", docelowa);
    q.bindValue(":ocena", ocena > 0 ? QVariant(ocena) : QVariant(QMetaType::fromType<int>()));
    q.bindValue(":rec", recenzja.trimmed().isEmpty() ? QVariant(QMetaType::fromType<QString>()) : QVariant(recenzja));
    q.bindValue(":id", idPodejscia);
    return q.exec();
}

bool DatabaseManager::usunPodejscie(int idPodejscia) {
    QSqlQuery q(db);
    q.prepare("DELETE FROM podejscia WHERE id = :id");
    q.bindValue(":id", idPodejscia);
    return q.exec();
}

bool DatabaseManager::dodajSesje(int idPodejscia, int przyrost, int sekundy, const QString& notatka) {
    // Dodanie sesji to zawsze dwa kroki w jednej transakcji:
    // 1. INSERT do dziennika_aktywnosci,
    // 2. UPDATE wartosc_aktualna w podejsciu o wynikający przyrost.
    // GREATEST(0, ...) chroni przed zejściem poniżej zera przy ujemnych przyrostach.
    db.transaction();

    QSqlQuery qIns(db);
    qIns.prepare("INSERT INTO dziennik_aktywnosci (id_podejscia, przyrost_jednostek, czas_rozpoczecia, czas_zakonczenia, czas_trwania_sekundy, notatka) "
                 "VALUES (:idP, :przyrost, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, :sekundy, :notatka)");
    qIns.bindValue(":idP",     idPodejscia);
    qIns.bindValue(":przyrost", przyrost);
    qIns.bindValue(":sekundy",  sekundy);
    qIns.bindValue(":notatka",  notatka.trimmed().isEmpty() ? QVariant(QMetaType::fromType<QString>()) : QVariant(notatka));
    if (!qIns.exec()) { db.rollback(); return false; }

    QSqlQuery qUpd(db);
    qUpd.prepare("UPDATE podejscia SET wartosc_aktualna = GREATEST(0, COALESCE(wartosc_aktualna, 0) + :delta) WHERE id = :idP");
    qUpd.bindValue(":delta", przyrost);
    qUpd.bindValue(":idP",   idPodejscia);
    if (!qUpd.exec()) { db.rollback(); return false; }

    db.commit();
    return true;
}

bool DatabaseManager::aktualizujSesje(int idSesji, int przyrost, int sekundy, const QString& notatka) {
    // Edycja sesji wymaga korekty wartosc_aktualna w podejściu.
    // Odczytujemy stary przyrost, liczymy różnicę i aplikujemy ją jako delta — zamiast przeliczać sumę od nowa.
    db.transaction();

    QSqlQuery qSel(db);
    qSel.prepare("SELECT id_podejscia, przyrost_jednostek FROM dziennik_aktywnosci WHERE id = :id");
    qSel.bindValue(":id", idSesji);
    if (!qSel.exec() || !qSel.next()) { db.rollback(); return false; }
    int idPodejscia   = qSel.value(0).toInt();
    int staryPrzyrost = qSel.value(1).toInt();

    QSqlQuery qUpdSes(db);
    qUpdSes.prepare("UPDATE dziennik_aktywnosci SET przyrost_jednostek = :przyrost, czas_trwania_sekundy = :sekundy, notatka = :notatka WHERE id = :id");
    qUpdSes.bindValue(":przyrost", przyrost);
    qUpdSes.bindValue(":sekundy",  sekundy);
    qUpdSes.bindValue(":notatka",  notatka.trimmed().isEmpty() ? QVariant(QMetaType::fromType<QString>()) : QVariant(notatka));
    qUpdSes.bindValue(":id", idSesji);
    if (!qUpdSes.exec()) { db.rollback(); return false; }

    // Aktualizujemy postęp podejścia o różnicę między nową a starą wartością.
    const int roznica = przyrost - staryPrzyrost;
    QSqlQuery qUpdPod(db);
    qUpdPod.prepare("UPDATE podejscia SET wartosc_aktualna = GREATEST(0, COALESCE(wartosc_aktualna, 0) + :delta) WHERE id = :idP");
    qUpdPod.bindValue(":delta", roznica);
    qUpdPod.bindValue(":idP",   idPodejscia);
    if (!qUpdPod.exec()) { db.rollback(); return false; }

    db.commit();
    return true;
}

bool DatabaseManager::usunSesje(int idSesji) {
    // Usunięcie sesji "cofa" jej wkład w postęp podejścia.
    // Odczytujemy przyrost przed DELETE, żeby wiedzieć o ile skorygować wartosc_aktualna.
    db.transaction();

    QSqlQuery qSel(db);
    qSel.prepare("SELECT id_podejscia, przyrost_jednostek FROM dziennik_aktywnosci WHERE id = :id");
    qSel.bindValue(":id", idSesji);
    if (!qSel.exec() || !qSel.next()) { db.rollback(); return false; }
    int idPodejscia = qSel.value(0).toInt();
    int przyrost    = qSel.value(1).toInt();

    QSqlQuery qDel(db);
    qDel.prepare("DELETE FROM dziennik_aktywnosci WHERE id = :id");
    qDel.bindValue(":id", idSesji);
    if (!qDel.exec()) { db.rollback(); return false; }

    QSqlQuery qUpd(db);
    qUpd.prepare("UPDATE podejscia SET wartosc_aktualna = GREATEST(0, COALESCE(wartosc_aktualna, 0) - :delta) WHERE id = :idP");
    qUpd.bindValue(":delta", przyrost);
    qUpd.bindValue(":idP",   idPodejscia);
    if (!qUpd.exec()) { db.rollback(); return false; }

    db.commit();
    return true;
}

bool DatabaseManager::ustawUlubione(int idMedium, bool ulubione) {
    QSqlQuery q(db);
    q.prepare("UPDATE multimedia SET czy_ulubione = :ulubione WHERE id = :id");
    q.bindValue(":ulubione", ulubione);
    q.bindValue(":id", idMedium);
    return q.exec();
}

QVariantMap DatabaseManager::pobierzCiekawostkiStatystyczne() {
    QVariantMap wynik;

    // 1. Maratończyk (Najdłuższa sesja)
    QSqlQuery q1(db);
    q1.prepare(R"(
        SELECT m.tytul, d.czas_trwania_sekundy
        FROM dziennik_aktywnosci d
        JOIN podejscia p ON d.id_podejscia = p.id
        JOIN multimedia m ON p.id_medium = m.id
        ORDER BY d.czas_trwania_sekundy DESC LIMIT 1
    )");
    if (q1.exec() && q1.next()) {
        wynik["rekordSesjaTytul"] = q1.value(0).toString();
        wynik["rekordSesjaCzas"] = q1.value(1).toLongLong();
    }

    // 2. Pożeracz Czasu (Najwięcej czasu łącznie)
    QSqlQuery q2(db);
    q2.prepare(R"(
        SELECT m.tytul, SUM(d.czas_trwania_sekundy) as suma
        FROM dziennik_aktywnosci d
        JOIN podejscia p ON d.id_podejscia = p.id
        JOIN multimedia m ON p.id_medium = m.id
        GROUP BY m.id, m.tytul
        ORDER BY suma DESC LIMIT 1
    )");
    if (q2.exec() && q2.next()) {
        wynik["pozeraczTytul"] = q2.value(0).toString();
        wynik["pozeraczCzas"] = q2.value(1).toLongLong();
    }

    // 3. Syndrom "Jeszcze jednej tury" (Najwięcej podejść)
    QSqlQuery q3(db);
    q3.prepare(R"(
        SELECT m.tytul, MAX(p.numer_podejscia) as max_podejsc
        FROM podejscia p
        JOIN multimedia m ON p.id_medium = m.id
        GROUP BY m.id, m.tytul
        ORDER BY max_podejsc DESC LIMIT 1
    )");
    if (q3.exec() && q3.next()) {
        wynik["uparyTytul"] = q3.value(0).toString();
        wynik["uparyIlosc"] = q3.value(1).toInt();
    }

    // 4. Najbardziej aktywny dzień tygodnia
    QSqlQuery q4(db);
    q4.prepare(R"(
        SELECT EXTRACT(ISODOW FROM czas_rozpoczecia) as dzien, SUM(czas_trwania_sekundy) as suma
        FROM dziennik_aktywnosci
        WHERE czas_rozpoczecia IS NOT NULL
        GROUP BY dzien
        ORDER BY suma DESC LIMIT 1
    )");
    if (q4.exec() && q4.next()) {
        int dzien = q4.value(0).toInt();
        QStringList dni = {"", "Poniedziałek", "Wtorek", "Środa", "Czwartek", "Piątek", "Sobota", "Niedziela"};
        wynik["najlepszyDzien"] = (dzien >= 1 && dzien <= 7) ? dni[dzien] : "Brak danych";
    }

    // 5. Pożeracz twórców (twórca, na którego poszło najwięcej czasu)
    QSqlQuery q5(db);
    q5.prepare(R"(
        SELECT m.tworcy, SUM(d.czas_trwania_sekundy) AS suma
        FROM dziennik_aktywnosci d
        JOIN podejscia p ON d.id_podejscia = p.id
        JOIN multimedia m ON p.id_medium = m.id
        WHERE m.tworcy IS NOT NULL AND m.tworcy <> ''
        GROUP BY m.tworcy
        ORDER BY suma DESC NULLS LAST LIMIT 1
    )");
    if (q5.exec() && q5.next()) {
        wynik["ulubionyTworca"] = q5.value(0).toString();
        wynik["ulubionyTworcaCzas"] = q5.value(1).toLongLong();
    }

    // 6. Dominująca dekada (z którego dziesięciolecia pochodzi najwięcej Twoich pozycji)
    QSqlQuery q6(db);
    q6.prepare(R"(
        SELECT (rok_wydania / 10) * 10 AS dekada, COUNT(*) AS ilosc
        FROM multimedia
        WHERE rok_wydania IS NOT NULL AND rok_wydania > 0
        GROUP BY dekada
        ORDER BY ilosc DESC LIMIT 1
    )");
    if (q6.exec() && q6.next()) {
        wynik["dominujacaDekada"] = q6.value(0).toInt();
        wynik["dominujacaDekadaIlosc"] = q6.value(1).toInt();
    }

    // 7. Wiek nadrabiania (średnie opóźnienie między premierą dzieła a dodaniem go do biblioteki)
    QSqlQuery q7(db);
    q7.prepare(R"(
        SELECT AVG(EXTRACT(YEAR FROM data_dodania) - rok_wydania)
        FROM multimedia
        WHERE rok_wydania IS NOT NULL AND rok_wydania > 0
          AND EXTRACT(YEAR FROM data_dodania) >= rok_wydania
    )");
    if (q7.exec() && q7.next() && !q7.value(0).isNull()) {
        wynik["sredniWiekNadrabiania"] = q7.value(0).toDouble();
    }

    // 8. Pora doby (kiedy w ciągu dnia logujesz najwięcej czasu — "Nocny Marek")
    QSqlQuery q8(db);
    q8.prepare(R"(
        SELECT pora FROM (
            SELECT
                CASE
                    WHEN EXTRACT(HOUR FROM czas_rozpoczecia) BETWEEN 6 AND 11  THEN 'Rano'
                    WHEN EXTRACT(HOUR FROM czas_rozpoczecia) BETWEEN 12 AND 17 THEN 'Popołudnie'
                    WHEN EXTRACT(HOUR FROM czas_rozpoczecia) BETWEEN 18 AND 23 THEN 'Wieczór'
                    ELSE 'Noc'
                END AS pora,
                SUM(czas_trwania_sekundy) AS suma
            FROM dziennik_aktywnosci
            WHERE czas_rozpoczecia IS NOT NULL
            GROUP BY pora
            ORDER BY suma DESC NULLS LAST
            LIMIT 1
        ) AS najaktywniejsza
    )");
    if (q8.exec() && q8.next()) {
        wynik["poraDoby"] = q8.value(0).toString();
    }

    return wynik;
}

QMap<int, QStringList> DatabaseManager::pobierzPrzypisaniaTagow() {
    // Zwraca mapę: id_medium → lista nazw przypisanych tagów.
    // Używane do filtrowania drzewa nawigacji i zaznaczania checkboxów w formularzu.
    QMap<int, QStringList> mapa;
    QSqlQuery q("SELECT mt.id_medium, t.nazwa FROM multimedia_tagi mt JOIN tagi t ON mt.id_tagu = t.id");
    while (q.next()) {
        mapa[q.value(0).toInt()].append(q.value(1).toString());
    }
    return mapa;
}

QList<PodejscieHistoryczne> DatabaseManager::pobierzWszystkieRecenzje() {
    // Zwraca zakończone podejścia z oceną lub recenzją, posortowane od najnowszych.
    QList<PodejscieHistoryczne> lista;
    QSqlQuery q(db);
    q.prepare(R"(
        SELECT p.id, p.id_medium, m.tytul, p.status, p.ocena, p.recenzja, p.data_ukonczenia, p.numer_podejscia
        FROM podejscia p
        JOIN multimedia m ON p.id_medium = m.id
        WHERE p.status IN ('Ukończone', 'Porzucone') AND (p.ocena IS NOT NULL OR p.recenzja IS NOT NULL)
        ORDER BY p.data_ukonczenia DESC
    )");

    if (q.exec()) {
        while (q.next()) {
            PodejscieHistoryczne p;
            p.id          = q.value(0).toInt();
            p.status      = q.value(3).toString();
            p.ocena       = q.value(4).toInt();
            p.tytulMedium = q.value(2).toString();
            p.recenzja    = q.value(5).toString();

            // data_ukonczenia traktujemy jako data_rozpoczecia (PodejscieHistoryczne nie ma osobnego pola).
            p.data_rozpoczecia = q.value(6).toDateTime();

            // Sesje z notatkami — używane jako "oś czasu" w widoku recenzji.
            QSqlQuery qS(db);
            qS.prepare("SELECT czas_rozpoczecia, notatka FROM dziennik_aktywnosci "
                       "WHERE id_podejscia = :id AND notatka IS NOT NULL AND notatka != '' "
                       "ORDER BY czas_rozpoczecia ASC");
            qS.bindValue(":id", p.id);
            if (qS.exec()) {
                while (qS.next()) {
                    Sesja s;
                    s.data    = qS.value(0).toDateTime();
                    s.notatka = qS.value(1).toString();
                    p.sesje.append(s);
                }
            }
            lista.append(p);
        }
    }
    return lista;
}

bool DatabaseManager::ustawTagiDlaMedium(int idMedium, const QList<int>& idTagow) {
    // Pełna wymiana tagów strategią DELETE → INSERT.
    // Zamiast śledzić które tagi dodano/usunięto, kasujemy wszystkie powiązania
    // i wstawiamy nowe. Prosto i bezpiecznie w ramach jednej transakcji.
    db.transaction();

    QSqlQuery qDel(db);
    qDel.prepare("DELETE FROM multimedia_tagi WHERE id_medium = :id");
    qDel.bindValue(":id", idMedium);
    if (!qDel.exec()) { db.rollback(); return false; }

    QSqlQuery qIns(db);
    qIns.prepare("INSERT INTO multimedia_tagi (id_medium, id_tagu) VALUES (:idM, :idT)");
    for (int idT : idTagow) {
        qIns.bindValue(":idM", idMedium);
        qIns.bindValue(":idT", idT);
        if (!qIns.exec()) { db.rollback(); return false; }
    }

    db.commit();
    return true;
}
