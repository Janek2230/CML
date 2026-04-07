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

DO $$
DECLARE
    i INT;
    j INT;
    v_medium_id INT;
    v_podejscie_id INT;
    v_data_dodania TIMESTAMP;
    v_data_rozpoczecia TIMESTAMP;
    v_data_ukonczenia TIMESTAMP;
    v_kategoria_id INT;
    v_platforma_id INT;
    v_wartosc_docelowa INT;
    v_status typ_statusu;
    v_obecna_data TIMESTAMP;
    v_przyrost INT;
    v_aktualna_wartosc INT;
BEGIN
    -- Masowe czyszczenie przed symulacją (żegnajcie śmieci)
    TRUNCATE dziennik_aktywnosci, podejscia, multimedia RESTART IDENTITY CASCADE;

    -- Upewniamy się, że masz jakiekolwiek kategorie i platformy, na których skrypt może operować
    INSERT INTO platformy (nazwa, typ_nosnika) VALUES
    ('Steam', 'Cyfrowa'), ('PlayStation 5', 'Fizyczna'), ('Półka', 'Fizyczna'), ('Netflix', 'Cyfrowa')
    ON CONFLICT DO NOTHING;

    INSERT INTO kategorie (nazwa, jednostka) VALUES
    ('Gry Wideo', 'godzin'), ('Książki', 'stron'), ('Seriale', 'odcinków')
    ON CONFLICT DO NOTHING;

    -- Generujemy 50 różnych tytułów
    FOR i IN 1..50 LOOP
        -- Losowa data dodania z ostatnich 3 lat (około 1095 dni wstecz od 2024-05-01)
        v_data_dodania := '2021-05-01 10:00:00'::timestamp + (random() * (interval '1095 days'));
        
        -- Losowanie kategorii i platformy (zakładamy, że ID od 1 do 3 są wolne)
        v_kategoria_id := floor(random() * 3 + 1);
        v_platforma_id := floor(random() * 4 + 1);

        -- Wrzucamy do bazy "twarde" dane
        INSERT INTO multimedia (tytul, id_kategorii, id_platformy, data_dodania)
        VALUES ('Symulowany Tytuł ' || i || ' (' || CASE WHEN v_kategoria_id = 1 THEN 'Gra' WHEN v_kategoria_id = 2 THEN 'Książka' ELSE 'Serial' END || ')', v_kategoria_id, v_platforma_id, v_data_dodania)
        RETURNING id INTO v_medium_id;

        -- Ustawianie sensownych wartości docelowych w zależności od kategorii
        IF v_kategoria_id = 1 THEN v_wartosc_docelowa := floor(random() * 80 + 20); -- Gry: 20-100h
        ELSIF v_kategoria_id = 2 THEN v_wartosc_docelowa := floor(random() * 600 + 200); -- Książki: 200-800 str.
        ELSE v_wartosc_docelowa := floor(random() * 50 + 10); END IF; -- Seriale: 10-60 odc.

        -- Generowanie podejść dla danego medium (od 1 do 2 podejść)
        FOR j IN 1..(floor(random() * 2 + 1)) LOOP
            -- Start podejścia maksymalnie miesiąc po dodaniu do biblioteki
            v_data_rozpoczecia := v_data_dodania + (random() * (interval '30 days'));

            -- Losujemy status życiowy tego podejścia
            IF random() > 0.4 THEN
                v_status := 'Ukończone';
                v_data_ukonczenia := v_data_rozpoczecia + (random() * (interval '60 days'));
                v_aktualna_wartosc := v_wartosc_docelowa;
            ELSIF random() > 0.6 THEN
                v_status := 'Porzucone';
                v_data_ukonczenia := v_data_rozpoczecia + (random() * (interval '15 days'));
                v_aktualna_wartosc := floor(random() * (v_wartosc_docelowa * 0.7)); -- rzucone w 70%
            ELSE
                v_status := 'W trakcie';
                v_data_ukonczenia := NULL;
                v_aktualna_wartosc := floor(random() * (v_wartosc_docelowa * 0.9)); -- w trakcie do 90%
            END IF;

            -- Jeśli to 2 podejście i robimy je teraz, przesuwamy daty, żeby wyglądało na "Zacznij od nowa"
            IF j = 2 THEN
                v_data_rozpoczecia := v_data_rozpoczecia + (interval '365 days');
                IF v_status != 'W trakcie' THEN
                    v_data_ukonczenia := v_data_rozpoczecia + (random() * (interval '60 days'));
                END IF;
            END IF;

            -- Wrzucamy podejście
            INSERT INTO podejscia (id_medium, numer_podejscia, status, ocena, wartosc_aktualna, wartosc_docelowa, data_rozpoczecia, data_ukonczenia)
            VALUES (
                v_medium_id, j, v_status,
                CASE WHEN v_status = 'Ukończone' THEN floor(random() * 5 + 6) ELSE NULL END, -- Dajemy oceny 6-10 tylko ukończonym
                v_aktualna_wartosc, v_wartosc_docelowa, v_data_rozpoczecia, v_data_ukonczenia
            ) RETURNING id INTO v_podejscie_id;

            -- GENEROWANIE HISTORII AKTYWNOŚCI (to zasili Twoje wykresy)
            IF v_status != 'Planowane' AND v_aktualna_wartosc > 0 THEN
                v_obecna_data := v_data_rozpoczecia;
                
                -- Dekrementujemy w pętli zmienną pomocniczą, odtwarzając historię wstecz, a raczej krok po kroku w przód
                DECLARE
                    pozostalo_do_zalogowania INT := v_aktualna_wartosc;
                BEGIN
                    WHILE pozostalo_do_zalogowania > 0 LOOP
                        -- Ile zrobiono podczas tej sesji?
                        IF v_kategoria_id = 1 THEN v_przyrost := floor(random() * 5 + 1); -- 1-5 godzin
                        ELSIF v_kategoria_id = 2 THEN v_przyrost := floor(random() * 60 + 10); -- 10-70 stron
                        ELSE v_przyrost := floor(random() * 4 + 1); END IF; -- 1-4 odcinki

                        -- Zabezpieczenie przed przekroczeniem progu
                        IF v_przyrost > pozostalo_do_zalogowania THEN 
                            v_przyrost := pozostalo_do_zalogowania; 
                        END IF;

                        -- Przesuwamy czas do przodu o kilka dni na kolejną sesję
                        v_obecna_data := v_obecna_data + (random() * (interval '5 days'));

                        -- Jeśli wyjechaliśmy za datę ukończenia, to ją korygujemy
                        IF v_status IN ('Ukończone', 'Porzucone') AND v_obecna_data > v_data_ukonczenia THEN
                            v_obecna_data := v_data_ukonczenia;
                        END IF;

                        -- Logujemy pojedynczy wpis
                        INSERT INTO dziennik_aktywnosci (id_podejscia, data_wpisu, przyrost_jednostek, czas_trwania_sekundy)
                        VALUES (v_podejscie_id, v_obecna_data, v_przyrost, floor(random() * 7200 + 1800));

                        pozostalo_do_zalogowania := pozostalo_do_zalogowania - v_przyrost;
                    END LOOP;
                END;
            END IF;

        END LOOP;
    END LOOP;
