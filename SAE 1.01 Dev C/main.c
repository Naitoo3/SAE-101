#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>

#pragma warning (disable: 4996 6031 6385 6262)

#define NB_UE 6
#define NB_SEMESTRES 6
#define MAX_ETUDIANTS 100
#define MAX_CHAR 30
#define NOTE_MOYENNE 10
#define NOTE_MOY_AJOURNEMENT 8
#define TROISEME_ANNEE_SEM_MIN 4
#define SEM_DIPLOME 6
#define DERNIERE_ANNEE 3

typedef enum {
    EN_COURS,
    DEMISSION_S,
    DEFAILLANT,
    AJOURNE,
    DIPLOME,
} StatutEtudiant; // Va servir à attribuer et/ou modifier le statut des étudiants

typedef struct {
    char prenom[MAX_CHAR];
    char nom[MAX_CHAR];
    float notes[NB_SEMESTRES][NB_UE]; // Une note par UE, 6 UE par semestres, 2 semestres par année
    int compensation[NB_SEMESTRES][NB_UE]; // 0 = Normal, 1 = ADC (compensé par RCUE), 2 = ADS (compensé par Année N+1)
    int id;
    StatutEtudiant statut;
    int semestreCourant; // Le semestre que l'étudiant est EN TRAIN de suivre (1 à 6)
} Etudiant; // La structure de l'etudiant

// ----------------------------------------- LES FONCTIONS ----------------------------------------- //

int EXIT(char* commande) {
    exit(0);
    return 0;
}

int INSCRIRE(Etudiant etudiants[], int* nbEtudiants, char* prenom, char* nom) {
    // Vérifier si déjà inscrit
    for (int i = 0; i < *nbEtudiants; i++) {
        if (strcmp(etudiants[i].prenom, prenom) == 0 && strcmp(etudiants[i].nom, nom) == 0) {
            printf("Nom incorrect\n");
            return 0;
        }
    }

    // Ajouter nouvel étudiant
    Etudiant* e = &etudiants[*nbEtudiants];
    strncpy(e->prenom, prenom, MAX_CHAR - 1);
    e->prenom[MAX_CHAR - 1] = '\0';
    strncpy(e->nom, nom, MAX_CHAR - 1);
    e->nom[MAX_CHAR - 1] = '\0';

    e->id = *nbEtudiants + 1;
    e->statut = EN_COURS;
    e->semestreCourant = 1;

    // Initialiser les notes à -1
    for (int s = 0; s < NB_SEMESTRES; s++)
        for (int u = 0; u < NB_UE; u++) {
            e->notes[s][u] = -1;
            e->compensation[s][u] = 0;
        }
    printf("Inscription enregistree (%d)\n", *nbEtudiants + 1);
    (*nbEtudiants)++;
    return 1;
}


void NOTE(Etudiant etudiants[], int nbEtudiants, int id, int ue, float note) {
    if (id < 1 || id > nbEtudiants) {
        printf("Identifiant incorrect\n");
        return;
    }

    if (ue < 1 || ue > NB_UE) {
        printf("UE incorrecte\n");
        return;
    }

    Etudiant* e = &etudiants[id - 1];

    // Si l'étudiant n'est plus dans la formation (ajourné, demissionnaire etc.), On ne peut pas lui donner de note.
    if (e->statut != EN_COURS) {
        printf("Etudiant hors formation\n");
        return;
    }

    if (note < 0.0f || note > 20.0f) {
        printf("Note incorrecte\n");
        return;
    }

    int semestre = e->semestreCourant - 1; // Le semestreCourant (1-6) est l'indice (0-5) + 1
    e->notes[semestre][ue - 1] = note;

    printf("Note enregistree\n");
}


