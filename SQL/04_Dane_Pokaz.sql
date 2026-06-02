-- =====================================================================
--  CML — Realistyczny seed pod pokaz  [PostgreSQL]
-- =====================================================================
--  Symuluje rok korzystania z aplikacji (ostatnie 12 miesięcy, licząc od dziś).
--  ~52 realne tytuły (gry/książki/seriale/filmy) z prawdziwymi twórcami i latami.
--
--  Wymusza logikę odczytaną z kodu:
--   * tylko NAJNOWSZE podejście bywa aktywne — starsze są zawsze terminalne
--     (Ukończone/Porzucone z datą zakończenia),
--   * wartosc_aktualna podejścia == suma przyrostów jego sesji,
--   * czas_zakonczenia = czas_rozpoczecia + czas_trwania_sekundy,
--   * spójność statusu i dat (Planowane bez dat/sesji, W trakcie bez daty końca itd.),
--   * godziny sesji z przewagą wieczoru/nocy (pod statystykę "Nocny Marek"),
--   * recenzje = lorem ipsum o zmiennej długości.
--
--  URUCHAMIAĆ PO 01_init_schemaV3.sql (czysty schemat V3).
--    psql -U postgres -d cml -f 01_init_schemaV3.sql
--    psql -U postgres -d cml -f 04_Dane_Pokaz.sql
-- =====================================================================

BEGIN;
SET client_encoding TO 'UTF8';

TRUNCATE TABLE dziennik_aktywnosci, podejscia, multimedia_tagi, tagi, multimedia, kategorie, platformy
    RESTART IDENTITY CASCADE;

-- ---------------------------------------------------------------------
-- 1. Słowniki (stałe ID, sekwencje ustawiane na końcu)
-- ---------------------------------------------------------------------
INSERT INTO kategorie (id, nazwa, jednostka) VALUES
    (1, 'Gry',     'procent'),
    (2, 'Książki', 'stron'),
    (3, 'Seriale', 'odcinków'),
    (4, 'Filmy',   'minut');

INSERT INTO platformy (id, nazwa, typ_nosnika) VALUES
    (1, 'PC',                'Cyfrowy'),
    (2, 'PlayStation 5',     'Fizyczny'),
    (3, 'Nintendo Switch',   'Fizyczny'),
    (4, 'Książka papierowa', 'Papier'),
    (5, 'Kindle',            'E-book'),
    (6, 'Netflix',           'Streaming'),
    (7, 'HBO Max',           'Streaming'),
    (8, 'Kino',              'Fizyczny');

INSERT INTO tagi (id, nazwa) VALUES
    (1, 'RPG'), (2, 'Sci-Fi'), (3, 'Fantasy'), (4, 'Dramat'), (5, 'Akcja'),
    (6, 'Komedia'), (7, 'Kryminał'), (8, 'Horror'), (9, 'Przygodowe'), (10, 'Klasyka');

-- ---------------------------------------------------------------------
-- 2. Funkcje pomocnicze (usuwane na końcu skryptu)
-- ---------------------------------------------------------------------

-- Buduje recenzję z 1..N losowych zdań lorem ipsum — daje naturalną zmienność długości.
CREATE OR REPLACE FUNCTION losuj_recenzje(p_min INT, p_max INT) RETURNS TEXT AS $$
DECLARE
    v_zdania TEXT[] := ARRAY[
        'Lorem ipsum dolor sit amet, consectetur adipiscing elit.',
        'Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.',
        'Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip.',
        'Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore.',
        'Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt.',
        'Quis autem vel eum iure reprehenderit qui in ea voluptate velit esse quam nihil.'
    ];
    v_ile INT := p_min + floor(random() * (p_max - p_min + 1))::INT;
    v_wynik TEXT := '';
    i INT;
BEGIN
    FOR i IN 1..v_ile LOOP
        v_wynik := v_wynik || CASE WHEN i > 1 THEN ' ' ELSE '' END
                   || v_zdania[1 + floor(random() * array_length(v_zdania, 1))::INT];
    END LOOP;
    RETURN v_wynik;
END;
$$ LANGUAGE plpgsql;

