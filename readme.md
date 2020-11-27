# Tunele

## Algorytm

1. Inicjalizacja Bogacza -> stan `RESTING`
2. Proces czeka jakis okreslony czas -> stan `LOOKING_FOR_GROUP`
3. Proces wysyla wiadomosci `GREQ` do wszystkich procesow.
4. Proces odbiera wiadomosci `GACK` od wszystkich procesow:
    a) `ID` procesu "bossa" - max(p_sender, p_own, p_boss) - najwyzszy priorytet wysylajacego, odbierajacego lub bossa procesu odbierajacego

    Proces, ktory jest bossem jesli posiada X czlonkow grupy (wlacznie ze soba) wysyla im `GCMPLT`, ktory mowi, ze skompletowana jest grupa i szukany jest portal.
5. Proces Boss wysyla wiadomosc 