void CURSUS(Etudiant* etudiants, int nbEtudiants, int id) {
    if (id < 1 || id > nbEtudiants) {
        printf("Identifiant incorrect\n");
        return;
    }

    Etudiant* e = &etudiants[id - 1];
    printf("%d %s %s\n", id, e->prenom, e->nom);

    // On affiche tous les semestres jusqu'au semestre courant
    // Sauf si diplômé, on affiche tout (les 6 semestres)
    int semestresAffichage = (e->statut == DIPLOME) ? NB_SEMESTRES : e->semestreCourant;

    for (int s = 0; s < semestresAffichage; s++) {
        printf("S%d - ", s + 1);

        int notes_presentes_semestre = 1; // Pour Bilan !!
        for (int u = 0; u < NB_UE; u++) {
            float note = e->notes[s][u];
            if (note == -1) {
                printf("* (*)");
                notes_presentes_semestre = 0;
            }
            else {
                float nt = floorf(note * 10.f) / 10.f;

                if (e->compensation[s][u] == 1) {      // ADC
                    printf("%.1f (ADC)", nt);
                }
                else if (e->compensation[s][u] == 2) { // ADS
                    printf("%.1f (ADS)", nt);
                }
                else {
                    if (nt >= 10.0f)
                        printf("%.1f (ADM)", nt);
                    else
                        printf("%.1f (AJ)", nt);
                }
            }

            if (u < NB_UE - 1) printf(" - "); // Il y'a 6 UE en tout, donc on termine l'affichage d'un semestre par un "-" comme demandé.
        }

        // Si c'est le dernier semestre (s+1 == e->semestreCourant)
        if (s + 1 == e->semestreCourant) {
            if (e->statut == EN_COURS) {
                printf(" - en cours\n");
                // On n'affiche pas les semestres futurs
                break;
            }
            else if (e->statut == DEMISSION_S) {
                printf(" - demission\n");
                break;
            }
            else if (e->statut == DEFAILLANT) {
                printf(" - defaillance\n");
                break;
            }
            else {
                // Cas (AJOURNE, DIPLOME) où le statut s'affiche sur la ligne Bilan
                printf(" -\n");
            }
        }
        else {
            // Semestre terminé normal
            printf(" -\n");
        }


        // --- AFFICHAGE LIGNE BILAN (RCUE) ---
        if ((s + 1) % 2 == 0) { // Si S2, S4, S6 (Semestre Pair)
            int annee = (s + 1) / 2;

            // Vérifier si on a les notes des 2 semestres (S(s) et S(s-1))
            int notes_presentes_bilan = notes_presentes_semestre; // Notes S(s)
            for (int u = 0; u < NB_UE; u++) {
                if (e->notes[s - 1][u] == -1) { // Notes S(s-1)
                    notes_presentes_bilan = 0;
                    break;
                }
            }

            // Si les notes manquent (ex: démission S1), on n'affiche pas le Bilan
            if (!notes_presentes_bilan) {
                continue;
            }

            printf("B%d - ", annee);
            for (int u = 0; u < NB_UE; u++) {
                float n1 = e->notes[s - 1][u];
                float n2 = e->notes[s][u];

                float moyenne = (n1 + n2) / 2.0f;
                float mt = floorf(moyenne * 10.f) / 10.f; // Utilisation de floorf pour avoir une moyenne tronquée

                int statutADS = (e->compensation[s - 1][u] == 2 || e->compensation[s][u] == 2);

                if (statutADS) {
                    printf("%.1f (ADS)", mt);
                }
                else if (mt >= 10.0f) {
                    printf("%.1f (ADM)", mt);
                }

                else if (mt >= 8.0f)
                    printf("%.1f (AJ)", mt);
                else
                    printf("%.1f (AJB)", mt);

                if (u < NB_UE - 1) printf(" - ");
            }

            // --- AFFICHAGE STATUT LIGNE BILAN ---
            // Si l'étudiant a été ajourné à la fin de ce semestre
            if (e->statut == AJOURNE && e->semestreCourant == (s + 1)) {
                printf(" - ajourne\n");
                break; // On n'affiche pas les semestres futurs
            }
            // Si l'étudiant est diplômé (forcément S6)
            else if (e->statut == DIPLOME && (s + 1) == NB_SEMESTRES) {
                printf(" - diplome\n");
                break;
            }
            else {
                printf(" -\n");
            }
        }
    }
}


