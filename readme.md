# Tunele

## Klasy i struktury
1. Klasy:
    * `Richman` - proces
2. Struktury:
    * `s_tunnel` - tunel
    * `s_message` - wiadomość


## Stany
1. `RESTING` - stan, w którym proces odpoczywa przed kolejną podróżą 
2. `LOOKING_FOR_GROUP` - stan, w którym proces ubiega się o grupę
3. `IN_GROUP` - stan, w którym proces otrzymał dostęp do grupy
4. `WAITING_FOR_GROUP` - stan, w ktorym proces-szef grupy czeka na odpowiedzi odnosnie `GROUP_COMPLETED`
5. `LOOKING_FOR_TUNNEL` - stan, w którym proces szuka tunelu
6. `WAITING_FOR_TUNNEL` - stan, w którym proces czeka, aż szef grupy znajdzie tunel
7. `IN_TUNEL` - stan, w którym proces przechodzi przez tunel
8. `IN_PARADISE` - stan, w którym proces jest w innym wymiarze


## Typy wiadomości
1. `GROUP_REQ` - wiadomość wysyłana przez proces w celu ubiegania sie o miejsce w grupie
2. `GROUP_ACK` - wiadomość jako odpowiedź procesu odbierającego `GROUP_REQ` akceptująca miejsce w grupie
3. `GROUP_IM_IN` - wiadomość informujaca, ze proces wszedl do grupy
4. `GROUP_COMPLETED` - wiadomość do członków grupy, że mogą zwolnić grupę i przejść w stan czekania na tunel
5. `GROUP_LEFT` - wiadomosc potwierdzajaca opuszczenie grupy
6. `GROUP_FREE` - a dla szukajacych grupy, zeby zaczeli od nowa
7. `TUNNEL_REQ` - wiadomość wysyłana przez proces w celu ubiegania sie o miejsce w tunelu
8. `TUNNEL_ACK` - wiadomość jako odpowiedź procesu odbierającego `TUNNEL_REQ` mowiaca o stanie tunelu (grupy w tunelu)
9. `TRIP` - wiadomość mówiąca procesom, że przez dany tunel w danym kierunku przechodzi grupa
10. `TRIP_FINISHED` - wiadomość mówiąca procesom, że trip w danym tunelu się zakończył


## Algorytm
1. Inicjalizacja Bogacza -> stan `RESTING`
   
2. Proces czeka jakis okreslony czas -> stan `LOOKING_FOR_GROUP`
   
3. Proces wysyla wiadomości `GROUP_REQ` do wszystkich procesow.
   
4. Proces odbiera wiadomości `GROUP_ACK` od wszystkich procesow:
    1. `OK` jeśli proces nie ubiega się o grupę lub  ma niższy priorytet,
    2. jeśli proces jest w grupie wstrzymaj wysyłanie `GROUP_ACK` do momentu wyjscia ze stanu `IN_GROUP`

5. Proces dostając minimum `liczba_procesow - miejsca_w_grupie` wiadomości typu `GROUP_ACK` przechodzi w stan `IN_GROUP` i wysyla wiadomość `GROUP_IM_IN`.
   
6. Proces, który jest w grupie i otrzymał wiadomość typu `GROUP_IM_IN` zwieksza licznik procesow w grupie. Jeśli licznik wynosi `miejsca_w_grupie - 1` to zostaje on szefem grupy. Zmienia swój stan na `LOOKING_FOR_TUNNEL` i wysyla do grupowiczow `GROUP_COMPLETED`.
   
7. Proces, który jest w grupie otrzymując wiadomość typu `GROUP_COMPLETED` przechodzi w stan `WAITING_FOR_TUNNEL` i odsyla `GROUP_LEFT`.

8. Proces szef zlicza ile `GROUP_LEFT` dostal. Jesli bedzie to liczba `miejsca_w_grupie - 1` to oznacza, ze wszystie procesy opuscily juz grupe, wiec wysyla wiadomosc `GROUP_FREE` do wszystkich.

9. Procesy, ktore sa `WAITING_FOR_TUNNEL` i dostana `GROUP_FREE` resetuja kolejki zaleglych wiadomosci i nie beda wysylaly `GROUP_ACK` dopoki nie dostana nowego `GROUP_REQ`.

10. Procesy, które są w stanie `LOOKING_FOR_GROUP` resetuja swoj licznik potwierdzen `GROUP_ACK` i resetuja zapisane `GROUP_REQ`.
   
11. Proces-szef wysyla wiadomość `TUNNEL_REQ`.
    
12. Jeżeli pytany proces jest szefem swojej grupy to odpowiada `TUNNEL_ACK`:
    1. jeśli jest wewnątrz tunelu to id tunelu i kierunkiem,
    2. jeśli nie są teraz w tunelu lub szuka tunelu i ma mniejszy priorytet to -1

13. Proces iteruje sie po tunelach i na podstawie informacji z wiadomości `TUNNEL_ACK` określa, do którego tunelu może wejść:
    1. Jeśli jest tunel, który jest wolny lub idzie w tym samym kierunku i ma wystarczającą pojemność to zajmij je.
    2. W momencie zajęcia miejsca proces zmienia stan na `IN_TUNNEL` i wysyła wiadomość `TRIP`
    3. jeśli nie uda się zająć miejsca w żadnym z tuneli to proces czeka na wiadomość typu `TRIP_FINISHED`

14. Procesy, które są w stanie `WAITING_FOR_TUNNEL` i dostają od swojego szefa wiadomość `TRIP` zmieniają stan na `IN_TUNNEL`.

15. Szef grupy wyznacza losowy czas, który zajmie grupie podróż przez tunel.

16. Po wyznaczonym kwancie czasu wysyła `TRIP_FINISHED` i zmienia stan na `IN_PARADISE`. Wszystie procesy z jego grupy również zmieniają swój stan na ten.
    
17. Szef znów wyzancza jakiś czas, po którym następuje wysłanie wiadomości `TUNNEL_WAIT` do grupowiczów, którzy zmieniają stan na `WAITING_FOR_TUNNEL`, a on sam zmienia stan na `LOOKING_FOR_TUNNEL`.
    
18. Kroki od 11 do 15 w drugą stronę (kierunek tunelu przeciwny).
    
19.  Szef po upłynięciu czasu wysyła do wszystkich grupowiczów wiadomość typu `TRIP_FINISHED`. Procesy, które byly z nim w grupie (łącznie z nim samym) "resetują" się i przechodzą w stan `RESTING`.