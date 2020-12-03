# Tunele

## Klasy i struktury
1. Klasy:
    * `Richman` - proces
2. Struktury:
    * `s_tunnel` - tunel
    * `s_message` - wiadomość


## Stany
1. `RESTING` - stan, w którym proces odpoczywa przed kolejną podróżą.
2. `LOOKING_FOR_TUNNEL` - stan, w którym proces szuka tunelu.
3. `IN_TUNEL` - stan, w którym proces przechodzi przez tunel.
4. `IN_PARADISE` - stan, w którym proces jest w innym wymiarze.


## Typy wiadomości
1. `TUNNEL_REQ` - wiadomość wysyłana przez proces w celu ubiegania sie o miejsce w tunelu.
2. `TUNNEL_ACK` - wiadomość jako odpowiedź procesu odbierającego `TUNNEL_REQ` mowiaca o stanie tunelu.
3. `TRIP` - wiadomość mówiąca procesom, że przez dany tunel w danym kierunku przechodzi bogacz.
4. `TRIP_FINISHED` - wiadomość mówiąca procesom, że podróż w danym tunelu się zakończyła.


## Algorytm
1. Proces się inicjalizuje i przechodzi w stan `RESTING`.
   
2. Proces czeka jakiś okreslony czas, a po nim zbiera ekipę i przechodzi w stan `LOOKING_FOR_TUNNEL`.
  
3. Proces wysyła wiadomość `TUNNEL_REQ`, aby dostać się do tunelu.
    
4. Jeżeli proces otrzymal `TUNNEL_REQ` to odpowiada `TUNNEL_ACK`:
    1. Jeśli proces jest wewnątrz tunelu to odpowiada ID, kierunkiem i pojemnością tunelu,
    2. Jeśli proces również szuka tunelu i ma większy priorytet to wstrzymuje zapytanie,
    3. W kazdym innym wypadku proces wysyła id tunelu -1

5. Proces wysyłający `TUNNEL_REQ` musi zebrać wiadomości od wszystkich innych procesów, żeby mógł zacząć wybierać tunel.

6. Proces iteruje się po tunelach i na podstawie informacji z wiadomości `TUNNEL_ACK` określa, do którego tunelu może wejść:
    1. Jeśli jest tunel, który jest wolny lub idzie w tym samym kierunku i ma wystarczającą pojemność to wchodzi do tunelu.
    2. W momencie zajęcia miejsca proces zmienia stan na `IN_TUNNEL` i wysyła wiadomość `TRIP` oraz odpowiada na zaległe wstrzymane `TUNNEL_REQ`.
    3. Jeśli nie uda się zająć miejsca w żadnym z tuneli to proces czeka na wiadomość typu `TRIP_FINISHED`

8. Proces wyznacza losowy czas, który określa jak długo będzie przechodził przez tunel.

9. Po wyznaczonym kwancie czasu wysyła `TRIP_FINISHED` i zmienia stan na `IN_PARADISE`.

10. Proces po pobycie w raju wyzancza jakiś czas, po którym zmienia stan na `LOOKING_FOR_TUNNEL`.
    
11. Kroki od 5 do 8 w drugą stronę (kierunek tunelu przeciwny).
    
12.  Proces po upłynięciu czasu kończy podróż i resetuje się przechodząc w stan `RESTING`.