void ETUDIANTS(Etudiant etudiants[], int nbEtudiants) {
    assert(nbEtudiants != 0); // Verifie si il y'a bien des étudiants inscrits dans la promo.

    for (int i = 0; i < nbEtudiants; i++) {
        const char* Statut;
        switch (etudiants[i].statut) {
        case EN_COURS:
            Statut = "en cours";
            break;
        case DEMISSION_S:
            Statut = "demission";
            break;
        case DEFAILLANT:
            Statut = "defaillance";
            break;
        case AJOURNE:
            Statut = "ajourne";
            break;
        case DIPLOME:
            Statut = "diplome";
            break;
        default:
            Statut = "inconnu";
            break;
        }

        printf("%d - %s %s - S%d - %s\n",
            etudiants[i].id,
            etudiants[i].prenom,
            etudiants[i].nom,
            etudiants[i].semestreCourant,
            Statut); // Affiche: son ID, son nom et prénom, le semestre courant et son statut.
    }
}

void JURY(Etudiant etudiants[], int nbEtudiants, int semestre) {
    if (semestre < 1 || semestre > NB_SEMESTRES) {
        printf("Semestre incorrect\n");
        return;
    }

    int semIndex = semestre - 1; // 0 à 5 pour le parcours du tableau

    // Vérification des notes manquantes
    int notesManquantes = 0;
    for (int i = 0; i < nbEtudiants; i++) {
        if (etudiants[i].statut == EN_COURS && etudiants[i].semestreCourant - 1 == semIndex) {
            for (int u = 0; u < NB_UE; u++) {
                if (etudiants[i].notes[semIndex][u] == -1) {
                    notesManquantes = 1;
                    break;
                }
            }
        }
        if (notesManquantes) break;
    }

    if (notesManquantes) {
        printf("Des notes sont manquantes\n");
        return;
    }


    int x = 0; // Compteur d'étudiants délibérés

    for (int i = 0; i < nbEtudiants; i++) {
        Etudiant* e = &etudiants[i];

        // On ne délibère que les étudiants EN_COURS dans le bon semestre
        if (e->statut != EN_COURS) continue;
        if (e->semestreCourant - 1 != semIndex) continue; // fonction continue = Ignorer cet élément et passer au suivant

        x++; // Cet étudiant sera délibéré

        //  JURY SEMESTRE IMPAIR (S1, S3, S5)
        if (semestre % 2 == 1) {
            e->semestreCourant++;
            continue;
        }

        //  JURY SEMESTRE PAIR (S2, S4, S6) -------------------------------- SPRINT 3 !!

        // 1. Calcul ADC : Compensation dans la même année
        for (int u = 0; u < NB_UE; u++) {
            float n1 = e->notes[semIndex - 1][u]; // S(N-1)
            float n2 = e->notes[semIndex][u];     // S(N)
            float rc = (n1 + n2) / 2.0f;

            if (rc >= 10) {
                if (n1 < NOTE_MOYENNE)
                    e->compensation[semIndex - 1][u] = 1; // ADC
                if (n2 < NOTE_MOYENNE)
                    e->compensation[semIndex][u] = 1;     // ADC
            }
        }

        // 2. Calcul ADS : compensation remboursée par l'année suivante 
        if (semestre >= 4) { // ADS possible seulement en B2 (S4) ou B3 (S6)
            int annee_prec_s1 = semIndex - 3; // ex: S4 (index 3) -> S1 (index 0)
            int annee_prec_s2 = semIndex - 2; // ex: S4 (index 3) -> S2 (index 1)

            for (int u = 0; u < NB_UE; u++) {
                float n_prec_1 = e->notes[annee_prec_s1][u];
                float n_prec_2 = e->notes[annee_prec_s2][u];
                float n_cur_1 = e->notes[semIndex - 1][u];
                float n_cur_2 = e->notes[semIndex][u];

                float rc_prev = (n_prec_1 + n_prec_2) / 2.0f;
                float rc_cur = (n_cur_1 + n_cur_2) / 2.0f;

                if (rc_cur >= NOTE_MOYENNE && rc_prev < NOTE_MOYENNE) {
                    // L'UE S(N-1) est compensée (ADS)
                    if (n_prec_1 < NOTE_MOYENNE) e->compensation[annee_prec_s1][u] = 2;
                    // L'UE S(N) est compensée (ADS)
                    if (n_prec_2 < NOTE_MOYENNE) e->compensation[annee_prec_s2][u] = 2;
                }
            }
        }

        // 3. Calcul Passage ou Ajournement (Règle RCUE)
        int rcueOK = 0;
        int rcueBad = 0; // (AJB)

        for (int u = 0; u < NB_UE; u++) {
            float n1 = e->notes[semIndex - 1][u];
            float n2 = e->notes[semIndex][u];
            float rc = (n1 + n2) / 2.0f;

            if (rc >= NOTE_MOYENNE)
                rcueOK++;
            if (rc < NOTE_MOY_AJOURNEMENT)
                rcueBad = 1;
        }

        int passe = 0; // 0 = Ajourné/Diplômé, 1 = Passe

        if (rcueOK > (NB_UE / 2) && rcueBad == 0) { // Règle RCUE OK
            passe = 1;
        }

        // 4. Règles de passage spécifiques
        if (semestre == TROISEME_ANNEE_SEM_MIN && passe) {
            int anneeValidee = 1;
            for (int u = 0; u < NB_UE; u++) {
                // Verifie S1 (index 0)
                if (e->notes[0][u] < 10.0f && e->compensation[0][u] == 0) anneeValidee = 0;
                //Verifie S2 (index 1)
                if (e->notes[1][u] < 10.0f && e->compensation[1][u] == 0) anneeValidee = 0;
            }
            if (!anneeValidee) passe = 0; // Bloqué, ne passe pas en S5 (3eme année).
        }
        else if (semestre == SEM_DIPLOME) {
            // DIPLOME : Toutes les UE des 3 années doivent être validées 
            int toutValidees = 1;
            for (int s = 0; s < NB_SEMESTRES; s++) {
                for (int u = 0; u < NB_UE; u++) {
                    if (e->notes[s][u] < 10.0f && e->compensation[s][u] == 0) {
                        toutValidees = 0;
                        break;
                    }
                }
                if (!toutValidees) break;
            }

            if (toutValidees) {
                e->statut = DIPLOME;
            }
            else {
                e->statut = AJOURNE;
            }
            passe = 0; // Dans tous les cas (diplômé ou ajourné), la scolarité s'arrête
        }

        // 5. Appliquer la décision
        if (passe) {
            e->semestreCourant++;
        }
        else {
            // Si on n'est pas déjà diplômé, on est ajourné
            if (e->statut != DIPLOME) {
                e->statut = AJOURNE;
            }
        }
    }
    // P.S: JURY semestre pair était super dur à coder!! :(

    printf("Semestre termine pour %d etudiant(s)\n", x);
}


