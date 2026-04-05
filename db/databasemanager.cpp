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

QList<std::shared_ptr<Multimedia>> DatabaseManager::getAllMultimedia() {
    QList<std::shared_ptr<Multimedia>> list;

    // Nasze potężne zapytanie omija brakujące postępy dzięki LEFT JOIN
    QSqlQuery query(
        "SELECT m.id, m.tytul, m.status, k.id, k.jednostka, "
        "p.wartosc_aktualna, p.wartosc_docelowa "
        "FROM multimedia m "
        "JOIN kategorie k ON m.id_kategorii = k.id "
        "LEFT JOIN postepy p ON m.id = p.id_medium"
        );

    while (query.next()) {
        Postep postep;
        // Zabezpieczenie przed NULL z bazy danych (efekt LEFT JOIN)
        postep.aktualna = query.value(5).isNull() ? 0 : query.value(5).toInt();
        postep.docelowa = query.value(6).isNull() ? 0 : query.value(6).toInt();
        postep.jednostka = query.value(4).toString();

        list.append(std::make_shared<Multimedia>(
            query.value(0).toInt(),     // id
            query.value(1).toString(),  // tytul
            query.value(3).toInt(),     // id_kategorii
            query.value(2).toString(),  // status
            postep
            ));
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
bool DatabaseManager::dodajNoweMedium(const QString &tytul, int idKat, const QString &typ, int cel) {
    QSqlQuery query;
    // 1. Dodajemy do tabeli głównej
    query.prepare("INSERT INTO multimedia (tytul, id_kategorii, status) VALUES (:tytul, :idKat, 'Planowane') RETURNING id");
    query.bindValue(":tytul", tytul);
    query.bindValue(":idKat", idKat);

    if (!query.exec() || !query.next()) return false;
    int noweId = query.value(0).toInt();

    // 2. Dodajemy rekord postępu dla tego ID
    query.prepare("INSERT INTO postepy (id_medium, wartosc_aktualna, wartosc_docelowa, jednostka) "
                  "VALUES (:id, 0, :cel, :typ)");
    query.bindValue(":id", noweId);
    query.bindValue(":cel", cel);
    query.bindValue(":typ", typ);

    return query.exec();
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

QStringList DatabaseManager::pobierzPlatformy() {
    QStringList lista;
    // Wyciągamy tylko unikalne wartości, które już ktoś kiedyś wpisał
    QSqlQuery query("SELECT DISTINCT jednostka FROM postepy WHERE jednostka IS NOT NULL AND jednostka != ''");
    while(query.next()) {
        lista.append(query.value(0).toString());
    }

    // Zabezpieczenie: Jeśli baza jest zupełnie pusta, podajemy standardowe opcje
    if (lista.isEmpty()) {
        lista << "strony" << "odcinki" << "trofea" << "seans";
    }

    return lista;
}