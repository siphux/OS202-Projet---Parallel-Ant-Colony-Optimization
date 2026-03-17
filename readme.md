# Projet Fourmis – Algorithme d'optimisation par colonies de fourmis

Ce projet implémente un algorithme inspiré des colonies de fourmis, avec trois versions :
- **Séquentielle**
- **OpenMP** (parallèle)
- **MPI** (distribué)

---

## Structure du projet
   Dossier   | Description                                      |
 |-----------|--------------------------------------------------|
 | `/src/`   | Contient le `Makefile` et les fichiers sources.  |
 | `/build/` | Répertoire de sortie pour les exécutables.      |

---

## Prérequis

- Compilateur C (GCC, Clang, etc.)
- OpenMP (pour la version parallèle)
- MPI (pour la version distribuée)

---

## Compilation

1. Placez-vous dans le dossier `/src/` :
```bash
cd src/
```

Lancez la compilation :
```bash
make all
```
Les exécutables seront générés dans `/build/`.

## Exécution

## Version OpenMP

```bash
export OMP_NUM_THREADS=4 ./build/ant_simu_vect.exe   # Remplacez 4 par le nombre de threads souhaité
```

## Version MPI
```bash
OMP_NUM_THREADS=1 mpirun -np 4 ./build/ant_simu_mpi.exe   # Remplacez 4 par le nombre de processus souhaité
```
