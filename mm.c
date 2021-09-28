/* Autor: Marcin Bieganek 310249
 *
 * Odnośnik do dokumentu hackMD, w którym znajduje się ten opis
 * wraz z ilustracjami znajduje się:
 * https://hackmd.io/h9V3Pa58QKunEPDLRiyT4w
 *
 * Znjaduje się on również w komentarzu do commita, zmieniającego
 * ten komentarz.
 *
 * Nagłówek i Stopka
 *
 * Nagłówek i stopka są takie same. Są rozmiaru jednego słowa, czyli 4 bajtów.
 * Składaję się one z rozmiaru (size), który jest wielokrotnością
 * 16 bajtów (stała ALIGNMENT). Dzięki temu na dwóch najmniej znaczących
 * bitach możemy przechowywać informację o tym czy blok jest wolny (FREE)
 * czy używany (USED), oraz o tym, czy poprzedni blok jest wolny (PREVFREE).
 * Pierwsza informacja jest przechowywana w pierwszym bicie od prawej,
 * a druga w drugim. Bazuje to na informacjach z wykładu.
 *
 * Blok zajęty i blok wolny
 *
 * Blok zajęty na początku ma nagłówek, następnie payload, czyli pamięć
 * na dane użytkownika, oraz na końcu może mieć opcjonalny padding,
 * który służy wyrównaniu rozmiaru bloku do wielokrotności
 * 16 (stała ALIGNMENT).
 *
 * Blok wolny na początku ma nagłówek, następnie wskaźnik na poprzedni
 * wolny blok, następnie wskaźnik na następny wolny blok,
 * następnie może mieć opcjonalny padding, który służy wyrównaniu
 * rozmiaru bloku do wielokrotnośći 16 (stała ALIGNMENT),
 * na końcu ma stopkę. Wskaźniki na poprzedni i następny blok są wskaźnikami
 * względnymi względem początku stosu (wartość z funkcji mem_heap_lo).
 * Dzięki takiemu rozwiązaniu możemy ograniczyć rozmiar tych wskaźników
 * do 4 bajtów. Jest to możliwe, ponieważ maksymalny rozmiar stosu jest
 * ograniczony do 4 GiB. Zastosowanie tego typu wskaźników pozwala zachować
 * minimalny rozmiar bloku równy 16 bajtów (stała ALIGNMENT).
 *
 * Sterta
 *
 * Na początku sterty mamy padding służący temu by payload bloków wypadał
 * na adresie będącym wielokrotnością 16 (stała ALIGNMENT).
 * Rozmiar tego paddingu to 3 słowa, czyli 12 bajtów, dzięki temu po
 * dodaniu tuż za nim nagłówka rozmiaru 1 słowa, czyli 4 bajtów,
 * payload wypadnie na wielokrotności 16.
 *
 * Następnie mamy specjalne wolne bloki, które reprezentują kolejne klasy
 * rozmiarów (sizeclasses) wolnych bloków. Liczba tych klas to 10
 * (stała NUMBER_OF_SIZECLASSES). Specjalność tych bloków polega na tym,
 * że są to wolne bloki minimalnego rozmiaru 16 bajtów (stała ALIGNMENT),
 * ale w nagłówku i stopce mają ustawiony rozmiar 0. Ponieważ są to
 * początkowo jedyne wolne bloki z danych klas rozmiarów to wskaźniki
 * na poprzednika i następnika mają ustawione na samych siebie.
 * Stosujemy takie rozwiązanie w celu optymalizacji działania funkcji
 * dodających, usuwających i wyszukujących wolne bloki. Pozwala to nam
 * nie przejmować się w tych funkcjach przypadkami krańcowymi,
 * gdy lista jest pusta, ponieważ zawsze jest na niej ten specjalny blok.
 * Rozmiar 0 w nagłówku został ustawiony, aby procedura szukająca wolnego
 * bloku o danym rozmiarze nigdy nie wybrała tych bloków specjalnych.
 * Kolejne użyte klasy rozmiarów to:
 * 16
 * 32
 * 48 - 64
 * 80 - 128
 * 144 - 256
 * 272 - 512
 * 528 - 1024
 * 1040 - 2048
 * 2064 - 4096
 * 4112 - +inf (4 GiB)
 *
 * Następnie mamy prologue i epilogue blok, które bazują na rozwiązaniu
 * z książki CSAPP. Prologue blok to zajęty blok minimalnego rozmiaru
 * 16 bajtów (stała ALIGNMENT). Zmienna globalna heap_start zawsze wskazuje
 * na niego. Epilogue blok to zajęty blok, który ma tylko nagłówek.
 * Jego rozmiar to 4 bajty (rozmiar słowa). Zmienna globalna last zawsze
 * wskazuje na niego. Te bloki nie reprezentują danych użytkownika.
 * Bloki przechowujące dane użytkownika i wolne bloki, będą umieszczane
 * pomiędzy blokiem prologue a epilogue. Dzięki temu te bloki zawsze będą
 * miały poprzednika i następnika (pierwszy blok jako poprzednika ma blok
 * prologue, a ostatni jako następnika ma epilogue). Dodatkowo warto zauważyć,
 * że w takim razie poprzednik epilogue to ostatni blok z tych na których
 * operujemy. Zastosowanie tych bloków pomocniczych pozwala nam nie rozważać
 * skrajnych przypadków przy różnych operacjach m.in. złączaniu wolnych bloków.
 *
 * Przydział bloku - malloc
 *
 * Gdy przydzielamy blok, najpierw musimy dodać do rozmiaru payloadu rozmiar
 * nagłówka i zaokrąglić to do wielokrotności 16 (stała ALIGNMENT).
 * Następnie korzystając z funkcji find_fit szukamy wolnego bloku, który jest
 * w stanie pomieścić nasz nowy blok. Funkcja ta przeszukuje listy wolnych
 * bloków z kolejnych klas rozmiarów, rozpoczynając od odpowiedniej dla
 * szukanego rozmiaru klasy.
 *
 * Jeśli znalazła to w miejscu wolnego bloku wstawiamy nowy zajęty blok,
 * usuwając wolny blok z listy wolnych bloków. Jeśli różnica w rozmiarach
 * jest na tyle duża, że ten wolny blok można podzielić na zajęty blok na
 * początku i nowy, mniejszy wolny blok w pozostałym  fragmencie,
 * to robimy to.
 *
 * Jeśli nie znalazła to znaczy, że musimy powiększyć stertę. Gdy ostatni
 * blok (poprzednik epilogue) jest wolny to powiększamy o brakujący nam
 * rozmiar. Następnie usuwamy ten blok z listy wolnych bloków. W miejscu
 * tego bloku, bloku epilogue i nowym obszarze umieszczamy nowy zajęty
 * blok. W jednym pozostąłym wolnym słowie na końcu nowo dodanego
 * obszaru umieszczamy nowy blok epilogue.
 *
 * Zwalnianie bloku - free
 * Przy zwalnianiu bloku wystarczy przerobić jego nagłówek i stopkę.
 * Następnie trzeba sprawdzić, czy sąsiedzi (poprzednik i następnik)
 * są wolni, jeśli są to łączymy je w jeden blok. Usuwamy przy tym wolnych
 * sąsiadów z listy wolnych bloków i dodajemy nowy wolny blok do listy
 * wolnych bloków w odpowiedniej klasie rozmiarów.
 *
 * Zmiana rozmiaru bloku - realloc
 * Na początku musimy sprawdzić szczególne przypadki użycia realloc,
 * czyli sytuacje, gdy tak naprawdę jest to free lub malloc. Następnie
 * sprawdzamy, czy wykorzystując jedynie stary blok i ewentualnie jego
 * wolnego następnika, jesteśmy w stanie zmienić odpowiednio rozmiar danego
 * bloku, żeby zadowolić żądanie. Gdy nowy i stary rozmiar bloku
 * (nie rozmiar payloadu) są takie same to nie musimy nic robić.
 * Gdy nowy rozmiar jest mniejszy to musimy odpowiednio zauktalizować stary
 * blok i ewentualnie we fragmencie, który był w starym bloku, ale nie jest
 * w nowym, tworzymy nowy wolny blok. Gdy nowy rozmiar jest większy od starego
 * to musimy sprawdzić, czy następnik jest wolny i czy jego rozmiar może pomóc
 * zmieścić nowy blok w tym miejscu. Jeśli tak to łączymy te bloki w jeden
 * zajęty blok i ewentualnie tworzymy nowy wolny blok na końcu tego fragmentu,
 * jeśli nowy blok jest mniejszego rozmiaru niż suma rozmiarów starego bloku
 * i następnika. Gdy te przypadki nie zachodzą to musimy zaalokować nowy blok
 * o nowym rozmiarze funkcją malloc, przekopiować dane przechowywane w starym
 * bloku do nowego bloku i zwolnić stary blok.
 */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

