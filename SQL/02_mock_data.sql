INSERT INTO platformy (nazwa, typ_nosnika) VALUES
('Steam', 'cyfrowy'),
('Półka w salonie', 'fizyczny');

INSERT INTO multimedia (tytul, typ_medium, status, id_platformy) VALUES
('Wiedźmin 3: Dziki Gon', 'gra', 'rozpoczęte', 1),
('Władca Pierścieni: Drużyna Pierścienia', 'ksiazka', 'ukończone', 2);

INSERT INTO postepy_ksiazki (id_medium, przeczytane_strony, wszystkie_strony) VALUES
(2, 544, 544);