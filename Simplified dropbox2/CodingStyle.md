### Coding style

V tomto projekte sa používa [Linux kernel coding style][1].
Použite súbor `.clang-format` z tohto repozitára na formátovanie vášho kódu.

### Dokumentácia

Na dokumentáciu sa používa [Doxygen][2]. Komentáre sa píšu v Javadoc style.

### Verzovanie

Bude existovať iba jediná vetva `master`, ktorá sa bude skladať z našich commitov.

##### Štruktúra commitov

Správy píšte v angličtine. Správy začínajú veľkým písmenom a slovesom v základnom tvare. Predmet commitu by nemal byť dlhší ako 50 znakov a končiť bodkou.
Príklad krátkeho commitu:
```shell
$ git commit -m "Remove unused variables"
```
Commity, ktoré potrebujú mať dlhší popisok napíšte cez príkaz
`git commit`. Predmet a telo správy by mali byť oddelené medzerou. Telo správy by malo byť max. 72 znakov široké. Príklad dlhej správy:
```
Add new Fruit module

This module adds new classes such as WaterMelon, Apple, Bananas, etc.
It also implements functions for communicating with our Vegetable
module. 
``` 
##### Nastavenie repozitára
Zadajte tento príkaz:
```shell
git config --local --add branch.master.mergeoptions --ff-only
```
kvôli zamezeniu mergov na `master` vetve.
##### Ako začať pracovať

1. Stiahni lokálne vetvu `master`

```shell
$ git pull 
```
2. Vytvor novú vetvu na ktorej budeš pracovať

```shell
$ git checkout -b moja_nova_vetva
```
3. Comitni nové zmeny na svojej vetve

##### Ako aplikovať zmeny na GitLab

4. Stiahni aktuálnu verziu vetvy `master`

```shell
$ git checkout master
$ git pull 
```
5. Presuň svoju pracovnú vetvu  za vetvu `master`

```shell
$ git checkout moja_nova_vetva
$ git rebase master 
```

6. Aktualizuj vetvu `master` aby obsahovala presunuté commity

```shell
$ git checkout master
$ git rebase moja_nova_vetva
```

7. Aplikuj svoje zmeny na GitLab

```shell
$ git push
```

[1]: https://www.kernel.org/doc/html/v4.10/process/coding-style.html
[2]: https://www.doxygen.nl/manual/docblocks.html
