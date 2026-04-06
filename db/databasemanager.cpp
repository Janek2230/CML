#include "databasemanager.h"

DatabaseManager::DatabaseManager() {
    db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName("localhost");
    db.setDatabaseName("cml");
    db.setUserName("postgres");
    db.setPassword("Q@wertyuiop");
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


// Zaktualizuj też SELECT, żeby pobierał id_platformy:
QList<std::shared_ptr<Multimedia>> DatabaseManager::getAllMultimedia() {
    QList<std::shared_ptr<Multimedia>> list;
    QSqlQuery query(
        "SELECT m.id, m.tytul, m.status, k.id, k.jednostka, "
        "p.wartosc_aktualna, p.wartosc_docelowa, m.data_dodania, m.id_platformy "
        "FROM multimedia m "
        "LEFT JOIN kategorie k ON m.id_kategorii = k.id "
        "LEFT JOIN postepy p ON m.id = p.id_medium"
        );

    while (query.next()) {
        Postep postep;
        postep.aktualna = query.value(5).isNull() ? 0 : query.value(5).toInt();
        postep.docelowa = query.value(6).isNull() ? 0 : query.value(6).toInt();
        postep.jednostka = query.value(4).toString();

        auto medium = std::make_shared<Multimedia>(
            query.value(0).toInt(),
            query.value(1).toString(),
            query.value(3).toInt(),
            query.value(8).toInt(), // id_platformy
            query.value(2).toString(),
            postep
            );
        medium->setDataDodania(query.value(7).toDateTime());
        list.append(medium);
    }
    return list;
}

// W databasemanager.h dodaj: QMap<QString, int> getGlobalStats();

QMap<QString, int> DatabaseManager::getGlobalStats() {
    QMap<QString, int> stats;
    // Zapytanie, które zlicza wszystko naraz
    QSqlQuery query("SELECT status, count(*) FROM multimedia GROUP BY status");

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

bool DatabaseManager::aktualizujPostep(int idMedium, const QString& status, int aktualna, int docelowa) {
    QSqlQuery query;
    // 1. Aktualizujemy wartości liczbowe
    query.prepare("UPDATE postepy SET wartosc_aktualna = :akt, wartosc_docelowa = :doc WHERE id_medium = :id");
    query.bindValue(":akt", aktualna);
    query.bindValue(":doc", docelowa);
    query.bindValue(":id", idMedium);
    if (!query.exec()) return false;

    // 2. Aktualizujemy status tekstem ("W trakcie", "Ukończone" itd.)
    query.prepare("UPDATE multimedia SET status = :status WHERE id = :id");
    query.bindValue(":status", status);
    query.bindValue(":id", idMedium);
    return query.exec();
}

bool DatabaseManager::zacznijOdNowa(int idMedium) {
    QSqlQuery query;
    // Zerujemy postęp i podbijamy licznik powtórek o 1!
    query.prepare("UPDATE postepy SET wartosc_aktualna = 0, liczba_powtorek = liczba_powtorek + 1 WHERE id_medium = :id");
    query.bindValue(":id", idMedium);
    if (!query.exec()) return false;

    // Wymuszamy zmianę statusu na "W trakcie"
    query.prepare("UPDATE multimedia SET status = 'W trakcie' WHERE id = :id");
    query.bindValue(":id", idMedium);
    return query.exec();
}

// DODAWANIE

// Dodawanie: naprawiony błąd z jednostką i uwzględnione id_platformy
bool DatabaseManager::dodajNoweMedium(const QString &tytul, int idKat, int idPlatformy, int cel) {
    db.transaction();

    QSqlQuery query1(db);
    // Wstawiamy id_platformy zgodnie ze schematem bazy!
    query1.prepare("INSERT INTO multimedia (tytul, id_kategorii, id_platformy, status) VALUES (:tytul, :idKat, :idPlat, 'Planowane') RETURNING id");
    query1.bindValue(":tytul", tytul);
    query1.bindValue(":idKat", idKat);
    query1.bindValue(":idPlat", idPlatformy > 0 ? QVariant(idPlatformy) : QVariant(QVariant::Int)); // Obsługa braku platformy (NULL)

    if (!query1.exec() || !query1.next()) {
        qDebug() << "BŁĄD INSERT MULTIMEDIA:" << query1.lastError().text();
        db.rollback();
        return false;
    }
    int noweId = query1.value(0).toInt();

    QSqlQuery query2(db);
    // Wyrzucono jednostkę! To pole nie istnieje w tej tabeli.
    query2.prepare("INSERT INTO postepy (id_medium, wartosc_aktualna, wartosc_docelowa) VALUES (:id, 0, :cel)");
    query2.bindValue(":id", noweId);
    query2.bindValue(":cel", cel);

    if (!query2.exec()) {
        qDebug() << "BŁĄD INSERT POSTEPY:" << query2.lastError().text();
        db.rollback();
        return false;
    }

    db.commit();
    return true;
}

// USUWANIE
bool DatabaseManager::usunMedium(int id) {
    QSqlQuery query;
    // Najpierw usuwamy postępy (klucz obcy), potem medium
    query.prepare("DELETE FROM postepy WHERE id_medium = :id");
    query.bindValue(":id", id);
    if (!query.exec()) return false;

    query.prepare("DELETE FROM multimedia WHERE id = :id");
    query.bindValue(":id", id);
    return query.exec();
}

QList<QPair<int, QString>> DatabaseManager::pobierzKategorie() {
    QList<QPair<int, QString>> lista;
    // Pobieramy ID i nazwę, sortujemy, żeby "Brak kategorii" (zazwyczaj ID 0 lub 1) był na górze
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

// Podobnie naprawiamy aktualizację
bool DatabaseManager::aktualizujDaneMedium(int id, const QString &tytul, int idKat, int idPlatformy, int cel) {
    db.transaction();
    QSqlQuery query(db);

    query.prepare("UPDATE multimedia SET tytul = :tytul, id_kategorii = :idKat, id_platformy = :idPlat, data_ostatniej_edycji = CURRENT_TIMESTAMP WHERE id = :id");
    query.bindValue(":tytul", tytul);
    query.bindValue(":idKat", idKat);
    query.bindValue(":idPlat", idPlatformy > 0 ? QVariant(idPlatformy) : QVariant(QVariant::Int));
    query.bindValue(":id", id);

    if (!query.exec()) {
        db.rollback();
        return false;
    }

    QSqlQuery query2(db);
    query2.prepare("UPDATE postepy SET wartosc_docelowa = :cel WHERE id_medium = :id");
    query2.bindValue(":cel", cel);
    query2.bindValue(":id", id);

    if (!query2.exec()) {
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
    // Pobieramy tylko unikalne wartości, żeby na liście nie było 50 razy "strony"
    QSqlQuery query("SELECT DISTINCT jednostka FROM kategorie WHERE jednostka IS NOT NULL AND jednostka != ''");

    while (query.next()) {
        lista.append(query.value(0).toString());
    }

    // Jeśli to pusta baza, dajemy pakiet startowy
    if (lista.isEmpty()) {
        lista << "szt." << "strony" << "odcinki" << "seanse" << "godziny";
    }
    return lista;
}

bool DatabaseManager::aktualizujKategorie(int id, const QString &nazwa, const QString &jednostka) {
    QSqlQuery query;
    query.prepare("UPDATE multimedia SET status = :status, data_ostatniej_edycji = CURRENT_TIMESTAMP WHERE id = :id");
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

bool DatabaseManager::usunKategorie(int idKat, bool usunPowiazane) {
    db.transaction();

    if (usunPowiazane) {
        // Usuwamy wszystko: najpierw postępy gier przypisanych do tej kategorii
        QSqlQuery q1(db);
        q1.prepare("DELETE FROM postepy WHERE id_medium IN (SELECT id FROM multimedia WHERE id_kategorii = :id)");
        q1.bindValue(":id", idKat);
        if(!q1.exec()) { db.rollback(); return false; }

        // Następnie usuwamy same gry/książki z tej kategorii
        QSqlQuery q2(db);
        q2.prepare("DELETE FROM multimedia WHERE id_kategorii = :id");
        q2.bindValue(":id", idKat);
        if(!q2.exec()) { db.rollback(); return false; }
    } else {
        // Zostawiamy gry, ale odpinamy je od tej kategorii (ustawiamy NULL)
        QSqlQuery q3(db);
        q3.prepare("UPDATE multimedia SET id_kategorii = NULL WHERE id_kategorii = :id");
        q3.bindValue(":id", idKat);
        if(!q3.exec()) { db.rollback(); return false; }
    }

    // Na samym końcu, gdy nie ma już konfliktów, usuwamy główną kategorię
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
        QSqlQuery q1(db);
        q1.prepare("DELETE FROM postepy WHERE id_medium IN (SELECT id FROM multimedia WHERE id_platformy = :id)");
        q1.bindValue(":id", idPlat);
        if(!q1.exec()) { db.rollback(); return false; }

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

bool DatabaseManager::zmienKategorieWielu(const QList<int>& idMultimediow, int nowaKategoriaId) {
    if (idMultimediow.isEmpty()) return false;

    db.transaction();
    QSqlQuery query(db);
    query.prepare("UPDATE multimedia SET id_kategorii = :idKat WHERE id = :idMed");

    for (int id : idMultimediow) {
        query.bindValue(":idKat", nowaKategoriaId > 0 ? QVariant(nowaKategoriaId) : QVariant(QVariant::Int));
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

bool DatabaseManager::usunWieleMultimediow(const QList<int>& idList) {
    if (idList.isEmpty()) return false;
    db.transaction();

    QSqlQuery qPostepy(db);
    qPostepy.prepare("DELETE FROM postepy WHERE id_medium = :id");

    QSqlQuery qMultimedia(db);
    qMultimedia.prepare("DELETE FROM multimedia WHERE id = :id");

    for (int id : idList) {
        // Usuwamy postępy
        qPostepy.bindValue(":id", id);
        if (!qPostepy.exec()) { db.rollback(); return false; }

        // Usuwamy medium
        qMultimedia.bindValue(":id", id);
        if (!qMultimedia.exec()) { db.rollback(); return false; }
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

QList<int> DatabaseManager::pobierzOstatnioEdytowane(int limit) {
    QList<int> lista;
    QSqlQuery query;
    // Sortujemy po dacie ostatniej edycji, którą dodaliśmy wcześniej
    query.prepare("SELECT id FROM multimedia ORDER BY data_ostatniej_edycji DESC NULLS LAST LIMIT :limit");
    query.bindValue(":limit", limit);
    if (query.exec()) {
        while(query.next()) {
            lista.append(query.value(0).toInt());
        }
    }
    return lista;
}