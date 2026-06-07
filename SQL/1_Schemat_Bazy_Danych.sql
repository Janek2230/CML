-- =====================================================================
-- Schemat bazy danych
-- =====================================================================
--  Ten plik:
--    1) CZYŚCI aktualną bazę (usuwa stare tabele i typ wyliczeniowy),
--    2) TWORZY nowy schemat.
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

-- 2.4 Główna tabela mediów
CREATE TABLE multimedia (
    id SERIAL PRIMARY KEY,
    tytul VARCHAR(255) NOT NULL,
    id_kategorii INTEGER REFERENCES kategorie(id) ON DELETE RESTRICT,
    id_platformy INTEGER REFERENCES platformy(id) ON DELETE RESTRICT,
    data_dodania TIMESTAMP DEFAULT CURRENT_TIMESTAMP, 
    rok_wydania INTEGER,        -- Rok premiery dzieła 
    tworcy VARCHAR(255),        
    czy_ulubione BOOLEAN DEFAULT FALSE
);

-- 2.5 Podejścia
CREATE TABLE podejscia (
    id SERIAL PRIMARY KEY,
    id_medium INTEGER REFERENCES multimedia(id) ON DELETE CASCADE,
    numer_podejscia INTEGER NOT NULL DEFAULT 1,
    status typ_statusu NOT NULL DEFAULT 'Planowane',
    ocena INTEGER CHECK (ocena >= 1 AND ocena <= 10), -- Oceniasz konkretne podejście
    recenzja TEXT,
    wartosc_aktualna INTEGER DEFAULT 0,
    wartosc_docelowa INTEGER,
    data_rozpoczecia TIMESTAMP,
    data_ukonczenia TIMESTAMP,
    UNIQUE(id_medium, numer_podejscia)
);

-- 2.6 Dziennik aktywności 
CREATE TABLE dziennik_aktywnosci (
    id SERIAL PRIMARY KEY,
    id_podejscia INTEGER REFERENCES podejscia(id) ON DELETE CASCADE,
    przyrost_jednostek INTEGER NOT NULL DEFAULT 0,
    czas_rozpoczecia TIMESTAMP,
    czas_zakonczenia TIMESTAMP,
    czas_trwania_sekundy INTEGER,
    data_wpisu TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    notatka TEXT
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