/* If you want debugging output, use the following macro.
 * When you hand in, remove the #define DEBUG line. */
//#define DEBUG
#ifdef DEBUG
#define debug(fmt, ...) printf("%s: " fmt "\n", __func__, __VA_ARGS__)
#define msg(...) printf(__VA_ARGS__)
#else
#define debug(fmt, ...)
#define msg(...)
#endif

#define __unused __attribute__((unused))

/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* !DRIVER */

/* Based on 'mm-implicit.c'. */
typedef int32_t word_t; /* Heap is bascially an array of 4-byte words. */

typedef enum {
  FREE = 0,     /* Block is free */
  USED = 1,     /* Block is used */
  PREVFREE = 2, /* Previous block is free (optimized boundary tags) */
} bt_flags;

static word_t *heap_start; /* Address of the first block */
static word_t *heap_end;   /* Address past last byte of last block */
static word_t *last;       /* Points at last block */
/* End based on 'mm-implicit.c'. */

#define NUMBER_OF_SIZECLASSES 10
static word_t *first_free_sizeclass; /* Address of the first block
                                        of the first sizeclass. */

/* --=[ boundary tag handling ]=-------------------------------------------- */

/* Based on 'mm-implicit.c'. */
static inline word_t bt_size(word_t *bt) {
  return *bt & ~(USED | PREVFREE);
}

