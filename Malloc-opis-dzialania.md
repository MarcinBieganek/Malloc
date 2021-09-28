# SO - malloc opis
###### tags: `SO`

### Autor: Marcin Bieganek 310249

#### Nagłówek i Stopka

Nagłówek i stopka są takie same. Są rozmiaru jednego słowa, czyli 4 bajtów. Składaję się one z rozmiaru (size), który jest wielokrotnością 16 bajtów (stała ALIGNMENT). Dzięki temu na dwóch najmniej znaczących bitach możemy przechowywać informację o tym czy blok jest wolny (FREE) czy używany (USED), oraz o tym, czy poprzedni blok jest wolny (PREVFREE). Pierwsza informacja jest przechowywana w pierwszym bicie od prawej, a druga w drugim. Bazuje to na informacjach z wykładu.

![](https://i.imgur.com/EGxrrF6.png)


#### Blok zajęty i blok wolny

Blok zajęty na początku ma nagłówek, następnie payload, czyli pamięć na dane użytkownika, oraz na końcu może mieć opcjonalny padding, który służy wyrównaniu rozmiaru bloku do wielokrotnośći 16 (stała ALIGNMENT).

![](https://i.imgur.com/EQFlJX2.png)


Blok wolny na początku ma nagłówek, następnie wskaźnik na poprzedni wolny blok, następnie wskaźnik na następny wolny blok, następnie może mieć opcjonalny padding, który służy wyrównaniu rozmiaru bloku do wielokrotnośći 16 (stała ALIGNMENT), na końcu ma stopkę. Wskaźniki na poprzedni i następny blok są wskaźnikami względnymi względem początku stosu (wartość z funkcji mem_heap_lo). Dzięki takiemu rozwiązaniu możemy ograniczyć rozmiar tych wskaźników do 4 bajtów. Jest to możliwe, ponieważ maksymalny rozmiar stosu jest ograniczony do 4 GiB. Zastosowanie tego typu wskaźników pozwala zachować minimalny rozmiar bloku równy 16 bajtów (stała ALIGNMENT).

![](https://i.imgur.com/3FYHhwL.png)


#### Sterta

Na początku sterty mamy padding służący temu by payload bloków wypadał na adresie będącym wielokrotnością 16 (stała ALIGNMENT). Rozmiar tego paddingu to 3 słowa, czyli 12 bajtów, dzięki temu po dodaniu tuż za nim nagłówka rozmiaru 1 słowa, czyli 4 bajtów, payload wypadnie na wielokrotności 16.

Następnie mamy specjalne wolne bloki, które reprezentują kolejne klasy rozmiarów (sizeclasses) wolnych bloków. Liczba tych klas to 10 (stała NUMBER_OF_SIZECLASSES). Specjalność tych bloków polega na tym, że są to wolne bloki minimalnego rozmiaru 16 bajtów (stała ALIGNMENT), ale w nagłówku i stopce mają ustawiony rozmiar 0. Ponieważ są to początkowo jedyne wolne bloki z danych klas rozmiarów to wskaźniki na poprzednika i następnika mają ustawione na samych siebie. Stosujemy takie rozwiązanie w celu optymalizacji działania funkcji dodających, usuwających i wyszukujących wolne bloki. Pozwala to nam nie przejmować się w tych funkcjach przypadkami krańcowymi, gdy lista jest pusta, ponieważ zawsze jest na niej ten specjalny blok. Rozmiar 0 w nagłówku został ustawiony, aby procedura szukająca wolnego bloku o danym rozmiarze nigdy nie wybrała tych bloków specjalnych. Kolejne użyte klasy rozmiarów to:
    16
    32
    48 - 64
    80 - 128
    144 - 256
    272 - 512
    528 - 1024
    1040 - 2048
    2064 - 4096
    4112 - +inf (4 GiB)

Następnie mamy prologue i epilogue blok, które bazują na rozwiązaniu z książki CSAPP. Prologue blok to zajęty blok minimalnego rozmiaru 16 bajtów (stała ALIGNMENT). Zmienna globalna heap_start zawsze wskazuje na niego. Epilogue blok to zajęty blok, który ma tylko nagłówek. Jego rozmiar to 4 bajty (rozmiar słowa). Zmienna globalna last zawsze wskazuje na niego. Te bloki nie reprezentują danych użytkownika. Bloki przechowujące dane użytkownika i wolne bloki, będą umieszczane pomiędzy blokiem prologue a epilogue. Dzięki temu te bloki zawsze będą miały poprzednika i następnika (pierwszy blok jako poprzednika ma blok prologue, a ostatni jako następnika ma epilogue). Dodatkowo warto zauważyć, że w takim razie poprzednik epilogue to ostatni blok z tych na których operujemy. Zastosowanie tych bloków pomocniczych pozwala nam nie rozważać skrajnych przypadków przy różnych operacjach m.in. złączaniu wolnych bloków. 

![](https://i.imgur.com/qovw9SV.png)


#### Przydział bloku - malloc

Gdy przydzielamy blok, najpierw musimy dodać do rozmiaru payloadu rozmiar nagłówka i zaokrąglić to do wielokrotności 16 (stała ALIGNMENT). Następnie korzystając z funkcji find_fit szukamy wolnego bloku, który jest w stanie pomieścić nasz nowy blok. Funkcja ta przeszukuje listy wolnych bloków z kolejnych klas rozmiarów, rozpoczynając od odpowiedniej dla szukanego rozmiaru klasy. 

Jeśli znalazła to w miejscu wolnego bloku wstawiamy nowy zajęty blok, usuwając wolny blok z listy wolnych bloków. Jeśli różnica w rozmiarach jest na tyle duża, że ten wolny blok można podzielić na zajęty blok na początku i nowy, mniejszy wolny blok w pozostałym  fragmencie, to robimy to. 

Jeśli nie znalazła to znaczy, że musimy powiększyć stertę. Gdy ostatni blok (poprzednik epilogue) jest wolny to powiększamy o brakujący nam rozmiar. Następnie usuwamy ten blok z listy wolnych bloków. W miejscu tego bloku, bloku epilogue i nowym obszarze umieszczamy nowy zajęty blok. W jednym pozostąłym wolnym słowie na końcu nowo dodanego obszaru umieszczamy nowy blok epilogue.

#### Zwalnianie bloku - free

Przy zwalnianiu bloku wystarczy przerobić jego nagłówek i stopkę. Następnie trzeba sprawdzić, czy sąsiedzi (poprzednik i następnik) są wolni, jeśli są to łączymy je w jeden blok. Usuwamy przy tym wolnych sąsiadów z listy wolnych bloków i dodajemy nowy wolny blok do listy wolnych bloków w odpowiedniej klasie rozmiarów. 

#### Zmiana rozmiaru bloku - realloc

Na początku musimy sprawdzić szczególne przypadki użycia realloc, czyli sytuacje, gdy tak naprawdę jest to free lub malloc. Następnie sprawdzamy, czy wykorzystując jedynie stary blok i ewentualnie jego wolnego następnika, jesteśmy w stanie zmienić odpowiednio rozmiar danego bloku, żeby zadowolić żądanie. Gdy nowy i stary rozmiar bloku (nie rozmiar payloadu) są takie same to nie musimy nic robić. Gdy nowy rozmiar jest mniejszy to musimy odpowiednio zauktalizować stary blok i ewentualnie we fragmencie, który był w starym bloku, ale nie jest w nowym, tworzymy nowy wolny blok. Gdy nowy rozmiar jest większy od starego to musimy sprawdzić, czy następnik jest wolny i czy jego rozmiar może pomóc zmieścić nowy blok w tym miejscu. Jeśli tak to łączymy te bloki w jeden zajęty blok i ewentualnie tworzymy nowy wolny blok na końcu tego fragmentu, jeśli nowy blok jest mniejszego rozmiaru niż suma rozmiarów starego bloku i następnika. Gdy te przypadki nie zachodzą to musimy zaalokować nowy blok o nowym rozmiarze funkcją malloc, przekopiować dane przechowywane w starym bloku do nowego bloku i zwolnić stary blok.
