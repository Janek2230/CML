BEGIN;

-- 1. CZYSZCZENIE BAZY
TRUNCATE TABLE dziennik_aktywnosci, podejscia, multimedia_tagi, tagi, multimedia, platformy, kategorie RESTART IDENTITY CASCADE;

-- 2. SŁOWNIKI
INSERT INTO platformy (nazwa, typ_nosnika) VALUES 
('Steam', 'Cyfrowe'), ('PlayStation 5', 'Konsola'), ('Netflix', 'VOD'), 
('Kindle', 'E-book'), ('Książka fizyczna', 'Papier'), ('Kino / TV', 'Ekran');

INSERT INTO kategorie (nazwa, jednostka) VALUES 
('Gra (Fabuła)', '%'),             -- 1
('Gra (Platyna)', 'osiągnięcia'),  -- 2
('Serial', 'odcinki'),             -- 3
('Książka', 'strony'),             -- 4
('Książka', 'rozdziały'),          -- 5
('Film', 'sztuki');                -- 6

INSERT INTO tagi (nazwa) VALUES 
('Sci-Fi'), ('Fantasy'), ('RPG'), ('Komedia'), ('Dramat'), ('Akcja');

-- =========================================================
-- 3. MULTIMEDIA (36 pozycji - po 3 na miesiąc od "dzisiaj")
-- =========================================================
INSERT INTO multimedia (id, tytul, id_kategorii, id_platformy, rok_wydania, czy_ulubione, data_dodania) VALUES
-- MIESIĄC 1 (Startujemy od dzisiaj)
(1, 'Cyberpunk 2077', 1, 1, 2020, FALSE, CURRENT_TIMESTAMP),
(2, 'Diuna', 4, 5, 1965, TRUE, CURRENT_TIMESTAMP),
(3, 'Interstellar', 6, 6, 2014, TRUE, CURRENT_TIMESTAMP),
-- MIESIĄC 2
(4, 'Wiedźmin 3', 2, 2, 2015, TRUE, CURRENT_TIMESTAMP + interval '1 month'),
(5, 'Breaking Bad', 3, 3, 2008, TRUE, CURRENT_TIMESTAMP + interval '1 month'),
(6, 'Matrix', 6, 6, 1999, FALSE, CURRENT_TIMESTAMP + interval '1 month'),
-- MIESIĄC 3
(7, 'Hades', 1, 1, 2020, FALSE, CURRENT_TIMESTAMP + interval '2 months'),
(8, 'Atomowe Nawyki', 5, 4, 2018, TRUE, CURRENT_TIMESTAMP + interval '2 months'),
(9, 'Diuna: Część Druga', 6, 6, 2024, TRUE, CURRENT_TIMESTAMP + interval '2 months'),
-- MIESIĄC 4
(10, 'Baldurs Gate 3', 2, 1, 2023, TRUE, CURRENT_TIMESTAMP + interval '3 months'),
(11, 'The Office', 3, 3, 2005, TRUE, CURRENT_TIMESTAMP + interval '3 months'),
(12, 'Incepcja', 6, 6, 2010, FALSE, CURRENT_TIMESTAMP + interval '3 months'),
-- MIESIĄC 5
(13, 'Red Dead Redemption 2', 1, 2, 2018, TRUE, CURRENT_TIMESTAMP + interval '4 months'),
(14, 'Problem Trzech Ciał', 4, 5, 2008, FALSE, CURRENT_TIMESTAMP + interval '4 months'),
(15, 'Gladiator', 6, 6, 2000, TRUE, CURRENT_TIMESTAMP + interval '4 months'),
-- MIESIĄC 6
(16, 'Elden Ring', 2, 2, 2022, FALSE, CURRENT_TIMESTAMP + interval '5 months'),
(17, 'Stranger Things', 3, 3, 2016, FALSE, CURRENT_TIMESTAMP + interval '5 months'),
(18, 'Blade Runner 2049', 6, 6, 2017, TRUE, CURRENT_TIMESTAMP + interval '5 months'),
-- MIESIĄC 7
(19, 'Stardew Valley', 1, 1, 2016, TRUE, CURRENT_TIMESTAMP + interval '6 months'),
(20, 'Władca Pierścieni', 4, 5, 1954, TRUE, CURRENT_TIMESTAMP + interval '6 months'),
(21, 'Joker', 6, 6, 2019, FALSE, CURRENT_TIMESTAMP + interval '6 months'),
-- MIESIĄC 8
(22, 'Hollow Knight', 2, 1, 2017, TRUE, CURRENT_TIMESTAMP + interval '7 months'),
(23, 'Gra o Tron', 3, 3, 2011, FALSE, CURRENT_TIMESTAMP + interval '7 months'),
(24, 'Avengers', 6, 6, 2012, FALSE, CURRENT_TIMESTAMP + interval '7 months'),
-- MIESIĄC 9
(25, 'Ghost of Tsushima', 1, 2, 2020, TRUE, CURRENT_TIMESTAMP + interval '8 months'),
(26, 'Fundacja', 5, 4, 1951, FALSE, CURRENT_TIMESTAMP + interval '8 months'),
(27, 'Batman', 6, 6, 2022, TRUE, CURRENT_TIMESTAMP + interval '8 months'),
-- MIESIĄC 10
(28, 'Sekiro', 2, 2, 2019, FALSE, CURRENT_TIMESTAMP + interval '9 months'),
(29, 'Arcane', 3, 3, 2021, TRUE, CURRENT_TIMESTAMP + interval '9 months'),
(30, 'Oppehneimer', 6, 6, 2023, TRUE, CURRENT_TIMESTAMP + interval '9 months'),
-- MIESIĄC 11
(31, 'Disco Elysium', 1, 1, 2019, TRUE, CURRENT_TIMESTAMP + interval '10 months'),
(32, 'Clean Code', 5, 4, 2008, FALSE, CURRENT_TIMESTAMP + interval '10 months'),
(33, 'Deadpool', 6, 6, 2016, TRUE, CURRENT_TIMESTAMP + interval '10 months'),
-- MIESIĄC 12
(34, 'Gothic', 2, 1, 2001, TRUE, CURRENT_TIMESTAMP + interval '11 months'),
(35, 'The Boys', 3, 3, 2019, TRUE, CURRENT_TIMESTAMP + interval '11 months'),
(36, 'Shrek', 6, 6, 2001, TRUE, CURRENT_TIMESTAMP + interval '11 months');