-- Tworzy jedno podejście wraz z sesjami. Pilnuje, że suma przyrostów == p_postep.
--   p_start    — data rozpoczęcia (NULL = Planowane, brak sesji),
--   p_koniec   — data_ukonczenia (NULL dla aktywnych/wstrzymanych),
--   p_ostatnia — górna granica czasu sesji (np. "ostatnia aktywność" dla Wstrzymane),
--   p_postep   — docelowa wartosc_aktualna tego podejścia.
CREATE OR REPLACE FUNCTION seed_podejscie(
    p_med_id   INT,
    p_numer    INT,
    p_status   TEXT,
    p_cel      INT,
    p_start    TIMESTAMP,
    p_koniec   TIMESTAMP,
    p_postep   INT,
    p_ocena    INT,
    p_recenzja TEXT,
    p_ostatnia TIMESTAMP,
    p_kat      INT
) RETURNS VOID AS $$
DECLARE
    v_pid INT;
    v_sesje INT;
    v_base INT;
    v_przyrost INT;
    v_suma INT := 0;
    v_span INTERVAL;
    v_ostatnia TIMESTAMP;
    v_czas_start TIMESTAMP;
    v_czas_koniec TIMESTAMP;
    v_trwanie INT;
    v_hour INT;
    v_notatka TEXT;
    v_notatki TEXT[] := ARRAY[
        'Lorem ipsum dolor sit amet.',
        'Consectetur adipiscing elit, sed do eiusmod.',
        'Świetna sesja, wciągnęło mnie na dłużej.',
        'Ut labore et dolore magna aliqua.',
        'Trochę się dłużyło, ale było warto.',
        'Duis aute irure dolor in reprehenderit.'
    ];
    i INT;
BEGIN
    INSERT INTO podejscia (id_medium, numer_podejscia, status, ocena, recenzja,
                           wartosc_aktualna, wartosc_docelowa, data_rozpoczecia, data_ukonczenia)
    VALUES (p_med_id, p_numer, p_status::typ_statusu,
            CASE WHEN p_ocena > 0 THEN p_ocena ELSE NULL END,
            CASE WHEN p_recenzja <> '' THEN p_recenzja ELSE NULL END,
            0, p_cel, p_start, p_koniec)
    RETURNING id INTO v_pid;

    -- Planowane (lub bez postępu) — żadnych sesji.
    IF p_start IS NULL OR p_postep <= 0 THEN
        RETURN;
    END IF;

    v_ostatnia := COALESCE(p_ostatnia, p_koniec, p_start + interval '1 day');
    IF v_ostatnia <= p_start THEN
        v_ostatnia := p_start + interval '6 hours';
    END IF;

    -- Filmy = jeden seans; reszta 2..9 sesji, ale nie więcej niż jest jednostek do zdobycia.
    v_sesje := CASE WHEN p_kat = 4 THEN 1 ELSE GREATEST(2, LEAST(9, 2 + floor(random() * 7)::INT)) END;
    v_sesje := LEAST(v_sesje, GREATEST(1, p_postep));
    v_base  := floor(p_postep::numeric / v_sesje)::INT;
    v_span  := v_ostatnia - p_start;

    FOR i IN 1..v_sesje LOOP
        IF i = v_sesje THEN
            v_przyrost := p_postep - v_suma;   -- reszta domyka sumę dokładnie do p_postep
        ELSE
            v_przyrost := v_base;
        END IF;
        IF v_przyrost < 0 THEN v_przyrost := 0; END IF;
        v_suma := v_suma + v_przyrost;

        -- Rozkład sesji w oknie podejścia + przesunięcie godziny na wieczór/noc.
        v_czas_start := p_start + v_span * ((i - 0.5)::double precision / v_sesje);
        v_hour := (ARRAY[18,19,20,21,22,23,0,1,21,22,20,15,16,13])[1 + floor(random() * 14)::INT];
        v_czas_start := date_trunc('day', v_czas_start)
                        + make_interval(hours => v_hour, mins => floor(random() * 60)::INT);
        IF v_czas_start < p_start    THEN v_czas_start := p_start; END IF;
        IF v_czas_start > v_ostatnia THEN v_czas_start := v_ostatnia - interval '2 hours'; END IF;

        -- Film: czas trwania ≈ długość filmu (minuty→sekundy); reszta: 30 min – 4 h.
        v_trwanie := CASE WHEN p_kat = 4 THEN GREATEST(1, p_postep) * 60
                          ELSE 1800 + floor(random() * 12600)::INT END;
        v_czas_koniec := v_czas_start + make_interval(secs => v_trwanie);

        v_notatka := CASE WHEN random() < 0.4
                          THEN v_notatki[1 + floor(random() * array_length(v_notatki, 1))::INT]
                          ELSE NULL END;

        INSERT INTO dziennik_aktywnosci (id_podejscia, przyrost_jednostek, czas_rozpoczecia, czas_zakonczenia,
                                         czas_trwania_sekundy, data_wpisu, notatka)
        VALUES (v_pid, v_przyrost, v_czas_start, v_czas_koniec, v_trwanie,
                v_czas_koniec + interval '3 minutes', v_notatka);
    END LOOP;

    UPDATE podejscia SET wartosc_aktualna = p_postep WHERE id = v_pid;
