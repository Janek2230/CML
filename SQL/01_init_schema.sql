CREATE TABLE platformy (
    id SERIAL PRIMARY KEY,
    nazwa VARCHAR(100) NOT NULL,
    typ_nosnika VARCHAR(50) NOT NULL
);

CREATE TABLE multimedia (
    id SERIAL PRIMARY KEY,
    tytul VARCHAR(255) NOT NULL,
    typ_medium VARCHAR(50) NOT NULL, 
    status VARCHAR(50) NOT NULL,     
    id_platformy INTEGER REFERENCES platformy(id) ON DELETE RESTRICT
);

CREATE TABLE postepy_ksiazki (
    id_medium INTEGER PRIMARY KEY REFERENCES multimedia(id) ON DELETE CASCADE,
    przeczytane_strony INTEGER DEFAULT 0,
    wszystkie_strony INTEGER NOT NULL
);