-- =========================================================
-- 4. PODEJŚCIA
-- =========================================================
INSERT INTO podejscia (id, id_medium, status, wartosc_aktualna, wartosc_docelowa, ocena, recenzja, data_rozpoczecia) VALUES
-- MIESIĄC 1
(1, 1, 'Porzucone', 35, 100, 5, 'Bugi zepsuły mi sejwa.', CURRENT_TIMESTAMP),
(2, 2, 'Ukończone', 600, 600, 9, 'Kawał dobrej sci-fi.', CURRENT_TIMESTAMP),
(3, 3, 'Ukończone', 1, 1, 10, 'Niesamowite kino.', CURRENT_TIMESTAMP),
-- MIESIĄC 2
(4, 4, 'W trakcie', 25, 78, NULL, NULL, CURRENT_TIMESTAMP + interval '1 month'),
(5, 5, 'Ukończone', 62, 62, 10, 'Arcydzieło telewizji.', CURRENT_TIMESTAMP + interval '1 month'),
(6, 6, 'Ukończone', 1, 1, 8, 'Klasyk.', CURRENT_TIMESTAMP + interval '1 month'),
-- MIESIĄC 3
(7, 7, 'W trakcie', 60, 100, NULL, NULL, CURRENT_TIMESTAMP + interval '2 months'),
(8, 8, 'Ukończone', 20, 20, 9, 'Bardzo przydatne na co dzień.', CURRENT_TIMESTAMP + interval '2 months'),
(9, 9, 'Ukończone', 1, 1, 9, NULL, CURRENT_TIMESTAMP + interval '2 months'),
-- MIESIĄC 4
(10, 10, 'Wstrzymane', 15, 54, 8, 'Za długie na teraz.', CURRENT_TIMESTAMP + interval '3 months'),
(11, 11, 'W trakcie', 120, 201, NULL, NULL, CURRENT_TIMESTAMP + interval '3 months'),
(12, 12, 'Ukończone', 1, 1, 9, NULL, CURRENT_TIMESTAMP + interval '3 months'),
-- MIESIĄC 5
(13, 13, 'Ukończone', 100, 100, 10, 'Najlepsza historia w grach.', CURRENT_TIMESTAMP + interval '4 months'),
(14, 14, 'W trakcie', 150, 450, NULL, NULL, CURRENT_TIMESTAMP + interval '4 months'),
(15, 15, 'Ukończone', 1, 1, 8, NULL, CURRENT_TIMESTAMP + interval '4 months'),
-- MIESIĄC 6
(16, 16, 'Planowane', 0, 42, NULL, NULL, NULL),
(17, 17, 'Wstrzymane', 12, 34, 6, 'Znudziło mi się na 2 sezonie.', CURRENT_TIMESTAMP + interval '5 months'),
(18, 18, 'Ukończone', 1, 1, 10, NULL, CURRENT_TIMESTAMP + interval '5 months'),
-- MIESIĄC 7
(19, 19, 'W trakcie', 80, 100, NULL, NULL, CURRENT_TIMESTAMP + interval '6 months'),
(20, 20, 'Planowane', 0, 1200, NULL, NULL, NULL),
(21, 21, 'Ukończone', 1, 1, 8, NULL, CURRENT_TIMESTAMP + interval '6 months'),
-- MIESIĄC 8
(22, 22, 'Planowane', 0, 63, NULL, NULL, NULL),
(23, 23, 'Porzucone', 60, 73, 4, 'Ostatni sezon dramat.', CURRENT_TIMESTAMP + interval '7 months'),
(24, 24, 'Ukończone', 1, 1, 7, NULL, CURRENT_TIMESTAMP + interval '7 months'),
-- MIESIĄC 9
(25, 25, 'W trakcie', 45, 100, NULL, NULL, CURRENT_TIMESTAMP + interval '8 months'),
(26, 26, 'W trakcie', 10, 30, NULL, NULL, CURRENT_TIMESTAMP + interval '8 months'),
(27, 27, 'Ukończone', 1, 1, 9, NULL, CURRENT_TIMESTAMP + interval '8 months'),
-- MIESIĄC 10
(28, 28, 'Porzucone', 10, 34, 5, 'Za trudne.', CURRENT_TIMESTAMP + interval '9 months'),
(29, 29, 'Ukończone', 9, 9, 10, NULL, CURRENT_TIMESTAMP + interval '9 months'),
(30, 30, 'Ukończone', 1, 1, 9, NULL, CURRENT_TIMESTAMP + interval '9 months'),
-- MIESIĄC 11
(31, 31, 'Planowane', 0, 100, NULL, NULL, NULL),
(32, 32, 'W trakcie', 5, 17, NULL, NULL, CURRENT_TIMESTAMP + interval '10 months'),
(33, 33, 'Ukończone', 1, 1, 8, NULL, CURRENT_TIMESTAMP + interval '10 months'),
-- MIESIĄC 12
(34, 34, 'W trakcie', 10, 50, NULL, NULL, CURRENT_TIMESTAMP + interval '11 months'),
(35, 35, 'Planowane', 0, 24, NULL, NULL, NULL),
(36, 36, 'Ukończone', 1, 1, 10, 'Najlepsze!', CURRENT_TIMESTAMP + interval '11 months');