static inline int bt_used(word_t *bt) {
  return *bt & USED;
}

static inline int bt_free(word_t *bt) {
  return !(*bt & USED);
}
/* End based on 'mm-implicit.c'. */

/* It's changed but based on 'mm-implicit.c'.
 * Given boundary tag address calculate it's buddy address.
 * If block is used it return header adress */
static inline word_t *bt_footer(word_t *bt) {
  return (void *)bt + (bt_free(bt)) * (bt_size(bt) - sizeof(word_t));
}

/* Based on 'mm-implicit.c'. */
/* Given payload pointer returns an address of boundary tag. */
static inline word_t *bt_fromptr(void *ptr) {
  msg("FROMPTR - payload: %p, block: %p\n", ptr, (word_t *)ptr - 1);
  return (word_t *)ptr - 1;
}

/* Previous block free flag handling for optimized boundary tags. */
static inline bt_flags bt_get_prevfree(word_t *bt) {
  return *bt & PREVFREE;
}

static inline void bt_clr_prevfree(word_t *bt) {
  if (bt)
    *bt &= ~PREVFREE;
}

static inline void bt_set_prevfree(word_t *bt) {
  *bt |= PREVFREE;
}
/* End based on 'mm-implicit.c'. */

/* Function name and signature based on 'mm-implicit.c'.
 * Returns address of next block.
 * Shouldn't be used on last block.*/
static inline word_t *bt_next(word_t *bt) {
  return (void *)bt + bt_size(bt);
}

/* Function name and signature based on 'mm-implicit.c'.
 * Returns address of previous block.
 * Shouldn't be used on first block.*/
static inline word_t *bt_prev(word_t *bt) {
  return (void *)bt - bt_size((void *)bt - sizeof(word_t));
}

/* Function name and signature based on 'mm-implicit.c'.
 * Creates boundary tag(s) for given block. */
static inline void bt_make(word_t *bt, size_t size, bt_flags flags) {
  /* Set header. */
  *bt = size | flags;
  /* bt_footer returns header if used and footer if free.
   * In case it's used we will set header twice.
   * It's for eliminating if statment. */
  word_t *footer = bt_footer(bt);
  *footer = size | flags;

  msg("MAKE - bt: %p, size: %ld, header: %d, btsize: %d\n", bt, size, *bt,
      bt_size(bt));
}

/* Based on 'mm-implicit.c'.
 * Returns address of payload. */
static inline void *bt_payload(word_t *bt) {
  msg("PAYLOAD - payload: %p, block: %p\n", bt + 1, bt);
  return bt + 1;
}

/* --=[ free block list handling ]=----------------------------------------- */

/* Sets pointer on previous block of bt on prev_free_bt. */
static inline void bt_set_prev_free(word_t *bt, word_t *prev_free_bt) {
  bt += 1;
  *bt = (word_t *)prev_free_bt - (word_t *)mem_heap_lo();
  msg("SET PREV - bt_prev_pointer: %p, prev_pointer: %d, bt: %p, prev_bt: %p\n",
      bt, *bt, bt - 1, prev_free_bt);
}

/* Gets pointer on previous block of bt. */
static inline word_t *bt_get_prev_free(word_t *bt) {
  bt += 1;
  msg("GET PREV - bt_prev_pointer: %p, prev: %p\n", bt,
      (word_t *)mem_heap_lo() + *bt);
  return (word_t *)mem_heap_lo() + *bt;
}

