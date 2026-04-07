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