END $$;













DO $$
DECLARE
    v_current_day TIMESTAMP;
    v_kategoria_id INT;
    v_platforma_id INT;
    v_wartosc_docelowa INT;
    v_medium_id INT;
    v_podejscie_id INT;
    v_przyrost INT;
    v_czas_trwania INT;
    rec RECORD;
    i INT;
    d INT;
BEGIN
    -- Masowe czyszczenie (zaoranie bazy)
    TRUNCATE dziennik_aktywnosci, podejscia, multimedia RESTART IDENTITY CASCADE;

    -- Upewniamy się, że słowniki istnieją
    INSERT INTO platformy (nazwa, typ_nosnika) VALUES ('Steam', 'Cyfrowa'), ('PlayStation 5', 'Fizyczna'), ('Półka', 'Fizyczna'), ('Netflix', 'Cyfrowa') ON CONFLICT DO NOTHING;
    INSERT INTO kategorie (nazwa, jednostka) VALUES ('Gry Wideo', 'godzin'), ('Książki', 'stron'), ('Seriale', 'odcinków') ON CONFLICT DO NOTHING;

    -- Tworzymy aż 200 tytułów, żeby w ogóle było w co "grać" przez te 1095 dni
    FOR i IN 1..200 LOOP
        v_kategoria_id := floor(random() * 3 + 1);
        v_platforma_id := floor(random() * 4 + 1);
        
        -- Gry rozkładamy losowo na przestrzeni ostatnich 3 lat
        INSERT INTO multimedia (tytul, id_kategorii, id_platformy, data_dodania)
        VALUES ('Tytuł testowy ' || i || ' (' || CASE WHEN v_kategoria_id = 1 THEN 'Gra' WHEN v_kategoria_id = 2 THEN 'Książka' ELSE 'Serial' END || ')', 
                v_kategoria_id, v_platforma_id, (CURRENT_DATE - interval '1095 days') + (random() * interval '1095 days'))
        RETURNING id INTO v_medium_id;
        
        IF v_kategoria_id = 1 THEN v_wartosc_docelowa := floor(random() * 60 + 10);
        ELSIF v_kategoria_id = 2 THEN v_wartosc_docelowa := floor(random() * 500 + 200);
        ELSE v_wartosc_docelowa := floor(random() * 40 + 10); END IF;

        -- Automatycznie "zaczynamy" podejście w dniu dodania do biblioteki
        INSERT INTO podejscia (id_medium, numer_podejscia, status, wartosc_aktualna, wartosc_docelowa, data_rozpoczecia)
        VALUES (v_medium_id, 1, 'W trakcie', 0, v_wartosc_docelowa, (SELECT data_dodania FROM multimedia WHERE id = v_medium_id))
        RETURNING id INTO v_podejscie_id;
    END LOOP;

    -- GŁÓWNA SYMULACJA: Idziemy dzień po dniu przez równe 3 lata (0 do 1095 dni)
    FOR d IN 0..1095 LOOP
        v_current_day := (CURRENT_DATE - interval '1095 days') + (d * interval '1 day');
        
        -- Szansa 90% na to, że danego dnia wykazujesz JAKĄKOLWIEK aktywność
        IF random() < 0.90 THEN
            
            -- Wybieramy losowo od 1 do 3 "aktywnych" tytułów, które już masz w bibliotece w tym konkretnym dniu
            FOR rec IN (
                SELECT p.id, p.wartosc_aktualna, p.wartosc_docelowa, m.id_kategorii 
                FROM podejscia p
                JOIN multimedia m ON p.id_medium = m.id
                WHERE p.status = 'W trakcie' AND p.data_rozpoczecia <= v_current_day
                ORDER BY random()
                LIMIT floor(random() * 3 + 1)
            ) LOOP
                
                -- Liczymy ile "jednostek" dzisiaj zrobiłeś
                IF rec.id_kategorii = 1 THEN 
                    v_przyrost := floor(random() * 4 + 1); -- grasz 1-4 godziny
                    v_czas_trwania := v_przyrost * 3600 + floor(random() * 1800);
                ELSIF rec.id_kategorii = 2 THEN 
                    v_przyrost := floor(random() * 50 + 10); -- czytasz 10-60 stron
                    v_czas_trwania := v_przyrost * 120 + floor(random() * 600);
                ELSE 
                    v_przyrost := floor(random() * 3 + 1); -- oglądasz 1-3 odcinki
                    v_czas_trwania := v_przyrost * 2700 + floor(random() * 900);
                END IF;
                
                -- Dodajemy postęp. Sprawdzamy czy tym razem ukończyłeś tytuł.
                IF rec.wartosc_aktualna + v_przyrost >= rec.wartosc_docelowa THEN
                    v_przyrost := rec.wartosc_docelowa - rec.wartosc_aktualna; -- Wyrównujemy do 100%
                    
                    UPDATE podejscia 
                    SET wartosc_aktualna = wartosc_docelowa, 
                        status = 'Ukończone', 
                        data_ukonczenia = v_current_day + (random() * interval '12 hours'),
                        ocena = floor(random() * 5 + 6) -- Losujemy ocenę 6-10
                    WHERE id = rec.id;
                ELSE
                    UPDATE podejscia 
                    SET wartosc_aktualna = rec.wartosc_aktualna + v_przyrost
                    WHERE id = rec.id;
                END IF;
                
                -- Zapis do Twojego nowego, pięknego dziennika aktywności
                -- Zwróć uwagę, że rozrzucamy logi losowo po godzinach tego samego dnia
                INSERT INTO dziennik_aktywnosci (id_podejscia, data_wpisu, przyrost_jednostek, czas_trwania_sekundy)
                VALUES (rec.id, v_current_day + (random() * interval '23 hours'), v_przyrost, v_czas_trwania);

            END LOOP;
        END IF;
    END LOOP;
    
    -- Symulacja porzucania - niektóre gry po prostu leżą "W trakcie" i o nich zapominasz. Baza to wyłapie.
    UPDATE podejscia
    SET status = 'Porzucone',
        data_ukonczenia = CURRENT_DATE - interval '15 days',
        ocena = floor(random() * 3 + 2) -- Oceny 2-4 dla porzuconych
    WHERE status = 'W trakcie' 
      AND data_rozpoczecia < CURRENT_DATE - interval '100 days'
      AND random() < 0.4; -- Porzucamy 40% takich "zakurzonych" gier

END $$;