/* Sets pointer on next block of bt on next_free_bt. */
static inline void bt_set_next_free(word_t *bt, word_t *next_free_bt) {
  bt += 2;
  *bt = (word_t *)next_free_bt - (word_t *)mem_heap_lo();
  msg("SET NEXT - bt_next_pointer: %p, next_pointer: %d\n", bt, *bt);
}

/* Gets pointer on next block of bt. */
static inline word_t *bt_get_next_free(word_t *bt) {
  bt += 2;
  msg("GET NEXT - bt_next_pointer: %p, next: %p\n", bt,
      (word_t *)mem_heap_lo() + *bt);
  return (word_t *)mem_heap_lo() + *bt;
}

/* Given sizeclass number returns pointer on first block of the sizeclass. */
static inline word_t *bt_get_first_free(int number_of_sizeclass) {
  word_t *sizeclass = (word_t *)first_free_sizeclass + number_of_sizeclass * 4;
  return sizeclass;
}

/* Initializes first block of the given sizeclass.
 * This block has size zero and is free.
 * It's prev and next is himself. */
static inline void bt_set_first_free(int number_of_sizeclass) {
  word_t *sizeclass = (word_t *)first_free_sizeclass + number_of_sizeclass * 4;
  *sizeclass = FREE;
  word_t *footer = (void *)sizeclass + (ALIGNMENT - sizeof(word_t));
  *footer = FREE;
  bt_set_prev_free(sizeclass, sizeclass);
  bt_set_next_free(sizeclass, sizeclass);
}

static inline int bt_find_sizeclass(size_t size) {
  /* First sizeclass have blocks of size 16. */
  int number_of_sizeclass;
  size_t curr_max_size = ALIGNMENT;
  for (number_of_sizeclass = 0; number_of_sizeclass < NUMBER_OF_SIZECLASSES - 1;
       number_of_sizeclass++) {
    if (curr_max_size >= size)
      break;
    curr_max_size *= 2;
  }
  return number_of_sizeclass;
}

/* Add block bt to list of free blocks in his sizeclass. */
#define FIFO
#ifdef LIFO
static inline void bt_add_free(word_t *bt) {
  msg("ADD - bt: %p\n", bt);
  int number_of_sizeclass = bt_find_sizeclass(bt_size(bt));
  word_t *first_free = bt_get_first_free(number_of_sizeclass);
  msg("num_sizeclass: %d, first_free: %p\n", number_of_sizeclass, first_free);

  word_t *second_free = bt_get_next_free(first_free);
  bt_set_next_free(bt, second_free);
  bt_set_prev_free(bt, first_free);
  bt_set_next_free(first_free, bt);
  bt_set_prev_free(second_free, bt);
}
#endif
#ifdef FIFO
static inline void bt_add_free(word_t *bt) {
  msg("ADD - bt: %p\n", bt);
  int number_of_sizeclass = bt_find_sizeclass(bt_size(bt));
  word_t *first_free = bt_get_first_free(number_of_sizeclass);
  msg("num_sizeclass: %d, first_free: %p\n", number_of_sizeclass, first_free);

  word_t *last_free = bt_get_prev_free(first_free);
  bt_set_next_free(bt, first_free);
  bt_set_prev_free(bt, last_free);
  bt_set_next_free(last_free, bt);
  bt_set_prev_free(first_free, bt);
}
#else
/* Adress ordered adding strategy. */
static inline void bt_add_free(word_t *bt) {
  msg("ADD\n");
  int number_of_sizeclass = bt_find_sizeclass(bt_size(bt));
  word_t *first_free = bt_get_first_free(number_of_sizeclass);
  if (first_free == NULL) {
    bt_set_prev_free(bt, bt);
    bt_set_next_free(bt, bt);
    // first_free = bt;
    bt_set_first_free(number_of_sizeclass, bt);
  } else {
    word_t *curr_bt = first_free;
    do {
      curr_bt = bt_get_next_free(curr_bt);
    } while (curr_bt != first_free && curr_bt < bt);

    word_t *prev_curr_bt = bt_get_prev_free(curr_bt);
    bt_set_next_free(bt, curr_bt);
    bt_set_prev_free(bt, prev_curr_bt);
    bt_set_next_free(prev_curr_bt, bt);
    bt_set_prev_free(curr_bt, bt);
  }
}
#endif
#ifdef ADDRESS_ORDERED
static inline void bt_add_free(word_t *bt) {
  msg("ADD\n");
  int number_of_sizeclass = bt_find_sizeclass(bt_size(bt));
  word_t *first_free = bt_get_first_free(number_of_sizeclass);

  word_t *curr_bt = first_free;
  do {
    curr_bt = bt_get_next_free(curr_bt);
  } while (curr_bt != first_free && curr_bt < bt);

  word_t *prev_curr_bt = bt_get_prev_free(curr_bt);
  bt_set_next_free(bt, curr_bt);
  bt_set_prev_free(bt, prev_curr_bt);
  bt_set_next_free(prev_curr_bt, bt);
  bt_set_prev_free(curr_bt, bt);
}
#endif