END;
$$ LANGUAGE plpgsql;

-- ---------------------------------------------------------------------
-- 3. Definicje mediów (tytuł, kategoria, platforma, rok, twórca, cel,
--    scenariusz, ulubione, tagi). Scenariusz steruje liczbą i statusem podejść:
--      U=Ukończone, PO=Porzucone, WT=W trakcie, WS=Wstrzymane,
--      P=Planowane (świeże), PS=Planowane stare (>180 dni, "kupka wstydu"),
--      PU=Porzucone→Ukończone, UU=Ukończone→Ukończone (powtórka),
--      UW=Ukończone→W trakcie (ponowne podejście), PPW=Porzucone→Porzucone→W trakcie.
-- ---------------------------------------------------------------------
CREATE TEMP TABLE seed_def (
    tytul     TEXT,
    id_kat    INT,
    id_plat   INT,
    rok       INT,
    tworcy    TEXT,
    cel       INT,
    scenario  TEXT,
    ulubione  BOOLEAN,
    tags      TEXT[]
) ON COMMIT DROP;

INSERT INTO seed_def (tytul, id_kat, id_plat, rok, tworcy, cel, scenario, ulubione, tags) VALUES
-- ----- GRY (cel = % ukończenia) -----
('Wiedźmin 3: Dziki Gon',     1, 1, 2015, 'CD Projekt RED',        100, 'UU',  TRUE,  ARRAY['RPG','Fantasy','Akcja']),
('Cyberpunk 2077',            1, 1, 2020, 'CD Projekt RED',        100, 'PU',  TRUE,  ARRAY['RPG','Sci-Fi','Akcja']),
('Baldur''s Gate 3',          1, 1, 2023, 'Larian Studios',        100, 'U',   TRUE,  ARRAY['RPG','Fantasy','Przygodowe']),
('Elden Ring',                1, 2, 2022, 'FromSoftware',          100, 'PPW', TRUE,  ARRAY['RPG','Akcja','Fantasy']),
('Hades',                     1, 3, 2020, 'Supergiant Games',      100, 'U',   FALSE, ARRAY['RPG','Akcja']),
('Red Dead Redemption 2',     1, 2, 2018, 'Rockstar Games',        100, 'WT',  FALSE, ARRAY['Przygodowe','Akcja']),
('The Last of Us Part II',    1, 2, 2020, 'Naughty Dog',           100, 'U',   FALSE, ARRAY['Akcja','Dramat']),
('Disco Elysium',             1, 1, 2019, 'ZA/UM',                 100, 'WS',  FALSE, ARRAY['RPG','Kryminał']),
('Hollow Knight',             1, 3, 2017, 'Team Cherry',           100, 'PO',  FALSE, ARRAY['Przygodowe','Akcja']),
('Stardew Valley',            1, 1, 2016, 'ConcernedApe',          100, 'WT',  FALSE, ARRAY['Przygodowe']),
('God of War',                1, 2, 2018, 'Santa Monica Studio',   100, 'U',   TRUE,  ARRAY['Akcja','Przygodowe']),
('Portal 2',                  1, 1, 2011, 'Valve',                 100, 'U',   FALSE, ARRAY['Przygodowe','Komedia']),
-- ----- KSIĄŻKI (cel = liczba stron) -----
('Diuna',                     2, 4, 1965, 'Frank Herbert',         720, 'U',   TRUE,  ARRAY['Sci-Fi','Klasyka']),
('Problem trzech ciał',       2, 5, 2008, 'Liu Cixin',            400, 'U',   FALSE, ARRAY['Sci-Fi']),
('Fundacja',                  2, 4, 1951, 'Isaac Asimov',          250, 'U',   FALSE, ARRAY['Sci-Fi','Klasyka']),
('Drużyna Pierścienia',       2, 4, 1954, 'J.R.R. Tolkien',        480, 'UU',  TRUE,  ARRAY['Fantasy','Klasyka','Przygodowe']),
('1984',                      2, 5, 1949, 'George Orwell',         320, 'U',   FALSE, ARRAY['Sci-Fi','Dramat','Klasyka']),
('Zbrodnia i kara',           2, 4, 1866, 'Fiodor Dostojewski',    670, 'WS',  FALSE, ARRAY['Dramat','Klasyka']),
('Ostatnie życzenie',         2, 4, 1993, 'Andrzej Sapkowski',     330, 'U',   TRUE,  ARRAY['Fantasy','Przygodowe']),
('Hyperion',                  2, 5, 1989, 'Dan Simmons',           500, 'WT',  FALSE, ARRAY['Sci-Fi']),
('Lód',                       2, 4, 2007, 'Jacek Dukaj',          1050, 'PS',  FALSE, ARRAY['Sci-Fi','Klasyka']),
('Solaris',                   2, 5, 1961, 'Stanisław Lem',         220, 'U',   FALSE, ARRAY['Sci-Fi','Klasyka']),
('Neuromancer',               2, 5, 1984, 'William Gibson',        270, 'PO',  FALSE, ARRAY['Sci-Fi']),
('Mistrz i Małgorzata',       2, 4, 1967, 'Michaił Bułhakow',      480, 'U',   FALSE, ARRAY['Dramat','Klasyka']),
('Droga Królów',              2, 5, 2010, 'Brandon Sanderson',    1010, 'WT',  TRUE,  ARRAY['Fantasy','Przygodowe']),
('Imię wiatru',               2, 4, 2007, 'Patrick Rothfuss',      660, 'U',   TRUE,  ARRAY['Fantasy']),
('Cień wiatru',               2, 5, 2001, 'Carlos Ruiz Zafón',     480, 'P',   FALSE, ARRAY['Dramat','Kryminał']),
('Sto lat samotności',        2, 4, 1967, 'Gabriel García Márquez', 430, 'PU', FALSE, ARRAY['Dramat','Klasyka']),
-- ----- SERIALE (cel = liczba odcinków) -----
('Breaking Bad',              3, 6, 2008, 'Vince Gilligan',         62, 'U',   TRUE,  ARRAY['Dramat','Kryminał']),
('Czarnobyl',                 3, 7, 2019, 'HBO',                     5, 'U',   FALSE, ARRAY['Dramat','Klasyka']),
('The Wire',                  3, 7, 2002, 'David Simon',            60, 'WS',  FALSE, ARRAY['Dramat','Kryminał']),
('Sukcesja',                  3, 7, 2018, 'HBO',                    39, 'U',   FALSE, ARRAY['Dramat','Komedia']),
('Dark',                      3, 6, 2017, 'Netflix',                26, 'U',   TRUE,  ARRAY['Sci-Fi','Dramat']),
('Arcane',                    3, 6, 2021, 'Fortiche',               18, 'U',   TRUE,  ARRAY['Akcja','Fantasy']),
('Better Call Saul',          3, 6, 2015, 'Vince Gilligan',         63, 'WT',  FALSE, ARRAY['Dramat','Kryminał']),
('Fleabag',                   3, 7, 2016, 'BBC',                    12, 'U',   FALSE, ARRAY['Komedia','Dramat']),
('Ted Lasso',                 3, 6, 2020, 'Apple',                  34, 'PO',  FALSE, ARRAY['Komedia','Dramat']),
('The Last of Us',            3, 7, 2023, 'HBO',                     9, 'U',   FALSE, ARRAY['Dramat','Akcja']),
('Mr. Robot',                 3, 6, 2015, 'Sam Esmail',             45, 'UW',  FALSE, ARRAY['Dramat','Kryminał']),
('True Detective',            3, 7, 2014, 'HBO',                     8, 'U',   FALSE, ARRAY['Kryminał','Dramat']),
-- ----- FILMY (cel = długość w minutach) -----
('Incepcja',                  4, 8, 2010, 'Christopher Nolan',     148, 'U',   TRUE,  ARRAY['Sci-Fi','Akcja']),
('Interstellar',              4, 8, 2014, 'Christopher Nolan',     169, 'UU',  TRUE,  ARRAY['Sci-Fi','Dramat']),
('Pulp Fiction',              4, 6, 1994, 'Quentin Tarantino',     154, 'U',   FALSE, ARRAY['Kryminał','Dramat','Klasyka']),
('Blade Runner 2049',         4, 7, 2017, 'Denis Villeneuve',      164, 'U',   FALSE, ARRAY['Sci-Fi','Dramat']),
('Diuna (2021)',              4, 8, 2021, 'Denis Villeneuve',      155, 'U',   TRUE,  ARRAY['Sci-Fi','Przygodowe']),
('Skazani na Shawshank',      4, 7, 1994, 'Frank Darabont',        142, 'U',   FALSE, ARRAY['Dramat','Klasyka']),
('Parasite',                  4, 6, 2019, 'Bong Joon-ho',          132, 'U',   FALSE, ARRAY['Dramat','Kryminał']),
('Matrix',                    4, 6, 1999, 'The Wachowskis',        136, 'U',   TRUE,  ARRAY['Sci-Fi','Akcja','Klasyka']),
('Gladiator',                 4, 8, 2000, 'Ridley Scott',          155, 'U',   FALSE, ARRAY['Akcja','Dramat','Klasyka']),
('Whiplash',                  4, 6, 2014, 'Damien Chazelle',       106, 'U',   FALSE, ARRAY['Dramat']),
('Oppenheimer',               4, 8, 2023, 'Christopher Nolan',     180, 'U',   TRUE,  ARRAY['Dramat','Klasyka']),
('Lśnienie',                  4, 7, 1980, 'Stanley Kubrick',       146, 'P',   FALSE, ARRAY['Horror','Klasyka']);