-- =========================================================
-- 5. DZIENNIK AKTYWNOŚCI (Sesje logowania)
-- =========================================================

-- MIESIĄC 1
INSERT INTO dziennik_aktywnosci (id_podejscia, przyrost_jednostek, czas_rozpoczecia, czas_zakonczenia, czas_trwania_sekundy, notatka) VALUES
(1, 15, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP + interval '3 hours', 10800, 'Kreator i prolog'),
(1, 20, CURRENT_TIMESTAMP + interval '2 days', CURRENT_TIMESTAMP + interval '2 days 4 hours', 14400, 'Akt pierwszy i glicze...'),
(2, 300, CURRENT_TIMESTAMP + interval '5 days', CURRENT_TIMESTAMP + interval '5 days 6 hours', 21600, 'Świetnie się czyta'),
(2, 300, CURRENT_TIMESTAMP + interval '10 days', CURRENT_TIMESTAMP + interval '10 days 6 hours', 21600, 'Skończona!'),
(3, 1, CURRENT_TIMESTAMP + interval '15 days', CURRENT_TIMESTAMP + interval '15 days 3 hours', 10800, 'Seans w Imax');

-- MIESIĄC 2
INSERT INTO dziennik_aktywnosci (id_podejscia, przyrost_jednostek, czas_rozpoczecia, czas_zakonczenia, czas_trwania_sekundy, notatka) VALUES
(4, 10, CURRENT_TIMESTAMP + interval '1 month 2 days', CURRENT_TIMESTAMP + interval '1 month 2 days 4 hours', 14400, 'Biały Sad'),
(4, 15, CURRENT_TIMESTAMP + interval '1 month 10 days', CURRENT_TIMESTAMP + interval '1 month 10 days 5 hours', 18000, 'Velen wbite'),
(5, 30, CURRENT_TIMESTAMP + interval '1 month 5 days', CURRENT_TIMESTAMP + interval '1 month 5 days 10 hours', 36000, 'Binge watching całego weekendu'),
(5, 32, CURRENT_TIMESTAMP + interval '1 month 20 days', CURRENT_TIMESTAMP + interval '1 month 20 days 10 hours', 36000, 'Finał niszczy system'),
(6, 1, CURRENT_TIMESTAMP + interval '1 month 25 days', CURRENT_TIMESTAMP + interval '1 month 25 days 2 hours', 7200, 'Kino nocne');

