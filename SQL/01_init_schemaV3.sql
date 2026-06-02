-- =====================================================================
--  CML — Schemat bazy danych, wersja V3 (czysta)  [PostgreSQL]
-- =====================================================================
--  Ten plik:
--    1) CZYŚCI aktualną bazę (usuwa stare tabele i typ wyliczeniowy),
--    2) TWORZY nowy schemat.
--
--  W stosunku do V2 usunięto JEDNĄ martwą kolumnę:
--    - multimedia.sciezka_okladki   (okładki nigdzie nie wyświetlane)
--
--  Pozostałe kolumny zostają, bo dostały realne zastosowanie:
--    - multimedia.rok_wydania       → podpis w szczegółach + statystyki
--    - multimedia.tworcy            → podpis w szczegółach + "pożeracz twórców"
--    - platformy.typ_nosnika        → grupowanie biblioteki wg typu nośnika
--    - dziennik_aktywnosci.data_wpisu → pole audytowe (kiedy zalogowano wpis)
--
--  Uruchomienie:  psql -U postgres -d cml -f 01_init_schemaV3.sql
-- =====================================================================

BEGIN;

-- ---------------------------------------------------------------------
-- 1. Czyszczenie poprzedniej bazy
--    Kolejność i CASCADE rozwiązują zależności kluczy obcych.
-- ---------------------------------------------------------------------
DROP TABLE IF EXISTS multimedia_tagi      CASCADE;
DROP TABLE IF EXISTS dziennik_aktywnosci  CASCADE;
DROP TABLE IF EXISTS podejscia            CASCADE;
DROP TABLE IF EXISTS tagi                 CASCADE;
DROP TABLE IF EXISTS multimedia           CASCADE;
DROP TABLE IF EXISTS kategorie            CASCADE;
DROP TABLE IF EXISTS platformy            CASCADE;

DROP TYPE  IF EXISTS typ_statusu;

-- ---------------------------------------------------------------------
-- 2. Nowy schemat
-- ---------------------------------------------------------------------

-- 2.1 Tabela słownikowa dla platform
CREATE TABLE platformy (
    id SERIAL PRIMARY KEY,
    nazwa VARCHAR(100) NOT NULL,
    typ_nosnika VARCHAR(50) NOT NULL -- np. 'Cyfrowy', 'Streaming', 'Papier', 'E-book'
);

-- 2.2 Tabela słownikowa dla kategorii (definiuje jednostkę logiczną)
CREATE TABLE kategorie (
    id SERIAL PRIMARY KEY,
    nazwa VARCHAR(255) NOT NULL,
    jednostka VARCHAR(50) -- np. 'strony', 'odcinki', '%', 'sztuki'
);

-- 2.3 Typ wyliczeniowy dla statusu podejścia
CREATE TYPE typ_statusu AS ENUM ('Planowane', 'W trakcie', 'Wstrzymane', 'Ukończone', 'Porzucone');

-- 2.4 Główna tabela mediów (tylko twarde, niezmienne dane)
--     (usunięto sciezka_okladki — okładki nigdy nie były wyświetlane)
CREATE TABLE multimedia (
    id SERIAL PRIMARY KEY,
    tytul VARCHAR(255) NOT NULL,
    id_kategorii INTEGER REFERENCES kategorie(id) ON DELETE RESTRICT,
    id_platformy INTEGER REFERENCES platformy(id) ON DELETE RESTRICT,
    data_dodania TIMESTAMP DEFAULT CURRENT_TIMESTAMP, -- Do liczenia "leżakowania"
    rok_wydania INTEGER,        -- Rok premiery dzieła (do statystyk "wieku nadrabiania")
    tworcy VARCHAR(255),        -- Autor / reżyser / studio
    czy_ulubione BOOLEAN DEFAULT FALSE
);

-- 2.5 Podejścia (serce logiki postępu — statusy i oceny)
CREATE TABLE podejscia (
    id SERIAL PRIMARY KEY,
    id_medium INTEGER REFERENCES multimedia(id) ON DELETE CASCADE,
    numer_podejscia INTEGER NOT NULL DEFAULT 1,
    status typ_statusu NOT NULL DEFAULT 'Planowane',
    ocena INTEGER CHECK (ocena >= 1 AND ocena <= 10), -- Oceniasz konkretne podejście
    recenzja TEXT,
    wartosc_aktualna INTEGER DEFAULT 0,
    wartosc_docelowa INTEGER, -- Wpisywane ręcznie przy dodawaniu podejścia
    data_rozpoczecia TIMESTAMP, -- Ustawiana z kodu Qt, gdy status zmienia się na 'W trakcie'
    data_ukonczenia TIMESTAMP,  -- Ustawiana z kodu Qt, gdy status zmienia się na 'Ukończone'/'Porzucone'
    UNIQUE(id_medium, numer_podejscia) -- Nie można mieć dwóch podejść nr 1 do tej samej gry
);

-- 2.6 Dziennik aktywności (proste logowanie zdarzeń do wykresów)
CREATE TABLE dziennik_aktywnosci (
    id SERIAL PRIMARY KEY,
    id_podejscia INTEGER REFERENCES podejscia(id) ON DELETE CASCADE,
    przyrost_jednostek INTEGER NOT NULL DEFAULT 0,
    czas_rozpoczecia TIMESTAMP, -- Moment kliknięcia "Start" w stoperze
    czas_zakonczenia TIMESTAMP, -- Moment kliknięcia "Stop"
    czas_trwania_sekundy INTEGER, -- Liczony z różnicy czasów albo wpisywany ręcznie
    data_wpisu TIMESTAMP DEFAULT CURRENT_TIMESTAMP, -- Audyt: kiedy wpis trafił do bazy
    notatka TEXT -- Opcjonalny komentarz do sesji (np. "Pokonałem trudnego bossa")
);

-- 2.7 Słownik tagów (np. 'RPG', 'Sci-Fi', 'Fantasy', 'Komedia', 'Co-op')
CREATE TABLE tagi (
    id SERIAL PRIMARY KEY,
    nazwa VARCHAR(50) UNIQUE NOT NULL
);

-- 2.8 Tabela łącząca multimedia z tagami
CREATE TABLE multimedia_tagi (
    id_medium INTEGER REFERENCES multimedia(id) ON DELETE CASCADE,
    id_tagu INTEGER REFERENCES tagi(id) ON DELETE CASCADE,
    PRIMARY KEY (id_medium, id_tagu)
);

COMMIT;
