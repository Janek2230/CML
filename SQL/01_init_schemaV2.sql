-- 1. Tabela słownikowa dla platform (zostaje bez zmian)
CREATE TABLE platformy (
    id SERIAL PRIMARY KEY,
    nazwa VARCHAR(100) NOT NULL,
    typ_nosnika VARCHAR(50) NOT NULL
);

-- 2. Tabela słownikowa dla kategorii (definiuje jednostkę logiczną)
CREATE TABLE kategorie (
    id SERIAL PRIMARY KEY,
    nazwa VARCHAR(255) NOT NULL,
    jednostka VARCHAR(50) NOT NULL -- np. 'strony', 'odcinki', '%', 'sztuki'
);

-- 3. Typ wyliczeniowy dla statusu podejścia
CREATE TYPE typ_statusu AS ENUM ('Planowane', 'W trakcie', 'Ukończone', 'Porzucone');

-- 4. Główna tabela mediów (tylko twarde, niezmienne dane)
CREATE TABLE multimedia (
    id SERIAL PRIMARY KEY,
    tytul VARCHAR(255) NOT NULL,
    id_kategorii INTEGER REFERENCES kategorie(id) ON DELETE RESTRICT,
    id_platformy INTEGER REFERENCES platformy(id) ON DELETE RESTRICT,
    sciezka_okladki VARCHAR(500),
    data_dodania TIMESTAMP DEFAULT CURRENT_TIMESTAMP -- Do liczenia "leżakowania"
);

-- 5. Podejścia (Serce logiki postępu - tu przenosimy statusy i oceny)
CREATE TABLE podejscia (
    id SERIAL PRIMARY KEY,
    id_medium INTEGER REFERENCES multimedia(id) ON DELETE CASCADE,
    numer_podejscia INTEGER NOT NULL DEFAULT 1,
    status typ_statusu NOT NULL DEFAULT 'Planowane',
    ocena INTEGER CHECK (ocena >= 1 AND ocena <= 10), -- Oceniasz konkretne podejście
    wartosc_aktualna INTEGER DEFAULT 0,
    wartosc_docelowa INTEGER NOT NULL, -- Wpisujesz ręcznie przy dodawaniu podejścia
    data_rozpoczecia TIMESTAMP, -- Ustawiana z poziomu kodu Qt, gdy status zmienia się na 'W trakcie'
    data_ukonczenia TIMESTAMP,  -- Ustawiana z poziomu kodu Qt, gdy status zmienia się na 'Ukończone'/'Porzucone'
    UNIQUE(id_medium, numer_podejscia) -- Nie można mieć dwóch podejść nr 1 do tej samej gry
);

-- 6. Dziennik aktywności (Tylko proste logowanie zdarzeń do wykresów)
CREATE TABLE dziennik_aktywnosci (
    id SERIAL PRIMARY KEY,
    id_podejscia INTEGER REFERENCES podejscia(id) ON DELETE CASCADE,
    data_wpisu TIMESTAMP DEFAULT CURRENT_TIMESTAMP, -- Kiedy dodano postęp
    przyrost_jednostek INTEGER NOT NULL, -- Ile dodano (np. +10, +1)
    czas_trwania_sekundy INTEGER DEFAULT 0 -- Czas trwania sesji w sekundach, do wykresów czasu spędzonego
);