/* Remove block bt from list of free blocks in his sizeclass. */
static inline void bt_remove_free(word_t *bt) {
  /* If there is only one free block. */
  msg("REMOVE - bt: %p\n", bt);

  word_t *prev_free = bt_get_prev_free(bt);
  word_t *next_free = bt_get_next_free(bt);
  bt_set_next_free(prev_free, next_free);
  bt_set_prev_free(next_free, prev_free);
}

/* --=[ miscellanous procedures ]=------------------------------------------ */

/* Calculates block size incl. header & payload,
 * and aligns it to block boundary (ALIGNMENT). */
static inline size_t blksz(size_t size) {
  return (size + sizeof(word_t) + ALIGNMENT - 1) & -ALIGNMENT;
}

/* Based on 'mm-implicit.c'.
 * Extends the heap. */
static void *morecore(size_t size) {
  void *ptr = mem_sbrk(size);
  if (ptr == (void *)-1)
    return NULL;
  return ptr;
}

/* Based on CSAPP, Fig. 9.46.
 * Coalesce free block with free neighbors. */
static inline word_t *coalesce(word_t *bt) {
  int prev_used = (bt_get_prevfree(bt) != PREVFREE);

  word_t *next = bt_next(bt);
  int next_used = bt_used(next);

  size_t size = bt_size(bt);

  /* Case 1
   * Prev & Next are used, so we don't change them. */
  if (prev_used && next_used) {

    /* Case 2
     * Prev is used, Next is free.
     * We have to join bt with Next. */
  } else if (prev_used && !next_used) {
    size += bt_size(next);

    bt_remove_free(next);
    bt_make(bt, size, FREE);
    bt_set_prevfree(bt_next(bt));
    /* Case 3
     * Prev is free, Next is used.
     * We have to join bt with Prev. */
  } else if (!prev_used && next_used) {
    word_t *prev = bt_prev(bt);
    size += bt_size(prev);

    bt = prev;
    bt_remove_free(prev);
    bt_make(bt, size, FREE | bt_get_prevfree(prev));
    bt_set_prevfree(bt_next(bt));
    /* Case 4
     * Prev is free & Next is free.
     * We have to join bt with Prev & Next. */
  } else {
    word_t *prev = bt_prev(bt);
    size += bt_size(next) + bt_size(prev);

    bt = prev;
    bt_remove_free(next);
    bt_remove_free(prev);
    bt_make(bt, size, FREE | bt_get_prevfree(prev));
    bt_set_prevfree(bt_next(bt));
  }
  /* We have to add newly created free block to list of free blocks.  */
  bt_add_free(bt);
  return bt;
}

/* --=[ mm_init ]=---------------------------------------------------------- */

int mm_init(void) {
  /* We have to add space at the beginig of the heap:
   * - (ALIGNMENT - sizeof(word_t)) - for alignment of payload,
   * - (NUMBER_OF_SIZECLASSES * ALIGNMENT) - for first block
   *                                         in every sizeclass,
   * - ALIGNMENT - for prologue block,
   * - sizeof(word_t) - for epilogue block.*/
  void *ptr =
    morecore(ALIGNMENT - sizeof(word_t) + (NUMBER_OF_SIZECLASSES * ALIGNMENT) +
             ALIGNMENT + sizeof(word_t));
  if (!ptr)
    return -1;
  heap_start = NULL;
  heap_end = NULL;
  last = NULL;
  /* Here we create prologue and epilogue block */
  word_t *prologue_bt = (word_t *)ptr + 3 + (NUMBER_OF_SIZECLASSES * 4);
  bt_make(prologue_bt, ALIGNMENT, USED);
  word_t *epilogue_bt = bt_next(prologue_bt);
  bt_make(epilogue_bt, sizeof(word_t), USED);
  heap_start = prologue_bt;
  last = epilogue_bt;
  heap_end = (void *)prologue_bt + bt_size(prologue_bt) + bt_size(epilogue_bt);

  /* Here we create first block in every sizeclass. */
  first_free_sizeclass = (word_t *)ptr + 3;
  for (int number_of_sizeclass = 0; number_of_sizeclass < NUMBER_OF_SIZECLASSES;
       number_of_sizeclass++)
    bt_set_first_free(number_of_sizeclass);
  return 0;
}

