# Tunele

## Klasy i struktury
1. Klasy:
    1.1) Richman - proces
2. Struktury:
    2.1) s_tunnel - tunel
    2.2) s_message - wiadomość


## Stany
1. `RESTING` - stan, w którym proces odpoczywa przed kolejną podróżą 
2. `LOOKING_FOR_GROUP` - stan, w którym proces ubiega się o grupę
3. `IN_GROUP` - stan, w którym proces otrzymał dostęp do grupy
4. `LOOKING_FOR_TUNNEL` - stan, w którym proces szuka tunelu
5. `WAITING_FOR_TUNNEL` - stan, w którym proces czeka, aż szef grupy znajdzie tunel
6. `IN_TUNEL` - stan, w którym proces przechodzi przez tunel

## Typy wiadomości
1. `GROUP_REQ` - wiadomość wysyłana przez proces w celu ubiegania sie o miejsce w grupie
2. `GROUP_ACK` - wiadomość jako odpowiedź procesu odbierającego `GROUP_REQ` akceptująca miejsce w grupie
3. `GROUP_WHO_REQ` - wiadomość pytająca kto jest w grupie
4. `GROUP_WHO_ACK` - wiadomość mówiąca "jestem w grupie"
5. `TUNNEL_WAIT` - wiadomość do członków grupy, że mogą zwolnić grupę i przejść w stan czekania na tunel


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
9. Proces-szef wysyla wiadomość `TUNNEL_REQ`.
    a. Jeżeli pytany proces jest szefem