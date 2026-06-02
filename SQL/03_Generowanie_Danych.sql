BEGIN;

-- Czyszczenie starych danych (jeśli istnieją), żebyś nie płakał, że duplikują Ci się klucze.
TRUNCATE TABLE dziennik_aktywnosci, podejscia, multimedia_tagi, tagi, multimedia, kategorie, platformy RESTART IDENTITY CASCADE;

DO $$
DECLARE
    -- Iteratory i ograniczenia czasowe
    v_date TIMESTAMP;
    v_end_date TIMESTAMP := CURRENT_TIMESTAMP;
    v_start_date TIMESTAMP := v_end_date - interval '3 years';
    v_month TIMESTAMP;
    v_items_added INT;
    
    -- Zmienne dla rekordów i przypisań
    v_cat_rand INT;
    v_plat INT;
    v_tytul VARCHAR;
    v_docelowa INT;
    v_data_dodania TIMESTAMP;
    new_med_id INT;
    v_attempts INT;
    
    -- Zmienne dla symulacji "Time Machine"
    v_num_sessions INT;
    v_active_count INT;
    v_selected_planowane INT[];
    v_selected_w_trakcie INT[];
    v_pid INT;
    v_czas_rozpoczecia TIMESTAMP;
    v_czas_zakonczenia TIMESTAMP;
    v_trwania INT;
    v_przyrost INT;
    v_aktualna INT;
    v_base_time TIMESTAMP;
    
    -- Słowniki w tablicach
    v_tytuly_gry TEXT[] := ARRAY['Wiedźmin 3', 'Cyberpunk 2077', 'Baldurs Gate 3', 'Elden Ring', 'Hades', 'Factorio', 'Civilization VI', 'Doom Eternal', 'RimWorld', 'Stardew Valley', 'Disco Elysium'];
    v_tytuly_ksiazki TEXT[] := ARRAY['Diuna', 'Fundacja', 'Problem trzech ciał', 'Ślepowidzenie', 'Hyperion', 'Władca Pierścieni', 'Silmarillion', '1984', 'Zbrodnia i kara', 'Lód', 'Neuromancer'];
    v_tytuly_seriale TEXT[] := ARRAY['Breaking Bad', 'The Wire', 'Sopranos', 'Czarnobyl', 'Sukcesja', 'Dark', 'Stranger Things', 'The Office', 'Bojack Horseman', 'Arcane', 'Fargo'];
    v_tytuly_filmy TEXT[] := ARRAY['Skazani na Shawshank', 'Matrix', 'Incepcja', 'Pulp Fiction', 'Mroczny Rycerz', 'Interstellar', 'Gladiator', 'Chłopcy z ferajny', 'Forrest Gump', 'Blade Runner'];

    -- Twórcy (autorzy / reżyserzy / studia) losowani per kategoria — zasilają statystykę "Pożeracz Twórców".
    v_tworcy VARCHAR;
    v_tworcy_gry TEXT[]     := ARRAY['CD Projekt RED', 'FromSoftware', 'Larian Studios', 'Supergiant Games', 'Valve', 'Rockstar Games'];
    v_tworcy_ksiazki TEXT[] := ARRAY['Frank Herbert', 'Isaac Asimov', 'Liu Cixin', 'Stanisław Lem', 'Andrzej Sapkowski', 'J.R.R. Tolkien'];
    v_tworcy_seriale TEXT[] := ARRAY['HBO', 'Netflix', 'Vince Gilligan', 'AMC', 'BBC'];
    v_tworcy_filmy TEXT[]   := ARRAY['Christopher Nolan', 'Quentin Tarantino', 'Ridley Scott', 'Stanley Kubrick', 'Denis Villeneuve'];