/* --=[ malloc ]=----------------------------------------------------------- */

#if 1
/* First fit startegy.
 * We start at first sizeclass which can have blocks of reqsz.
 * We search through sizeclass free list for fit.
 * If not found search next sizeclass. */
static word_t *find_fit(size_t reqsz) {
  for (int number_of_sizeclass = bt_find_sizeclass(reqsz);
       number_of_sizeclass < NUMBER_OF_SIZECLASSES; number_of_sizeclass++) {
    word_t *first_free = bt_get_first_free(number_of_sizeclass);

    word_t *curr_bt = first_free;
    do {
      if (bt_size(curr_bt) >= reqsz) {
        return curr_bt;
      }
      curr_bt = bt_get_next_free(curr_bt);
    } while (curr_bt != first_free);
  }
  return NULL;
}
#else
/* Best fit startegy.
 * We start at first sizeclass which can have blocks of reqsz.
 * We search through sizeclass free list for best fit.
 * If not found search next sizeclass. */
static word_t *find_fit(size_t reqsz) {
  word_t *best_bt;
  for (int number_of_sizeclass = bt_find_sizeclass(reqsz);
       number_of_sizeclass < NUMBER_OF_SIZECLASSES; number_of_sizeclass++) {
    word_t *first_free = bt_get_first_free(number_of_sizeclass);

    best_bt = NULL;
    word_t *curr_bt = first_free;
    do {
      if (bt_size(curr_bt) >= reqsz) {
        if (best_bt == NULL)
          best_bt = curr_bt;
        else if ((bt_size(curr_bt) - reqsz) < (bt_size(best_bt) - reqsz))
          best_bt = curr_bt;
        if (bt_size(best_bt) == reqsz)
          return best_bt;
      }
      curr_bt = bt_get_next_free(curr_bt);
    } while (curr_bt != first_free);

    if (best_bt != NULL)
      return best_bt;
  }
  return best_bt;
}
#endif

/* Based on CSAPP */
void *malloc(size_t size) {
  msg("\nMALLOC\n");
  if (size == 0)
    return NULL;

  msg("MALL - size: %ld\n", size);
  size_t blksize = blksz(size);
  msg("MALL - blksize: %ld\n", blksize);
  word_t *bt = find_fit(blksize);

  if (bt != NULL) {
    msg("FINDED\n");
    /* Possible we should split. */
    size_t old_blk_size = bt_size(bt);
    if ((old_blk_size - blksize) >= ALIGNMENT) {
      msg("old_blk_size: %ld, size: %ld, diff: %ld\n", old_blk_size, blksize,
          (old_blk_size - blksize));
      bt_remove_free(bt);
      bt_make(bt, blksize, USED | bt_get_prevfree(bt));

      word_t *new_free_block = bt_next(bt);
      msg("new_free_block: %p\n", new_free_block);
      bt_make(new_free_block, (old_blk_size - blksize), FREE);
      bt_add_free(new_free_block);
    } else {
      bt_remove_free(bt);
      bt_make(bt, old_blk_size, USED | bt_get_prevfree(bt));
      bt_clr_prevfree(bt_next(bt));
    }

    return bt_payload(bt);
  }

  msg("NOT FINDED\n");

  int last_free = bt_get_prevfree(last) >> 1;
  if (last_free) {
    bt = bt_prev(last);
    size_t size_we_need = blksize - bt_size(bt);

    void *ptr = morecore(size_we_need);
    if (ptr == NULL)
      return NULL;

    bt_remove_free(bt);
    bt_make(bt, blksize, USED | bt_get_prevfree(bt));

    last = (void *)bt + bt_size(bt);
    bt_make(last, sizeof(word_t), USED);
  } else {
    bt = last;
    void *ptr = morecore(blksize);
    if (ptr == NULL)
      return NULL;

    bt_make(bt, blksize, USED | bt_get_prevfree(bt));

    last = (void *)bt + bt_size(bt);
    bt_make(last, sizeof(word_t), USED);
  }
  heap_end = (void *)last + bt_size(last);

  msg("EXT - heap_start: %p, last: %p, heap_end: %p\n", heap_start, last,
      heap_end);
  msg("EXT - size: %d, free: %d\n", bt_size(bt), bt_free(bt));

  return bt_payload(bt);
}

/* --=[ free ]=------------------------------------------------------------- */