void DEMISSION(Etudiant* etudiants, int nbEtudiants, int id) {
    if (id < 1 || id > nbEtudiants) {
        printf("Identifiant incorrect\n");
        return;
    }

    Etudiant* e = &etudiants[id - 1];

    if (e->statut != EN_COURS) {
        printf("Etudiant hors formation\n");
        return;
    }

    e->statut = DEMISSION_S;
    printf("Demission enregistree\n");
}

void DEFAILLANCE(Etudiant* etudiants, int nbEtudiants, int id) {
    if (id < 1 || id > nbEtudiants) {
        printf("Identifiant incorrect\n");
        return;
    }

    Etudiant* e = &etudiants[id - 1];

    if (e->statut != EN_COURS) {
        printf("Etudiant hors formation\n");
        return;
    }

    e->statut = DEFAILLANT;
    printf("Defaillance enregistree\n");
}


void BILAN(Etudiant* etudiants, int nbEtudiants, int anneeNumero) {
    if (anneeNumero < 1 || anneeNumero > DERNIERE_ANNEE) {
        printf("Annee incorrecte\n");
        return;
    }

    int nbDemissions = 0, nbDefaillants = 0, nbEnCours = 0, nbAjournes = 0, nbPasse = 0;

    int semMin = (anneeNumero - 1) * 2 + 1; // Année 1 -> Sem 1 etc.
    int semMax = anneeNumero * 2;         // Année 1 -> Sem 2 etc.

    for (int i = 0; i < nbEtudiants; i++) {
        Etudiant* e = &etudiants[i];
        int semCourant = e->semestreCourant;

        // 1. L'étudiant n'a jamais atteint cette année
        if (semCourant < semMin) {
            continue;
        }

        // 2. L'étudiant est EN COURS dans cette année
        if (e->statut == EN_COURS && (semCourant == semMin || semCourant == semMax)) {
            nbEnCours++;
        }
        // 3. L'étudiant a DEMISSIONNE ou ETE DEFAILLANT dans cette année
        else if ((e->statut == DEMISSION_S || e->statut == DEFAILLANT) && (semCourant == semMin || semCourant == semMax)) {
            if (e->statut == DEMISSION_S) nbDemissions++;
            else nbDefaillants++;
        }
        // 4. L'étudiant a été AJOURNE à la fin de cette année
        else if (e->statut == AJOURNE && semCourant == semMax) {
            nbAjournes++;
        }
        // 5. L'étudiant est PASSE (est dans une année >)
        else if (semCourant > semMax) {
            nbPasse++;
        }
        // 6. L'étudiant est DIPLOME (compte comme "passe" pour B1, B2, B3)
        else if (e->statut == DIPLOME) {
            // Si DIPLOME, il a forcément "passé" B1, B2, et B3
            nbPasse++;
        }
    }

    printf("%d demission(s)\n", nbDemissions);
    printf("%d defaillance(s)\n", nbDefaillants);
    printf("%d en cours\n", nbEnCours);
    printf("%d ajourne(s)\n", nbAjournes);
    printf("%d passe(s)\n", nbPasse);
}

