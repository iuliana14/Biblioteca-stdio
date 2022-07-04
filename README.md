# Biblioteca-stdio

## Descriere

Tema 2 consta in implementarea bibliotecii stdio, care presupune lucrul cu fisiere. Implementarea
functiilor din cadrul bibliotecii stdio a reprezentat un challenge, iar punctul de plecare al
acesteia a fost enuntul temei, care a fost destul de sugestiv, dar si laboratoarele facute pana
acum.


## Organizare

- so_stdio.h -> headerul pus la dispozitie
- so_stdio.c -> fisierul in care este implementata solutia temei
- Makefile -> realizeaza compilarea bibliotecii dinamice libso_stdio.so


## Implementare

Pentru implementarea bibliotecii stdio am avut nevoie de structura so_file, care contine 
urmatoarele:

typedef struct _so_file {
	int fd; // file descriptor
	int eof;
	int error;
	unsigned char buffer[SIZE];
	int buff_pos; // buffer position
	int opening_mode; // 1 - read, 2 - write
	int bytes; // number of bytes readed
	int cursor;
	int child;
} SO_FILE;

A fost implementat tot enuntul temei, fara functionalitati extra fata de cele descrise in enunt.

Enuntul temei este destul de clar si sugestiv, iar implementarea a avut acest punct de pornire,
fara alte functionalitati extra. De asemenea, si laboratoarele au fost de mare ajutor in
implementarea temei.

O parte din dificultatile pe care le-am avut au fost la verificarea functiilor so_fputc si
so_fgetc, unde a fost necesar un timp mai mult de rezolvare a problemelor de memcheck, in care
am descoperit ca aceasta apare de fiecare data atunci cand rularea testului nu ajunge pana la 
so_fclose. De asemenea, functia so_fflush a avut nevoie de mai multa atentie, deoarece in 
urma testarii datele nu se scriau cum trebuie in buffer si se realiza so_fflush cand nu era 
necesar, iar pentru acest lucru am adaugat un while, care se realizeaza cat timp nu a scris 
numarul total de caractere.


## Compilare si rulare

Pentru compilarea bibliotecii dinamice libso_stdio.so, care se realizeaza folosind "make"
am folosit Makefile-ul implementat de mine.

Realizare build:
	gcc -Wall -shared so_stdio.o -o libso_stdio.so
	gcc -Wall -g -fPIC -c so_stdio.c

Pentru testare se foloseste Makefile.checker, care va folosi biblioteca dinamica libso_stdio.so
si se realizeaza astfel:
	make -f Makefile.checker

De asemenea, pentru rularea unui singur test am folosit:
	./_test/run_test.sh init
	./_test/run_test.sh <nrTest>


## Bibliografie

[Web-site](https://ocw.cs.pub.ro/courses/so/laboratoare/laborator-02)
[Web-site](https://linux.die.net/man/2/open)
[Web-site](https://linux.die.net/man/2/read)
[Web-site](https://linux.die.net/man/2/write)
[Web-site](https://linux.die.net/man/2/lseek)
[Web-site](https://linux.die.net/man/2/lseek)
[Web-site](https://ocw.cs.pub.ro/courses/so/laboratoare/laborator-03)
[Web-site](https://www.geeksforgeeks.org/c-program-demonstrate-fork-and-pipe/)


