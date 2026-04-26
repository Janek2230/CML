DROP TABLE IF EXISTS postepy CASCADE;
DROP TABLE IF EXISTS multimedia CASCADE;
DROP TABLE IF EXISTS platformy CASCADE;
DROP TABLE IF EXISTS kategorie CASCADE;
DROP TYPE IF EXISTS typ_statusu CASCADE;

INSERT INTO platformy (nazwa, typ_nosnika) VALUES
('Steam (PC)', 'Cyfrowy'),          
('PlayStation 5', 'Płyta/Cyfrowy'), 
('Netflix', 'Streaming'),           
('Kindle', 'E-book'),              
('Półka w salonie', 'Fizyczny');    


INSERT INTO kategorie (nazwa, jednostka) VALUES
('Gry', 'trofea'),       
('Książki', 'strony'),   
('Filmy', 'seans'),      
('Seriale', 'odcinki');  

INSERT INTO multimedia (tytul, id_kategorii, status, id_platformy, ocena, sciezka_okladki) VALUES
('Cyberpunk 2077', 1, 'W trakcie', 1, 8, '/covers/cp2077.jpg'),                 
('Wiedźmin: Ostatnie Życzenie', 2, 'Ukończone', 5, 9, '/covers/wiedzmin.jpg'),  
('Diuna: Część Druga', 3, 'Ukończone', 3, 10, '/covers/diuna2.jpg'),            
('Shogun', 4, 'W trakcie', 3, 9, '/covers/shogun.jpg'),                         
('Hollow Knight', 1, 'Planowane', 1, NULL, '/covers/hk.jpg');                   

INSERT INTO postepy (id_medium, wartosc_aktualna, wartosc_docelowa) VALUES
(1, 45, 57),  
(2, 280, 280), 
(3, 1, 1),
(4, 3, 10),    
(5, 0, 63);  




-- 1. WYPEŁNIANIE PLATFORM
INSERT INTO platformy (nazwa, typ_nosnika) VALUES
('Steam', 'Cyfrowy'),
('PlayStation 5', 'Płyta/Cyfrowy'),
('Nintendo Switch', 'Kartridż'),
('Kindle', 'E-book'),
('Audible', 'Audiobook'),
('Netflix', 'Streaming');

-- 2. WYPEŁNIANIE KATEGORII
INSERT INTO kategorie (nazwa, jednostka) VALUES
('Gry RPG', 'godziny'),
('Książki Beletrystyka', 'strony'),
('Seriale Anime', 'odcinki'),
('Kursy Online', '%'),
('Gry FPS', 'poziomy');

-- 3. WYPEŁNIANIE MULTIMEDIÓW (30 rekordów)
INSERT INTO multimedia (tytul, id_kategorii, id_platformy) VALUES
('Wiedźmin 3', 1, 1), ('Cyberpunk 2077', 1, 1), ('Elden Ring', 1, 2),
('Hobbit', 2, 4), ('Diuna', 2, 4), ('Władca Pierścieni', 2, 4),
('Naruto', 3, 6), ('One Piece', 3, 6), ('Attack on Titan', 3, 6),
('Kurs C++', 4, 1), ('Kurs Git', 4, 1), ('Kurs SQL', 4, 1),
('DOOM Eternal', 5, 2), ('Half-Life 2', 5, 1), ('Halo', 5, 2),
('Final Fantasy VII', 1, 2), ('Bloodborne', 1, 2), ('Zelda: TotK', 1, 3),
('Harry Potter', 2, 5), ('Sherlock Holmes', 2, 5), ('Fundacja', 2, 4),
('Death Note', 3, 6), ('Arcane', 3, 6), ('Stranger Things', 3, 6),
('God of War', 1, 2), ('Hades', 1, 3), ('Stardew Valley', 1, 3),
('Metro 2033', 2, 4), ('1984', 2, 4), ('Wielki Gatsby', 2, 5);

-- 4. WYPEŁNIANIE PODEJŚĆ (Dla pierwszych 15 multimediów)
-- Dzięki temu zobaczysz w Qt różnicę między "posiadanymi" a "ogrywanymi"
INSERT INTO podejscia (id_medium, numer_podejscia, status, ocena, wartosc_aktualna, wartosc_docelowa, data_rozpoczecia) VALUES
(1, 1, 'Ukończone', 10, 100, 100, '2023-01-01'),
(2, 1, 'W trakcie', 8, 45, 80, '2023-12-20'),
(3, 1, 'Planowane', NULL, 0, 60, NULL),
(4, 1, 'Ukończone', 9, 320, 320, '2023-05-10'),
(5, 1, 'W trakcie', NULL, 150, 600, '2024-01-05'),
(7, 1, 'W trakcie', 7, 12, 220, '2023-11-15'),
(10, 1, 'W trakcie', NULL, 20, 100, '2024-02-01'),
(13, 1, 'Porzucone', 5, 5, 25, '2023-06-01'),
(16, 1, 'Planowane', NULL, 0, 40, NULL),
(17, 1, 'Ukończone', 10, 50, 50, '2023-08-15'),
(18, 1, 'W trakcie', 9, 15, 120, '2024-03-01'),
(22, 1, 'Ukończone', 9, 37, 37, '2023-02-10'),
(25, 1, 'Planowane', NULL, 0, 30, NULL),
(26, 1, 'W trakcie', 8, 20, 50, '2024-03-10'),
(28, 1, 'Planowane', NULL, 0, 350, NULL);

-- 5. DZIENNIK AKTYWNOŚCI (Przykładowe wpisy do wykresów)
INSERT INTO dziennik_aktywnosci (id_podejscia, przyrost_jednostek, czas_trwania_sekundy) VALUES
(2, 5, 3600), (2, 10, 7200), -- Postępy w Cyberpunku
(5, 50, 1800), (5, 30, 2400), -- Czytanie Diuny
(7, 1, 1200), (7, 3, 3600);    -- Oglądanie Naruto