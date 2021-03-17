
# Komentarz do zmierzonych czasów
Czas wykonywania programu nie zmienił się pomiędzy różnymi typami ładowania 
bibliotek i poziomami optymalizacji. Wynika to z tego że najbardziej 
czaosochłonnymi operacjami programu są operacje przenoszenia pamięci oraz
wczytywania pliku które są zaimplementowane w bibliotece standardowej.


# Komentarz do dynamicznego ładowania biblioteki

Kod dynamicznego ładowania funkcji jest generowany przy użyciu makr
znajdujących się w pliku nagłówkowym `zad1/lib.h`. Makro LIB_TRAMPOLINES
wykorzystuje wcześniej zdefiniowane makro będące listą wywołań innego marka 
podanego jako argument. Generuje ono odpowiednią strukturę z wskaźnikami 
na wszystkie funkcje, funcje trampolinowe (ang. trampoline function)
oraz odpowiednią funckje do ich ładowania. Wszystkie funkcje są oznaczone 
jako `static inline`, aby uniknąć błędów kompilacji.

Wynik generowania wywowłania marka z pliku `zad1/merge.c`:

`gcc -nostdinc -E -DLIB_DYNAMIC ./zad1/merge.c`

```c
...
struct _merge {
  FILE * ( * merge_files)(FILE * left, FILE * right);
};
struct _merge _merge;
static inline void lib_load_merge(void) {
  _merge.merge_files = dlsym(_lib_dylib_handle, "merge_files");
}
static inline FILE * merge_files(FILE * left, FILE * right) {
  return ( * _merge.merge_files)(left, right);
}
...
```

Wygenerowane funckje trampolinowe posiadają taką samą deklarację jak funkcje 
biblioteki z tą różnicą że zostały oznaczone jako `static inline`. Ich ciało 
zawiera wyłącznie wywowałnie wskaźnika załadowanej funkcji z biblioteki dynamicznej
i zwrócenie wyniku działania.

Takie podejście umożliwa wykorzystywanie tego samego interfejsu biblioteki nie
zależnie od sposobu dołączania biblioteki.