-- MIESIĄC 3
INSERT INTO dziennik_aktywnosci (id_podejscia, przyrost_jednostek, czas_rozpoczecia, czas_zakonczenia, czas_trwania_sekundy, notatka) VALUES
(7, 30, CURRENT_TIMESTAMP + interval '2 months 2 days', CURRENT_TIMESTAMP + interval '2 months 2 days 3 hours', 10800, 'Pierwsze runy'),
(7, 30, CURRENT_TIMESTAMP + interval '2 months 10 days', CURRENT_TIMESTAMP + interval '2 months 10 days 3 hours', 10800, 'Pokonanie Hydry'),
(8, 10, CURRENT_TIMESTAMP + interval '2 months 5 days', CURRENT_TIMESTAMP + interval '2 months 5 days 2 hours', 7200, 'Poranne czytanie'),
(8, 10, CURRENT_TIMESTAMP + interval '2 months 8 days', CURRENT_TIMESTAMP + interval '2 months 8 days 2 hours', 7200, 'Koniec poradnika'),
(9, 1, CURRENT_TIMESTAMP + interval '2 months 20 days', CURRENT_TIMESTAMP + interval '2 months 20 days 3 hours', 10800, 'Film z dziewczyną');

-- RESZTA ZAPISÓW (skrócona dystrybucja po kilka wpisów na losowe media z kolejnych miesięcy)
INSERT INTO dziennik_aktywnosci (id_podejscia, przyrost_jednostek, czas_rozpoczecia, czas_zakonczenia, czas_trwania_sekundy) VALUES
(10, 15, CURRENT_TIMESTAMP + interval '3 months 5 days', CURRENT_TIMESTAMP + interval '3 months 5 days 4 hours', 14400),
(11, 120, CURRENT_TIMESTAMP + interval '3 months 10 days', CURRENT_TIMESTAMP + interval '3 months 10 days 20 hours', 72000),
(12, 1, CURRENT_TIMESTAMP + interval '3 months 20 days', CURRENT_TIMESTAMP + interval '3 months 20 days 2 hours', 7200),
(13, 100, CURRENT_TIMESTAMP + interval '4 months 5 days', CURRENT_TIMESTAMP + interval '4 months 15 days 30 hours', 108000),
(14, 150, CURRENT_TIMESTAMP + interval '4 months 10 days', CURRENT_TIMESTAMP + interval '4 months 10 days 5 hours', 18000),
(15, 1, CURRENT_TIMESTAMP + interval '4 months 20 days', CURRENT_TIMESTAMP + interval '4 months 20 days 3 hours', 10800),
(17, 12, CURRENT_TIMESTAMP + interval '5 months 5 days', CURRENT_TIMESTAMP + interval '5 months 5 days 6 hours', 21600),
(18, 1, CURRENT_TIMESTAMP + interval '5 months 20 days', CURRENT_TIMESTAMP + interval '5 months 20 days 3 hours', 10800),
(19, 80, CURRENT_TIMESTAMP + interval '6 months 5 days', CURRENT_TIMESTAMP + interval '6 months 15 days 20 hours', 72000),
(21, 1, CURRENT_TIMESTAMP + interval '6 months 20 days', CURRENT_TIMESTAMP + interval '6 months 20 days 2 hours', 7200),
(23, 60, CURRENT_TIMESTAMP + interval '7 months 5 days', CURRENT_TIMESTAMP + interval '7 months 25 days 40 hours', 144000),
(24, 1, CURRENT_TIMESTAMP + interval '7 months 20 days', CURRENT_TIMESTAMP + interval '7 months 20 days 2 hours', 7200),
(25, 45, CURRENT_TIMESTAMP + interval '8 months 5 days', CURRENT_TIMESTAMP + interval '8 months 5 days 8 hours', 28800),
(26, 10, CURRENT_TIMESTAMP + interval '8 months 10 days', CURRENT_TIMESTAMP + interval '8 months 10 days 4 hours', 14400),
(27, 1, CURRENT_TIMESTAMP + interval '8 months 20 days', CURRENT_TIMESTAMP + interval '8 months 20 days 3 hours', 10800),
(28, 10, CURRENT_TIMESTAMP + interval '9 months 5 days', CURRENT_TIMESTAMP + interval '9 months 5 days 5 hours', 18000),
(29, 9, CURRENT_TIMESTAMP + interval '9 months 10 days', CURRENT_TIMESTAMP + interval '9 months 10 days 6 hours', 21600),
(30, 1, CURRENT_TIMESTAMP + interval '9 months 20 days', CURRENT_TIMESTAMP + interval '9 months 20 days 3 hours', 10800),
(32, 5, CURRENT_TIMESTAMP + interval '10 months 10 days', CURRENT_TIMESTAMP + interval '10 months 10 days 2 hours', 7200),
(33, 1, CURRENT_TIMESTAMP + interval '10 months 20 days', CURRENT_TIMESTAMP + interval '10 months 20 days 2 hours', 7200),
(34, 10, CURRENT_TIMESTAMP + interval '11 months 5 days', CURRENT_TIMESTAMP + interval '11 months 5 days 3 hours', 10800),
(36, 1, CURRENT_TIMESTAMP + interval '11 months 20 days', CURRENT_TIMESTAMP + interval '11 months 20 days 2 hours', 7200);

-- Aktualizacja liczników dla sekwencji (wymagane w Postgres, żeby następny autoincrement z aplikacji nie wyrzucił błędu!)
SELECT setval('platformy_id_seq', (SELECT MAX(id) FROM platformy));
SELECT setval('kategorie_id_seq', (SELECT MAX(id) FROM kategorie));
SELECT setval('tagi_id_seq', (SELECT MAX(id) FROM tagi));
SELECT setval('multimedia_id_seq', (SELECT MAX(id) FROM multimedia));
SELECT setval('podejscia_id_seq', (SELECT MAX(id) FROM podejscia));
SELECT setval('dziennik_aktywnosci_id_seq', (SELECT MAX(id) FROM dziennik_aktywnosci));

COMMIT;