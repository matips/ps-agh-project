# ps-agh-project
Project for PS classess (AGH 2014/2015). Description in Polish:
System plików wyświetlający zawartość zależną od kontekstu w którym się znajduje (np. włączone urządzenia, lokalizacja)

System plików zrealizowany jest w oparciu o bibliotkę FUSE (http://fuse.sourceforge.net/). Aby uruhcomić ją w systemie niezbędne jest zainstalowanie pakietu FUSE (opis instalacji dostępny na stornie projektu). 

Kompilacja programu może zostać zrealizwana przy użyciu Makefile. Wywołanie skompilowanego programu to:
  hello <storage> <mouned_dir_and_fuse_options>
gdzie:
  <storage> to folder który zostanie użyty do przechowywania danych (prgram umieści tam dane na swoje potrzeby)
  <mouned_dir_and_fuse_options> forlder w którym ma zostać zamonowany system plików; oprócz tego można dołączyć opcje dla biblioteki FUSE (nie jest do jednak potrzebne)

Program ustala środowisko w jakim działa w oparciu o katalog /dev/. Przy każdym zapytaniu kierowanum do naszego systemu plików wyliczany jest hash mająca jednoznacznie definiować "kontekst" w jakim użytkownik się znajduje. Sposób zapisu danych przez mój system plików jest bardoz prosty, bo wewnętrznie korzysta z obecnie używanego systemu plików. Każdy plik jaki użytkownik zapisze do mojego systemu plików zostanie zapisany w folderze: 
  <storage>/<hash srodowiska>/<sciezka pliku uzytkownika> 
Gdy użytkownik odczytuje pliki z mojego systemu plików, system wyszukuje je w odpowiednim folderze i zwraca odpowidnie informacje lub zawartość.  

Przykładowe użycie programu:
![screenshot](https://github.com/matips/ps-agh-project/blob/master/screenshot.png)
