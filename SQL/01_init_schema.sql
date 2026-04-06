CREATE TABLE platformy (
    id SERIAL PRIMARY KEY,
    nazwa VARCHAR(100) NOT NULL,
    typ_nosnika VARCHAR(50) NOT NULL
);

CREATE TABLE kategorie (
    id SERIAL PRIMARY KEY,
    nazwa VARCHAR(255) NOT NULL,
    jednostka VARCHAR(50) NOT NULL -- np. przeczytane strony, obejrzane odcinki, 
);

CREATE TYPE typ_statusu AS ENUM ('Planowane', 'W trakcie', 'Ukończone', 'Porzucone');

CREATE TABLE multimedia (
    id SERIAL PRIMARY KEY,
    tytul VARCHAR(255) NOT NULL,
    id_kategorii INTEGER REFERENCES kategorie(id),
    status typ_statusu NOT NULL, 
    id_platformy INTEGER REFERENCES platformy(id) ON DELETE RESTRICT,
    data_dodania TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    data_ostatniej_edycji TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    ocena INTEGER CHECK (ocena >= 1 AND ocena <= 10),
    sciezka_okladki VARCHAR(500)                      
);

CREATE TABLE postepy (
    id SERIAL PRIMARY KEY,
    id_medium INTEGER UNIQUE REFERENCES multimedia(id) ON DELETE CASCADE,
    wartosc_aktualna INTEGER DEFAULT 0,
    wartosc_docelowa INTEGER DEFAULT 0,
    liczba_powtorek INTEGER DEFAULT 0
);