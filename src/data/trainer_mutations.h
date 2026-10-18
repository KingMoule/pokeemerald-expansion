// Mutations des adversaires de fin de campagne (wiki section 13 : « un Pokémon muté apportant
// une couverture différente »). Une entrée mute le dernier Pokémon de l'espèce dans l'équipe.
// Choix provisoires : le nouveau type donne le STAB à une attaque que le Pokémon connaît déjà.

static const struct TrainerMutation sTrainerMutations[] =
{
    // Conseil 4 et Maître.
    { TRAINER_SIDNEY,  SPECIES_ABSOL,     TYPE_DARK,   TYPE_FLYING }, // Aéropique devient STAB.
    { TRAINER_PHOEBE,  SPECIES_DUSCLOPS,  TYPE_GHOST,  TYPE_GROUND }, // Séisme devient STAB.
    { TRAINER_GLACIA,  SPECIES_GLALIE,    TYPE_ICE,    TYPE_GHOST  }, // Ball'Ombre devient STAB.
    { TRAINER_DRAKE,   SPECIES_SALAMENCE, TYPE_FLYING, TYPE_FIRE   }, // Dragon/Feu : Lance-Flammes devient STAB.
    { TRAINER_WALLACE, SPECIES_GYARADOS,  TYPE_FLYING, TYPE_GROUND }, // Eau/Sol : Séisme devient STAB.
    // Dernières revanches des champions d'arène (combats doubles).
    { TRAINER_ROXANNE_5,       SPECIES_STEELIX,  TYPE_GROUND,  TYPE_ROCK   }, // Acier/Roche : Éboulement.
    { TRAINER_WATTSON_5,       SPECIES_MAGNETON, TYPE_STEEL,   TYPE_WATER  }, // Électrik/Eau sous la pluie.
    { TRAINER_NORMAN_5,        SPECIES_SLAKING,  TYPE_NORMAL,  TYPE_GHOST  }, // Ball'Ombre devient STAB.
    { TRAINER_WINONA_5,        SPECIES_ALTARIA,  TYPE_FLYING,  TYPE_GROUND }, // Dragon/Sol : Séisme.
    { TRAINER_TATE_AND_LIZA_5, SPECIES_SOLROCK,  TYPE_PSYCHIC, TYPE_FIRE   }, // Roche/Feu au soleil.
    { TRAINER_JUAN_5,          SPECIES_KINGDRA,  TYPE_DRAGON,  TYPE_ICE    }, // Eau/Glace : Laser Glace.
    // Défis à règles annoncées.
    { TRAINER_CHALLENGE_ELEMENT,     SPECIES_CHARIZARD, TYPE_FLYING, TYPE_DRAGON },
    { TRAINER_CHALLENGE_DOUBLE,      SPECIES_KINGDRA,   TYPE_DRAGON, TYPE_ICE    },
    { TRAINER_CHALLENGE_VARIETY,     SPECIES_ALTARIA,   TYPE_FLYING, TYPE_FIRE   },
    { TRAINER_CHALLENGE_RISING,      SPECIES_BLAZIKEN,  TYPE_FIRE,   TYPE_ELECTRIC },
    { TRAINER_CHALLENGE_ENDURANCE_2, SPECIES_SALAMENCE, TYPE_FLYING, TYPE_GROUND },
};