/* Based on CSAPP */
void free(void *ptr) {
  msg("\nFREE\n");
  if (ptr != NULL) {
    word_t *bt = bt_fromptr(ptr);
    size_t size = bt_size(bt);
    bt_make(bt, size, FREE | bt_get_prevfree(bt));
    bt_set_prevfree(bt_next(bt));
    coalesce(bt);

    msg("FREE - EXT2 - heap_start: %p, last: %p, heap_end: %p\n", heap_start,
        last, heap_end);
  }
}

/* --=[ realloc ]=---------------------------------------------------------- */

void *realloc(void *old_ptr, size_t size) {
  msg("\nREALLOC\n");
  if (size == 0) {
    free(old_ptr);
    return NULL;
  }

  if (old_ptr == NULL)
    return malloc(size);

  void *new_ptr = old_ptr;
  word_t *old_bt = bt_fromptr(old_ptr);

  size_t new_blk_size = blksz(size);
  size_t old_blk_size = bt_size(old_bt);

  if (new_blk_size == old_blk_size) {
    return old_ptr;
  } else if (new_blk_size < old_blk_size) {
    new_ptr = bt_payload(old_bt);

    if ((old_blk_size - new_blk_size) >= ALIGNMENT) {
      msg("old_blk_size: %ld, size: %ld, diff: %ld\n", old_blk_size,
          new_blk_size, (old_blk_size - new_blk_size));

      bt_make(old_bt, new_blk_size, USED | bt_get_prevfree(old_bt));

      word_t *new_free_block = bt_next(old_bt);
      msg("new_free_block: %p\n", new_free_block);
      bt_make(new_free_block, (old_blk_size - new_blk_size), FREE);
      bt_add_free(new_free_block);
      bt_set_prevfree(bt_next(new_free_block));
    }
    return old_ptr;

  } else if (new_blk_size > old_blk_size) {
    size_t size_avaible_around_old_blk = old_blk_size;

    word_t *next = bt_next(old_bt);
    int next_free = bt_free(next);

    if (next_free) {
      size_avaible_around_old_blk += bt_size(next);
      if (size_avaible_around_old_blk >= new_blk_size) {
        bt_remove_free(next);
        bt_make(old_bt, size_avaible_around_old_blk,
                USED | bt_get_prevfree(old_bt));
        bt_clr_prevfree(bt_next(old_bt));

        new_ptr = bt_payload(old_bt);

        old_blk_size = bt_size(old_bt);
        if ((old_blk_size - new_blk_size) >= ALIGNMENT) {
          msg("old_blk_size: %ld, size: %ld, diff: %ld\n", old_blk_size,
              new_blk_size, (old_blk_size - new_blk_size));

          bt_make(old_bt, new_blk_size, USED | bt_get_prevfree(old_bt));

          word_t *new_free_block = bt_next(old_bt);
          msg("new_free_block: %p\n", new_free_block);
          bt_make(new_free_block, (old_blk_size - new_blk_size), FREE);
          bt_add_free(new_free_block);
          bt_set_prevfree(bt_next(new_free_block));
        }
        return new_ptr;
      }
    }
  }

  new_ptr = malloc(size);
  if (new_ptr == NULL)
    return NULL;

  size_t old_size = bt_size(old_bt) - sizeof(word_t);
  if (size < old_size)
    old_size = size;
  memcpy(new_ptr, old_ptr, old_size);

  free(old_ptr);

  return new_ptr;
}

/* --=[ calloc ]=----------------------------------------------------------- */

void *calloc(size_t nmemb, size_t size) {
  size_t bytes = nmemb * size;
  void *new_ptr = malloc(bytes);
  if (new_ptr)
    memset(new_ptr, 0, bytes);
  return new_ptr;
}

/* --=[ mm_checkheap ]=----------------------------------------------------- */