BEGIN

    -- 1. Inicjalizacja słowników - przypisuję sztywne ID dla pewności przy relacjach
    INSERT INTO kategorie (id, nazwa, jednostka) VALUES
    (1, 'Gry', 'procent'),
    (2, 'Książki', 'stron'),
    (3, 'Seriale', 'odcinków'),
    (4, 'Filmy', 'minut');

    INSERT INTO platformy (id, nazwa, typ_nosnika) VALUES
    (1, 'PC', 'Cyfrowy'),
    (2, 'PlayStation 5', 'Płyta Blu-ray'),
    (3, 'Książka papierowa', 'Papier'),
    (4, 'Kindle', 'E-book'),
    (5, 'Netflix', 'Streaming'),
    (6, 'HBO Max', 'Streaming'),
    (7, 'Kino', 'Taśma/Cyfrowe');

    INSERT INTO tagi (id, nazwa) VALUES 
    (1, 'RPG'), (2, 'Sci-Fi'), (3, 'Fantasy'), (4, 'Komedia'), (5, 'Dramat'), (6, 'Akcja');

    -- 2. Generowanie multimediów - krok po kroku, 3-8 na miesiąc
    FOR v_month IN SELECT generate_series(
        date_trunc('month', v_start_date),
        date_trunc('month', v_end_date),
        '1 month'::interval
    ) LOOP
        v_items_added := floor(random() * 6 + 3); -- od 3 do 8 pozycji na miesiąc
        
        FOR i IN 1..v_items_added LOOP
            v_cat_rand := floor(random() * 4 + 1); 
            
            -- Determinowanie parametrów zależnych od kategorii
            IF v_cat_rand = 1 THEN
                v_plat := CASE WHEN random() > 0.5 THEN 1 ELSE 2 END;
                v_tytul := v_tytuly_gry[floor(random() * array_length(v_tytuly_gry, 1) + 1)];
                v_tworcy := v_tworcy_gry[floor(random() * array_length(v_tworcy_gry, 1) + 1)];
                v_docelowa := 100;
            ELSIF v_cat_rand = 2 THEN
                v_plat := CASE WHEN random() > 0.5 THEN 3 ELSE 4 END;
                v_tytul := v_tytuly_ksiazki[floor(random() * array_length(v_tytuly_ksiazki, 1) + 1)];
                v_tworcy := v_tworcy_ksiazki[floor(random() * array_length(v_tworcy_ksiazki, 1) + 1)];
                v_docelowa := floor(random() * 700 + 200);
            ELSIF v_cat_rand = 3 THEN
                v_plat := CASE WHEN random() > 0.5 THEN 5 ELSE 6 END;
                v_tytul := v_tytuly_seriale[floor(random() * array_length(v_tytuly_seriale, 1) + 1)];
                v_tworcy := v_tworcy_seriale[floor(random() * array_length(v_tworcy_seriale, 1) + 1)];
                v_docelowa := floor(random() * 40 + 10);
            ELSE
                v_plat := CASE WHEN random() > 0.7 THEN 7 ELSE 5 END;
                v_tytul := v_tytuly_filmy[floor(random() * array_length(v_tytuly_filmy, 1) + 1)];
                v_tworcy := v_tworcy_filmy[floor(random() * array_length(v_tworcy_filmy, 1) + 1)];
                v_docelowa := floor(random() * 120 + 90);
            END IF;

            -- Data dodania (losowy dzień z wylosowanego miesiąca, z blokadą na przyszłość)
            v_data_dodania := LEAST(v_month + (random() * 27 * interval '1 day'), v_end_date);
            
            INSERT INTO multimedia (tytul, id_kategorii, id_platformy, data_dodania, rok_wydania, tworcy)
            VALUES (v_tytul, v_cat_rand, v_plat, v_data_dodania, floor(random() * 24 + 2000), v_tworcy)
            RETURNING id INTO new_med_id;
            
            -- Losowe tagi (1-3 sztuki)
            FOR t IN 1..(floor(random() * 3 + 1)) LOOP
                INSERT INTO multimedia_tagi (id_medium, id_tagu)
                VALUES (new_med_id, floor(random() * 6 + 1)) ON CONFLICT DO NOTHING;
            END LOOP;
            
            -- Generowanie zarysów podejść (50% ma 2 lub 3 podejścia)
            v_attempts := CASE WHEN random() > 0.5 THEN floor(random() * 2 + 2) ELSE 1 END; 
            FOR a IN 1..v_attempts LOOP
                INSERT INTO podejscia (id_medium, numer_podejscia, status, wartosc_docelowa)
                VALUES (new_med_id, a, 'Planowane', v_docelowa);
            END LOOP;
        END LOOP;
    END LOOP;

    -- 3. Wehikuł Czasu - Generowanie absolutnie chronologicznej i logicznej aktywności
    FOR v_date IN SELECT generate_series(
        v_start_date,
        v_end_date,
        '1 day'::interval
    ) LOOP
        
        -- Zliczamy ile użytkownik ma aktualnie rozgrzebanych spraw
        SELECT COUNT(*) INTO v_active_count FROM podejscia WHERE status = 'W trakcie';

        -- A. Uruchamianie podejść o statusie 'Planowane' (tylko gdy mamy na to "moce przerobowe")
        IF v_active_count < 4 AND random() < 0.15 THEN
            v_selected_planowane := ARRAY(
                SELECT p.id 
                FROM podejscia p
                JOIN multimedia m ON m.id = p.id_medium
                WHERE p.status = 'Planowane' 
                  AND m.data_dodania <= v_date + interval '8 hours'
                  AND NOT EXISTS (
                      SELECT 1 FROM podejscia p2 
                      WHERE p2.id_medium = p.id_medium 
                        AND p2.numer_podejscia < p.numer_podejscia 
                        AND p2.status != 'Ukończone'
                  )
                ORDER BY random() 
                LIMIT 1
            );
            
            IF coalesce(array_length(v_selected_planowane, 1), 0) > 0 THEN
                UPDATE podejscia 
                SET status = 'W trakcie', 
                    data_rozpoczecia = v_date + interval '10 hours' + (random() * interval '2 hours')
                WHERE id = ANY(v_selected_planowane);
            END IF;
        END IF;

        -- B. Logowanie codziennych sesji (0-3 podejścia na dzień)
        v_num_sessions := floor(random() * 4); 
        IF v_num_sessions > 0 THEN
            v_selected_w_trakcie := ARRAY(
                SELECT id FROM podejscia 
                WHERE status = 'W trakcie'
                ORDER BY random() 
                LIMIT v_num_sessions
            );
            
            IF coalesce(array_length(v_selected_w_trakcie, 1), 0) > 0 THEN
                v_base_time := v_date + interval '16 hours'; 
                
                FOREACH v_pid IN ARRAY v_selected_w_trakcie
                LOOP
                    v_trwania := floor(random() * (14400 - 1800 + 1) + 1800); -- Od 30 min do 4h
                    v_czas_rozpoczecia := v_base_time;
                    v_czas_zakonczenia := v_czas_rozpoczecia + (v_trwania || ' seconds')::interval;
                    
                    v_base_time := v_czas_zakonczenia + interval '15 minutes'; 
                    
                    SELECT wartosc_aktualna, wartosc_docelowa INTO v_aktualna, v_docelowa FROM podejscia WHERE id = v_pid;
                    
                    v_przyrost := ceil(v_docelowa * (0.1 + random() * 0.3));
                    IF v_aktualna + v_przyrost >= v_docelowa THEN
                        v_przyrost := v_docelowa - v_aktualna;
                    END IF;
                    
                    INSERT INTO dziennik_aktywnosci (id_podejscia, przyrost_jednostek, czas_rozpoczecia, czas_zakonczenia, czas_trwania_sekundy, data_wpisu)
                    VALUES (v_pid, v_przyrost, v_czas_rozpoczecia, v_czas_zakonczenia, v_trwania, v_czas_zakonczenia + interval '2 minutes');
                    
                    UPDATE podejscia 
                    SET wartosc_aktualna = wartosc_aktualna + v_przyrost
                    WHERE id = v_pid;
                    
                    IF v_aktualna + v_przyrost = v_docelowa THEN
                        UPDATE podejscia
                        SET status = 'Ukończone',
                            data_ukonczenia = v_czas_zakonczenia,
                            ocena = floor(random() * 10 + 1) 
                        WHERE id = v_pid;
                    END IF;
                END LOOP;
            END IF;
        END IF;
        
        -- C. Losowe zamrażanie i odmrażanie 'Wstrzymane' (żeby w dashboardzie nie było nudno)
        UPDATE podejscia SET status = 'Wstrzymane' WHERE status = 'W trakcie' AND random() < 0.015;
        UPDATE podejscia SET status = 'W trakcie' WHERE status = 'Wstrzymane' AND random() < 0.05;

        -- D. Brutalne porzucanie (Rage-quit logic)
        -- 0.5% szans dziennie, że pozycja, z którą się męczysz lub którą wstrzymałeś, wyląduje w śmieciach.
        UPDATE podejscia
        SET status = 'Porzucone',
            data_ukonczenia = v_date + interval '20 hours'
        WHERE status IN ('W trakcie', 'Wstrzymane') AND random() < 0.005;

    END LOOP;
END $$;

-- 4. Ustawienie sekwencji na sam koniec
SELECT setval('platformy_id_seq', (SELECT COALESCE(MAX(id), 1) FROM platformy));
SELECT setval('kategorie_id_seq', (SELECT COALESCE(MAX(id), 1) FROM kategorie));
SELECT setval('multimedia_id_seq', (SELECT COALESCE(MAX(id), 1) FROM multimedia));
SELECT setval('podejscia_id_seq', (SELECT COALESCE(MAX(id), 1) FROM podejscia));
SELECT setval('dziennik_aktywnosci_id_seq', (SELECT COALESCE(MAX(id), 1) FROM dziennik_aktywnosci));
SELECT setval('tagi_id_seq', (SELECT COALESCE(MAX(id), 1) FROM tagi));

COMMIT;