// ----------------------------------------- MAIN ----------------------------------------- //
int main() {
    Etudiant etudiants[MAX_ETUDIANTS];
    int nbEtudiants = 0;
    char commande[MAX_CHAR];


    while (1) {
        if (scanf("%31s", commande) != 1) {
            break;
        }
        commande[MAX_CHAR - 1] = '\0';

        if (strcmp(commande, "EXIT") == 0) {
            EXIT(commande);
        }
        else if (strcmp(commande, "INSCRIRE") == 0) {
            char prenom[MAX_CHAR], nom[MAX_CHAR];
            scanf("%30s %30s", prenom, nom);
            INSCRIRE(etudiants, &nbEtudiants, prenom, nom);
        }
        else if (strcmp(commande, "NOTE") == 0) {
            int id, ue;
            float note;
            scanf("%d %d %f", &id, &ue, &note);
            NOTE(etudiants, nbEtudiants, id, ue, note);
        }
        else if (strcmp(commande, "CURSUS") == 0) {
            int id;
            scanf("%d", &id);
            CURSUS(etudiants, nbEtudiants, id);
        }
        else if (strcmp(commande, "ETUDIANTS") == 0) {
            ETUDIANTS(etudiants, nbEtudiants);
        }
        else if (strcmp(commande, "JURY") == 0) {
            int semestre;
            scanf("%d", &semestre);
            JURY(etudiants, nbEtudiants, semestre);
        }
        else if (strcmp(commande, "DEMISSION") == 0) {
            int id;
            scanf("%d", &id);
            DEMISSION(etudiants, nbEtudiants, id);
        }
        else if (strcmp(commande, "DEFAILLANCE") == 0) {
            int id;
            scanf("%d", &id);
            DEFAILLANCE(etudiants, nbEtudiants, id);
        }
        else if (strcmp(commande, "BILAN") == 0) {
            int AnneeNumero;
            scanf("%d", &AnneeNumero);
            BILAN(etudiants, nbEtudiants, AnneeNumero);
        }
        else {
            printf("Commande inconnue\n");
        }
    }
    return 0;
}