void mm_checkheap(int verbose) {
  /*
  if (verbose) {
    Print list of all free blocks.
    msg("free_list:\n");
    for (int number_of_sizeclass = 0;
          number_of_sizeclass < NUMBER_OF_SIZECLASSES;
          number_of_sizeclass++) {
      msg("sizeclass: %d\n", number_of_sizeclass);
      word_t *first_free = bt_get_first_free(number_of_sizeclass);
      word_t *curr_bt = first_free;
      do {
        msg("bt: %p, size: %d, free: %d, next: %p, prev:%p\n",
            curr_bt, bt_size(curr_bt), bt_free(curr_bt),
            bt_get_next_free(curr_bt), bt_get_prev_free(curr_bt));
        curr_bt = bt_get_next_free(curr_bt);
      } while (curr_bt != first_free);
    }
     Print all blocks on the heap.
    msg("heap:\n");
    for (word_t *bt2 = heap_start; bt2 != last; bt2 = bt_next(bt2))
      msg("bt: %p, size: %d, free: %d\n", bt2, bt_size(bt2), bt_free(bt2));
  }

   Check if all blocks on free lists are free.
  for (int number_of_sizeclass = 0;
       number_of_sizeclass < NUMBER_OF_SIZECLASSES;
       number_of_sizeclass++) {
    word_t *first_free = bt_get_first_free(number_of_sizeclass);

    word_t *curr_bt = first_free;
    do {
      if (bt_used(curr_bt)) {
        msg("Block on free list is not free.\n");
        msg("bt: %p, size: %d, free: %d\n", bt2, bt_size(bt2), bt_free(bt2));
        exit(EXIT_FAILURE);
      }
      curr_bt = bt_get_next_free(curr_bt);
    } while (curr_bt != first_free);
  }

  Check if all free blocks are on free list.
  for (word_t *bt = heap_start; bt != last; bt = bt_next(bt)) {
    int finded = 0;
    for (int number_of_sizeclass = bt_find_sizeclass(bt_size(bt));
          number_of_sizeclass < NUMBER_OF_SIZECLASSES;
          number_of_sizeclass++) {
      word_t *first_free = bt_get_first_free(number_of_sizeclass);

      word_t *curr_bt = first_free;
      do {
        if (curr_bt == bt) {
          finded = 1;
          break;
        }
        curr_bt = bt_get_next_free(curr_bt);
      } while (curr_bt != first_free);
      if (finded)
        break;
    }
    if (!finded) {
      msg("Free block is not in free list.");
      msg("bt: %p, size: %d, free: %d\n", bt, bt_size(bt), bt_free(bt));
      exit(EXIT_FAILURE);
    }
  }

   Check if there is no two free neighbors.
  int last_free = USED;
  for (word_t *bt = heap_start; bt != last; bt = bt_next(bt)) {
    if (bt_free(bt) && last_free == FREE) {
      msg("Two free blocks are neighbors");
      msg("last_bt: %p, size: %d, free: %d\n", bt_prev(bt),
  bt_size(bt_prev(bt)), bt_free(bt_prev(bt))); msg("bt: %p, size: %d, free:
  %d\n", bt, bt_size(bt), bt_free(bt)); exit(EXIT_FAILURE);
    }
    last_free = bt_used(bt);
  }

   Check if prev & next pointers point to adress on the heap.
  for (int number_of_sizeclass = 0;
       number_of_sizeclass < NUMBER_OF_SIZECLASSES;
       number_of_sizeclass++) {
    word_t *first_free = bt_get_first_free(number_of_sizeclass);

    word_t *curr_bt = first_free;
    do {
      if ((word_t *)mem_heap_lo() >= bt_get_prev_free(curr_bt) ||
          (word_t *)mem_heap_hi() <= bt_get_prev_free(curr_bt) ||
          (word_t *)mem_heap_lo() >= bt_get_next_free(curr_bt) ||
          (word_t *)mem_heap_hi() <= bt_get_next_free(curr_bt)) {
        msg("Prev & Next pointers doesn't point to adress on the heap.\n");
        msg("bt: %p, size: %d, free: %d\n", curr_bt, bt_size(curr_bt),
  bt_free(curr_bt)); msg("bt_next_free: %p, bt_prev_free: %p\n",
            bt_get_prev_free(curr_bt), bt_get_next_free(curr_bt));
        exit(EXIT_FAILURE);
      }
      curr_bt = bt_get_next_free(curr_bt);
    } while (curr_bt != first_free);
  }

   Check if prev & next pointers point to free block.
  for (int number_of_sizeclass = 0;
       number_of_sizeclass < NUMBER_OF_SIZECLASSES;
       number_of_sizeclass++) {
    word_t *first_free = bt_get_first_free(number_of_sizeclass);

    word_t *curr_bt = first_free;
    do {
      if (bt_used(bt_get_prev_free(curr_bt)) ||
  bt_used(bt_get_next_free(curr_bt))) { msg("Prev & Next pointers doesn't point
  to free block.\n"); msg("bt: %p, size: %d, free: %d\n", curr_bt,
  bt_size(curr_bt), bt_free(curr_bt)); msg("bt_next_free: %p, bt_prev_free:
  %p\n", bt_get_prev_free(curr_bt), bt_get_next_free(curr_bt));
        exit(EXIT_FAILURE);
      }
      curr_bt = bt_get_next_free(curr_bt);
    } while (curr_bt != first_free);
  }
   */
}