-- ---------------------------------------------------------------------
-- 4. Generowanie mediów, tagów, podejść i sesji
-- ---------------------------------------------------------------------
DO $$
DECLARE
    r RECORD;
    v_med INT;
    v_now TIMESTAMP := CURRENT_TIMESTAMP;
    v_added TIMESTAMP;
    v_dni INT;
    v_dur INT;
    v_s1 TIMESTAMP; v_e1 TIMESTAMP;
    v_s2 TIMESTAMP; v_e2 TIMESTAMP;
    v_s3 TIMESTAMP;
    v_ostatnia TIMESTAMP;
    v_postep INT;
    v_tag TEXT;
    v_tag_id INT;
BEGIN
    FOR r IN SELECT * FROM seed_def ORDER BY tytul LOOP

        -- Ile dni temu pozycja trafiła do biblioteki (zależnie od scenariusza).
        v_dni := CASE r.scenario
            WHEN 'PS'  THEN 185 + floor(random() * 120)::INT
            WHEN 'P'   THEN   5 + floor(random() * 150)::INT
            WHEN 'WT'  THEN  30 + floor(random() * 120)::INT
            WHEN 'WS'  THEN  90 + floor(random() * 150)::INT
            WHEN 'PU'  THEN 220 + floor(random() * 120)::INT
            WHEN 'UU'  THEN 220 + floor(random() * 120)::INT
            WHEN 'UW'  THEN 200 + floor(random() * 130)::INT
            WHEN 'PPW' THEN 250 + floor(random() * 100)::INT
            ELSE             40 + floor(random() * 280)::INT   -- U, PO
        END;
        v_added := v_now - make_interval(days => v_dni);

        INSERT INTO multimedia (tytul, id_kategorii, id_platformy, data_dodania, rok_wydania, tworcy, czy_ulubione)
        VALUES (r.tytul, r.id_kat, r.id_plat, v_added, r.rok, r.tworcy, r.ulubione)
        RETURNING id INTO v_med;

        -- Tagi
        FOREACH v_tag IN ARRAY r.tags LOOP
            SELECT id INTO v_tag_id FROM tagi WHERE nazwa = v_tag;
            IF v_tag_id IS NOT NULL THEN
                INSERT INTO multimedia_tagi (id_medium, id_tagu) VALUES (v_med, v_tag_id) ON CONFLICT DO NOTHING;
            END IF;
        END LOOP;

        -- Typowa długość pojedynczego podejścia (dni) wg kategorii.
        v_dur := CASE r.id_kat
                    WHEN 1 THEN 21 + floor(random() * 35)::INT   -- gry
                    WHEN 2 THEN 14 + floor(random() * 28)::INT   -- książki
                    WHEN 3 THEN 10 + floor(random() * 28)::INT   -- seriale
                    ELSE 0 END;                                  -- filmy (jeden seans)

        IF r.scenario = 'U' THEN
            v_s1 := v_added + interval '3 days';
            v_e1 := LEAST(v_s1 + make_interval(days => v_dur), v_now - interval '2 days');
            PERFORM seed_podejscie(v_med, 1, 'Ukończone', r.cel, v_s1, v_e1, r.cel,
                                   6 + floor(random() * 5)::INT, losuj_recenzje(2, 4), v_e1, r.id_kat);

        ELSIF r.scenario = 'PO' THEN
            v_s1 := v_added + interval '3 days';
            v_e1 := LEAST(v_s1 + make_interval(days => GREATEST(2, (v_dur * 0.6)::INT)), v_now - interval '2 days');
            v_postep := GREATEST(1, floor(r.cel * (0.2 + random() * 0.45))::INT);
            PERFORM seed_podejscie(v_med, 1, 'Porzucone', r.cel, v_s1, v_e1, v_postep,
                                   CASE WHEN random() < 0.6 THEN 2 + floor(random() * 4)::INT ELSE 0 END,
                                   CASE WHEN random() < 0.6 THEN losuj_recenzje(1, 2) ELSE '' END,
                                   v_e1, r.id_kat);

        ELSIF r.scenario = 'WT' THEN
            v_s1 := v_added + interval '3 days';
            v_ostatnia := v_now - make_interval(days => 2 + floor(random() * 12)::INT);
            IF v_ostatnia <= v_s1 THEN v_ostatnia := v_s1 + interval '10 days'; END IF;
            v_postep := GREATEST(1, floor(r.cel * (0.25 + random() * 0.5))::INT);
            PERFORM seed_podejscie(v_med, 1, 'W trakcie', r.cel, v_s1, NULL, v_postep, 0, '', v_ostatnia, r.id_kat);

        ELSIF r.scenario = 'WS' THEN
            v_s1 := v_added + interval '3 days';
            v_ostatnia := v_now - make_interval(days => 35 + floor(random() * 100)::INT);
            IF v_ostatnia <= v_s1 THEN v_ostatnia := v_s1 + interval '10 days'; END IF;
            v_postep := GREATEST(1, floor(r.cel * (0.2 + random() * 0.45))::INT);
            PERFORM seed_podejscie(v_med, 1, 'Wstrzymane', r.cel, v_s1, NULL, v_postep, 0, '', v_ostatnia, r.id_kat);

        ELSIF r.scenario IN ('P', 'PS') THEN
            PERFORM seed_podejscie(v_med, 1, 'Planowane', r.cel, NULL, NULL, 0, 0, '', NULL, r.id_kat);

        ELSIF r.scenario = 'PU' THEN
            v_s1 := v_added + interval '3 days';
            v_e1 := v_s1 + make_interval(days => GREATEST(2, (v_dur * 0.5)::INT));
            v_postep := GREATEST(1, floor(r.cel * (0.2 + random() * 0.35))::INT);
            PERFORM seed_podejscie(v_med, 1, 'Porzucone', r.cel, v_s1, v_e1, v_postep,
                                   3 + floor(random() * 3)::INT, losuj_recenzje(1, 2), v_e1, r.id_kat);
            v_s2 := v_e1 + make_interval(days => 15 + floor(random() * 30)::INT);
            v_e2 := LEAST(v_s2 + make_interval(days => GREATEST(2, v_dur)), v_now - interval '2 days');
            IF v_e2 <= v_s2 THEN v_e2 := v_s2 + interval '5 days'; END IF;
            PERFORM seed_podejscie(v_med, 2, 'Ukończone', r.cel, v_s2, v_e2, r.cel,
                                   7 + floor(random() * 4)::INT, losuj_recenzje(2, 4), v_e2, r.id_kat);

        ELSIF r.scenario = 'UU' THEN
            v_s1 := v_added + interval '3 days';
            v_e1 := v_s1 + make_interval(days => GREATEST(2, (v_dur * 0.6)::INT));
            PERFORM seed_podejscie(v_med, 1, 'Ukończone', r.cel, v_s1, v_e1, r.cel,
                                   7 + floor(random() * 4)::INT, losuj_recenzje(2, 3), v_e1, r.id_kat);
            v_s2 := v_e1 + make_interval(days => 20 + floor(random() * 40)::INT);
            v_e2 := LEAST(v_s2 + make_interval(days => GREATEST(2, (v_dur * 0.6)::INT)), v_now - interval '2 days');
            IF v_e2 <= v_s2 THEN v_e2 := v_s2 + interval '5 days'; END IF;
            PERFORM seed_podejscie(v_med, 2, 'Ukończone', r.cel, v_s2, v_e2, r.cel,
                                   7 + floor(random() * 4)::INT, losuj_recenzje(1, 3), v_e2, r.id_kat);

        ELSIF r.scenario = 'UW' THEN
            v_s1 := v_added + interval '3 days';
            v_e1 := v_s1 + make_interval(days => GREATEST(2, (v_dur * 0.6)::INT));
            PERFORM seed_podejscie(v_med, 1, 'Ukończone', r.cel, v_s1, v_e1, r.cel,
                                   7 + floor(random() * 4)::INT, losuj_recenzje(2, 4), v_e1, r.id_kat);
            v_s2 := v_e1 + make_interval(days => 20 + floor(random() * 40)::INT);
            v_ostatnia := v_now - make_interval(days => 2 + floor(random() * 12)::INT);
            IF v_ostatnia <= v_s2 THEN v_ostatnia := v_s2 + interval '10 days'; END IF;
            v_postep := GREATEST(1, floor(r.cel * (0.25 + random() * 0.5))::INT);
            PERFORM seed_podejscie(v_med, 2, 'W trakcie', r.cel, v_s2, NULL, v_postep, 0, '', v_ostatnia, r.id_kat);

        ELSIF r.scenario = 'PPW' THEN
            v_s1 := v_added + interval '2 days';
            v_e1 := v_s1 + make_interval(days => GREATEST(2, (v_dur * 0.4)::INT));
            PERFORM seed_podejscie(v_med, 1, 'Porzucone', r.cel, v_s1, v_e1,
                                   GREATEST(1, floor(r.cel * (0.15 + random() * 0.2))::INT),
                                   2 + floor(random() * 3)::INT, losuj_recenzje(1, 1), v_e1, r.id_kat);
            v_s2 := v_e1 + make_interval(days => 12 + floor(random() * 25)::INT);
            v_e2 := v_s2 + make_interval(days => GREATEST(2, (v_dur * 0.4)::INT));
            PERFORM seed_podejscie(v_med, 2, 'Porzucone', r.cel, v_s2, v_e2,
                                   GREATEST(1, floor(r.cel * (0.2 + random() * 0.25))::INT),
                                   3 + floor(random() * 3)::INT, losuj_recenzje(1, 2), v_e2, r.id_kat);
            v_s3 := v_e2 + make_interval(days => 12 + floor(random() * 20)::INT);
            v_ostatnia := v_now - make_interval(days => 2 + floor(random() * 12)::INT);
            IF v_ostatnia <= v_s3 THEN v_ostatnia := v_s3 + interval '8 days'; END IF;
            v_postep := GREATEST(1, floor(r.cel * (0.25 + random() * 0.4))::INT);
            PERFORM seed_podejscie(v_med, 3, 'W trakcie', r.cel, v_s3, NULL, v_postep, 0, '', v_ostatnia, r.id_kat);
        END IF;

    END LOOP;
END $$;

-- ---------------------------------------------------------------------
-- 5. Sekwencje słowników (multimedia/podejscia/dziennik dostały ID z SERIAL)
-- ---------------------------------------------------------------------
SELECT setval('kategorie_id_seq', (SELECT MAX(id) FROM kategorie));
SELECT setval('platformy_id_seq', (SELECT MAX(id) FROM platformy));
SELECT setval('tagi_id_seq',      (SELECT MAX(id) FROM tagi));

-- 6. Sprzątanie funkcji pomocniczych
DROP FUNCTION seed_podejscie(INT, INT, TEXT, INT, TIMESTAMP, TIMESTAMP, INT, INT, TEXT, TIMESTAMP, INT);
DROP FUNCTION losuj_recenzje(INT, INT);

COMMIT;
