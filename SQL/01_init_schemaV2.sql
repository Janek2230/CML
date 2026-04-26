-- 1. Zrzucamy bombę na domyślny schemat (usuwa wszystko: tabele, widoki, relacje, typy ENUM)
DROP SCHEMA public CASCADE;

-- 2. Odbudowujemy pusty schemat
CREATE SCHEMA public;

-- 3. Przywracamy uprawnienia (żeby baza znowu wpuszczała Twoją aplikację na standardowych prawach)
GRANT ALL ON SCHEMA public TO postgres;
GRANT ALL ON SCHEMA public TO public;

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
    jednostka VARCHAR(50) -- np. 'strony', 'odcinki', '%', 'sztuki'
);

-- 3. Typ wyliczeniowy dla statusu podejścia
CREATE TYPE typ_statusu AS ENUM ('Planowane', 'W trakcie', 'Wstrzymane', 'Ukończone', 'Porzucone');

-- 4. Główna tabela mediów (tylko twarde, niezmienne dane)
CREATE TABLE multimedia (
    id SERIAL PRIMARY KEY,
    tytul VARCHAR(255) NOT NULL,
    id_kategorii INTEGER REFERENCES kategorie(id) ON DELETE RESTRICT,
    id_platformy INTEGER REFERENCES platformy(id) ON DELETE RESTRICT,
    sciezka_okladki VARCHAR(500),
    data_dodania TIMESTAMP DEFAULT CURRENT_TIMESTAMP, -- Do liczenia "leżakowania"
    rok_wydania INTEGER,
    tworcy VARCHAR(255),

    czy_ulubione BOOLEAN DEFAULT FALSE
);

-- 5. Podejścia (Serce logiki postępu - tu przenosimy statusy i oceny)
CREATE TABLE podejscia (
    id SERIAL PRIMARY KEY,
    id_medium INTEGER REFERENCES multimedia(id) ON DELETE CASCADE,
    numer_podejscia INTEGER NOT NULL DEFAULT 1,
    status typ_statusu NOT NULL DEFAULT 'Planowane',
    ocena INTEGER CHECK (ocena >= 1 AND ocena <= 10), -- Oceniasz konkretne podejście
    recenzja TEXT,
    wartosc_aktualna INTEGER DEFAULT 0,
    wartosc_docelowa INTEGER, -- Wpisujesz ręcznie przy dodawaniu podejścia
    data_rozpoczecia TIMESTAMP, -- Ustawiana z poziomu kodu Qt, gdy status zmienia się na 'W trakcie'
    data_ukonczenia TIMESTAMP,  -- Ustawiana z poziomu kodu Qt, gdy status zmienia się na 'Ukończone'/'Porzucone'
    UNIQUE(id_medium, numer_podejscia) -- Nie można mieć dwóch podejść nr 1 do tej samej gry
    
);

-- 6. Dziennik aktywności (Tylko proste logowanie zdarzeń do wykresów)
CREATE TABLE dziennik_aktywnosci (
    id SERIAL PRIMARY KEY,
    id_podejscia INTEGER REFERENCES podejscia(id) ON DELETE CASCADE,
    przyrost_jednostek INTEGER NOT NULL DEFAULT 0,
    czas_rozpoczecia TIMESTAMP, -- Moment kliknięcia "Start" w stoperze
    czas_zakonczenia TIMESTAMP, -- Moment kliknięcia "Stop"
    
    -- Można zostawić czas w sekundach wyliczany triggerem z różnicy czasów, 
    -- albo wpisywany ręcznie, jeśli użytkownik dodaje wpis z palca bez stopera.
    czas_trwania_sekundy INTEGER, 
    data_wpisu TIMESTAMP DEFAULT CURRENT_TIMESTAMP, -- Kiedy wpis został dodany do bazy
    
    notatka TEXT -- Opcjonalny komentarz do sesji (np. "Pokonałem trudnego bossa", "Nudny rozdział")
);

-- Słownik tagów (np. 'RPG', 'Sci-Fi', 'Fantasy', 'Komedia', 'Co-op')
CREATE TABLE tagi (
    id SERIAL PRIMARY KEY,
    nazwa VARCHAR(50) UNIQUE NOT NULL
);

-- Tabela łącząca multimedia z tagami
CREATE TABLE multimedia_tagi (
    id_medium INTEGER REFERENCES multimedia(id) ON DELETE CASCADE,
    id_tagu INTEGER REFERENCES tagi(id) ON DELETE CASCADE,
    PRIMARY KEY (id_medium, id_tagu)
);

