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
4. `LOOKING_FOR_TUNNEL` - stan, w którym proces szuka tunelu
5. `WAITING_FOR_TUNNEL` - stan, w którym proces czeka, aż szef grupy znajdzie tunel
6. `IN_TUNEL` - stan, w którym proces przechodzi przez tunel
7. `IN_PARADISE` - stan, w którym proces jest w innym wymiarze


## Typy wiadomości
1. `GROUP_REQ` - wiadomość wysyłana przez proces w celu ubiegania sie o miejsce w grupie
2. `GROUP_ACK` - wiadomość jako odpowiedź procesu odbierającego `GROUP_REQ` akceptująca miejsce w grupie
3. `GROUP_WHO_REQ` - wiadomość pytająca kto jest w grupie
4. `GROUP_WHO_ACK` - wiadomość mówiąca "jestem w grupie"
5. `TUNNEL_WAIT` - wiadomość do członków grupy, że mogą zwolnić grupę i przejść w stan czekania na tunel
6. `TRIP` - wiadomość mówiąca procesom, że przez dany tunel w danym kierunku przechodzi grupa
7. `TRIP_FINISHED` - wiadomość mówiąca procesom, że trip w danym tunelu się zakończył


## Algorytm
1. Inicjalizacja Bogacza -> stan `RESTING`
   
2. Proces czeka jakis okreslony czas -> stan `LOOKING_FOR_GROUP`
   
3. Proces wysyla wiadomości `GROUP_REQ` do wszystkich procesow.
   
4. Proces odbiera wiadomości `GROUP_ACK` od wszystkich procesow:
    a) `OK` jeśli proces nie ubiega się o grupę lub  ma niższy priorytet,
    b) jeśli proces jest w grupie wstrzymaj wysyłanie `GROUP_ACK` do momentu wyjscia ze stanu `LOOKING_FOR_GROUP`

5. Proces dostając minimum `liczba_procesow - miejsca_w_grupie + 1` wiadomości typu `GROUP_ACK` przechodzi w stan `IN_GROUP` i wysyla wiadomość `GROUP_WHO_REQ`.
   
6. Proces, który jest w grupie i otrzymał wiadomość typu `GROUP_WHO_REQ` odpowiada `GROUP_WHO_ACK`.
   
7. Proces zlicza wszystkie `GROUP_WHO_ACK`, które otrzymał i jeżeli ich liczba jest równa liczbie `miejsca_w_grupie - 1` zostaje on szefem grupy, który będzie szukał tunelu przechodząc w stan `LOOKING_FOR_TUNNEL` i wysyła on do wszystkich z grupy wiadomość `TUNNEL_WAIT`.
   
8. Proces, który jest w grupie otrzymując wiadomość typu `TUNNEL_WAIT` przechodzi w stan `WAITING_FOR_TUNNEL` i zwalnia miejsce w grupie wysyłając zaległe `GROUP_ACK`.
   
9.  Proces-szef wysyla wiadomość `TUNNEL_REQ`.
    
10. Jeżeli pytany proces jest szefem swojej grupy to odpowiada `TUNNEL_ACK`:
    a. jeśli jest wewnątrz tunelu to id tunelu i kierunkiem,
    b. jeśli nie są teraz w tunelu to -1

11. Proces iteruje sie po tunelach i na podstawie informacji z wiadomości `TUNNEL_ACK` określa, do którego tunelu może wejść:
    a. Jeśli jest tunel, który jest wolny lub idzie w tym samym kierunku i ma wystarczającą pojemność to zajmij je.
    b. W momencie zajęcia miejsca proces zmienia stan na `IN_TUNNEL` i wysyła wiadomość `TRIP`
    c. jeśli nie uda się zająć miejsca w żadnym z tuneli to proces czeka na wiadomość typu `TRIP_FINISHED`

12. Procesy, które są w stanie `WAITING_FOR_TUNNEL` i dostają od swojego szefa wiadomość `TRIP` zmieniają stan na `IN_TUNNEL`.

13. Szef grupy wyznacza losowy czas, który zajmie grupie podróż przez tunel.

14. Po wyznaczonym kwancie czasu wysyła `TRIP_FINISHED` i zmienia stan na `IN_PARADISE`. Wszystie procesy z jego grupy również zmieniają swój stan na ten.
    
15. Szef znów wyzancza jakiś czas, po którym następuje wysłanie wiadomości `TUNNEL_WAIT` do grupowiczów, którzy zmieniają stan na `WAITING_FOR_TUNNEL`, a on sam zmienia stan na `LOOKING_FOR_TUNNEL`.
    
16. Kroki od 9 do 13 w drugą stronę (kierunek tunelu przeciwny).
    
17.  Szef po upłynięciu czasu wysyła do wszystkich grupowiczów wiadomość typu `TRIP_FINISHED`. Procesy, które byly z nim w grupie (łącznie z nim samym) "resetują" się i przechodzą w stan `RESTING`.